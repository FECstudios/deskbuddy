import sys
import time
import logging
import webbrowser
from ctypes import cast, POINTER

import serial
import serial.tools.list_ports

from comtypes import CLSCTX_ALL, CoInitialize, CoUninitialize
from pycaw.pycaw import AudioUtilities, IAudioEndpointVolume
from pycaw.constants import CLSID_MMDeviceEnumerator
from comtypes.client import CreateObject
from pycaw.pycaw import IMMDeviceEnumerator, EDataFlow, ERole


logging.basicConfig(
    level=logging.INFO,
    format="[%(asctime)s] %(levelname)s  %(message)s",
    datefmt="%H:%M:%S",
)
log = logging.getLogger("DeskBuddy")

BAUD_RATE          = 115200
RECONNECT_DELAY_S  = 3          # seconds between reconnect attempts
READ_TIMEOUT_S     = 1          # serial read timeout
ESP32_USB_IDS      = [          # known ESP32 USB vendor IDs (decimal)
    0x10C4,  # Silicon Labs CP210x
    0x1A86,  # CH340 / CH341
    0x0403,  # FTDI
    0x303A,  # Espressif native USB (ESP32-S3 etc.)
]


# ─── Port Auto-Detection ────────────────────────────────────────────────────

def find_esp32_port() -> str | None:
    """Scan available COM ports and return the first that looks like an ESP32."""
    ports = serial.tools.list_ports.comports()
    for p in ports:
        vid = p.vid  # may be None for ports without USB info
        if vid is not None and vid in ESP32_USB_IDS:
            log.info("Auto-detected ESP32 on %s  (VID=0x%04X, %s)", p.device, vid, p.description)
            return p.device
        # Fallback: description heuristic
        desc = (p.description or "").lower()
        if any(k in desc for k in ("cp210", "ch340", "ch341", "ftdi", "esp", "uart")):
            log.info("Auto-detected likely ESP32 on %s  (%s)", p.device, p.description)
            return p.device

    log.warning("No ESP32 detected. Available ports: %s",
                [p.device for p in ports] or ["none"])
    return None


# ─── Windows Audio — Master Volume ─────────────────────────────────────────

def get_volume_controller() -> POINTER(IAudioEndpointVolume) | None:
    """Return a pycaw volume controller for the default audio playback endpoint."""
    try:
        devices = AudioUtilities.GetSpeakers()
        interface = devices.Activate(IAudioEndpointVolume._iid_, CLSCTX_ALL, None)
        ctrl = cast(interface, POINTER(IAudioEndpointVolume))
        log.info("Volume controller acquired.")
        return ctrl
    except Exception as exc:
        log.error("Could not acquire volume controller: %s", exc)
        return None


def set_system_volume(ctrl: POINTER(IAudioEndpointVolume), percent: int) -> None:
    """Set Windows master volume to *percent* (0–100)."""
    scalar = max(0.0, min(1.0, percent / 100.0))
    try:
        ctrl.SetMasterVolumeLevelScalar(scalar, None)
        log.info("Volume → %d%%", percent)
    except Exception as exc:
        log.error("set_system_volume failed: %s", exc)


# ─── Windows Audio — Microphone Mute ───────────────────────────────────────

def get_mic_controller() -> POINTER(IAudioEndpointVolume) | None:
    """Return a pycaw volume/mute controller for the default capture endpoint."""
    try:
        enumerator = CreateObject(CLSID_MMDeviceEnumerator, interface=IMMDeviceEnumerator)
        mic_device = enumerator.GetDefaultAudioEndpoint(EDataFlow.eCapture.value, ERole.eConsole.value)
        interface   = mic_device.Activate(IAudioEndpointVolume._iid_, CLSCTX_ALL, None)
        ctrl        = cast(interface, POINTER(IAudioEndpointVolume))
        log.info("Microphone controller acquired.")
        return ctrl
    except Exception as exc:
        log.error("Could not acquire microphone controller: %s", exc)
        return None


def set_mic_mute(ctrl: POINTER(IAudioEndpointVolume), muted: bool) -> None:
    """Mute or unmute the default capture endpoint."""
    try:
        ctrl.SetMute(1 if muted else 0, None)
        log.info("Microphone → %s", "MUTED" if muted else "UNMUTED")
    except Exception as exc:
        log.error("set_mic_mute failed: %s", exc)


# ─── Macro ──────────────────────────────────────────────────────────────────

def run_macro() -> None:
    """Tarayıcıda YouTube'u açan makro."""
    log.info("MACRO triggered — Opening YouTube...")
    try:
        webbrowser.open("https://www.youtube.com")
    except Exception as exc:
        log.error("Failed to open YouTube: %s", exc)


# ─── PC to ESP32 Time Sync ──────────────────────────────────────────────────

def send_time_sync(ser: serial.Serial) -> None:
    """Sends the current local PC time to the ESP32."""
    if ser and ser.is_open:
        time_str = time.strftime("TIME:%H:%M:%S\n")
        try:
            ser.write(time_str.encode("utf-8"))
            log.info("Time sync sent to ESP32: %s", time_str.strip())
        except Exception as exc:
            log.error("Failed to send time sync: %s", exc)


# ─── Message Dispatch ───────────────────────────────────────────────────────

def handle_message(line: str, vol_ctrl, mic_ctrl) -> None:
    """Parse and act on a single serial line from the ESP32."""
    line = line.strip()
    if not line:
        return

    if line.startswith("VOL:"):
        raw = line[4:]
        try:
            percent = int(raw)
        except ValueError:
            log.warning("Bad VOL value: %r", raw)
            return
        if not 0 <= percent <= 100:
            log.warning("VOL out of range: %d", percent)
            return
        if vol_ctrl:
            set_system_volume(vol_ctrl, percent)
        else:
            log.warning("VOL command received but volume controller unavailable.")

    elif line == "MIC:ON":
        if mic_ctrl:
            set_mic_mute(mic_ctrl, muted=False)
        else:
            log.warning("MIC:ON received but mic controller unavailable.")

    elif line == "MIC:OFF":
        if mic_ctrl:
            set_mic_mute(mic_ctrl, muted=True)
        else:
            log.warning("MIC:OFF received but mic controller unavailable.")

    elif line == "MACRO:RUN":
        run_macro()

    else:
        log.debug("Unknown message: %r", line)


# ─── Serial Loop (with auto-reconnect & sync) ──────────────────────────────

def serial_loop(vol_ctrl, mic_ctrl) -> None:
    """Main loop: open serial port, read lines, sync time, reconnect on failure."""
    ser: serial.Serial | None = None
    last_sync_time = 0.0

    while True:
        # ── Connect ───────────────────────────────────────────────────
        if ser is None or not ser.is_open:
            port = find_esp32_port()
            if port is None:
                log.info("Waiting %ds before retrying port scan …", RECONNECT_DELAY_S)
                time.sleep(RECONNECT_DELAY_S)
                continue

            try:
                ser = serial.Serial(port, BAUD_RATE, timeout=READ_TIMEOUT_S)
                log.info("Connected to %s @ %d baud.", port, BAUD_RATE)
                
                send_time_sync(ser)
                last_sync_time = time.time()
                
            except serial.SerialException as exc:
                log.error("Could not open %s: %s — retrying in %ds …", port, exc, RECONNECT_DELAY_S)
                time.sleep(RECONNECT_DELAY_S)
                ser = None
                continue

        # ── Periyodik Zaman Senkronizasyonu (60 Saniyede Bir) ──────────
        if time.time() - last_sync_time > 60:
            send_time_sync(ser)
            last_sync_time = time.time()

        # ── Read ──────────────────────────────────────────────────────
        try:
            raw = ser.readline()
            if not raw:
                continue
            line = raw.decode("utf-8", errors="ignore").strip()
            if line:
                handle_message(line, vol_ctrl, mic_ctrl)

        except (serial.SerialException, OSError) as exc:
            log.warning("Serial/OS error: %s — disconnected. Reconnecting …", exc)
            try:
                ser.close()
            except Exception:
                pass
            ser = None
            time.sleep(RECONNECT_DELAY_S)


# ─── Entry Point ────────────────────────────────────────────────────────────

def main() -> None:
    CoInitialize()
    try:
        log.info("DeskBuddy listener starting …")

        vol_ctrl = get_volume_controller()
        mic_ctrl = get_mic_controller()

        if vol_ctrl is None:
            log.warning("Volume control unavailable — VOL commands will be ignored.")
        if mic_ctrl is None:
            log.warning("Mic control unavailable — MIC commands will be ignored.")

        serial_loop(vol_ctrl, mic_ctrl)

    except KeyboardInterrupt:
        log.info("Interrupted by user. Goodbye.")
        sys.exit(0)
    finally:
        CoUninitialize()


if __name__ == "__main__":
    main()
