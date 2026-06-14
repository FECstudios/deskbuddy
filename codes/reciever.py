import serial
import time

from ctypes import cast, POINTER
from comtypes import CLSCTX_ALL
from pycaw.pycaw import AudioUtilities, IAudioEndpointVolume

devices = AudioUtilities.GetSpeakers()
interface = devices.Activate(IAudioEndpointVolume._iid_, CLSCTX_ALL, None)
volume_ctrl = cast(interface, POINTER(IAudioEndpointVolume))

mic_devices = AudioUtilities.GetMicrophone()
mic_interface = mic_devices.Activate(IAudioEndpointVolume._iid_, CLSCTX_ALL, None)
mic_ctrl = cast(mic_interface, POINTER(IAudioEndpointVolume))

PORT = "COM7"
BAUD = 115200

ser = serial.Serial(PORT, BAUD, timeout=1)

def set_system_volume(percent):
    volume_ctrl.SetMasterVolumeLevelScalar(percent / 100.0, None)

def set_mic_state(on: bool):
    mic_ctrl.SetMute(0 if on else 1, None)

def run_macro():
    print("Macro running")


print("Listening to my buddy...")
while True:
    line = ser.readline().decode(errors='ignore').strip()
    if not line:
        continue

    if line.startswith("VOL:"):
        vol = int(line.split(":")[1])
        set_system_volume(vol)
        print(f"Volume set to: {vol}")

    elif line.startswith("MIC:"):
        mic_on = line.split(":")[1] == "ON"
        set_mic_state(mic_on)
        print(f"Mic mode: {' ON' if mic_on else ' OFF'}")

    elif line.startswith("MACRO:"):
        run_macro()
