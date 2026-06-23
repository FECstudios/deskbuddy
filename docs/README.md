# DESK-BUDDY

A desk tool to control audio settings and access some cool macros easily!

We built this device because checking your media controls while you are working on something or while gaming is anoying. It's a compact, 3D-printed desktop companion featuring a 1.8" LCD screen, a rotary encoder, and two touch sensors. 

Instead of cluttering your computer screen, this device mirrors your actions instantly. Rotate the encoder to set the  volume or mute sounds, 

<img width="705" height="1000" alt="English Project" src="https://github.com/user-attachments/assets/1197e7d8-4c52-482f-8271-c04dd24c3865" />

## Gallery
<p align="center">
  <img src="https://github.com/user-attachments/assets/108f1965-df0a-46dc-80c2-bad23318c3a3" width="700" />
</p>
<p align="center">
<img width="3000" height="2198" alt="circuit_image (1)" src="https://github.com/user-attachments/assets/6217358c-e751-4df9-a215-24410a01eabc" />
</p>
<p align="center"><b>Perf board placement:</b></p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/e22524b3-4b7b-4882-aaa6-1dbc29643dd9" width="700" />
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/efc96c5a-7e23-4909-8809-da2a50fadd43" width="700" />
</p>



<p align="center">
  <img src="https://github.com/user-attachments/assets/aa8bd878-55d1-42cf-aa1d-78bac9b5a996" width="700" />
</p>


## What does it do
* **Rotate the encoder** -> Adjust system volume. The 1.8" LCD instantly shows a visual volume slider updating in real-time.
* **Click the encoder** -> Mute audio. The screen status switches to a muted speaker graphic.
* **Touch Sensor 1 (Macro)** -> Triggers a custom PC macro (such as a screenshot or opening youtube) with a visual confirmation popup on the LCD.
* **Touch Sensor 2 (Microphone)** -> Hardware mute toggle for your mic. The screen instantly displays a crossed-out microphone icon so you always know your exact status.


## Hardware
| Component | What it's for |
| :--- | :--- |
| **ESP32 Dev Board (38-pin)** | The microcontroller. Handles inputs, updates the screen state, and talks to the PC. |
| **1.8" ST7735 LCD Screen** | UI Display. Shows active status icons (mic status, volume bar, mute states). |
| **Rotary Encoder (KY-040)** | Digital volume knob + push button for master mute. |
| **TTP223 Touch Sensors (x2)** | Capacitive touch buttons for microphone mute and custom macro shortcuts. |
| **3D-Printed PLA Case** | Custom designed enclosure to fit perfectly into a desktop setup. |

## How it works
The ESP32 connects to your PC via USB and communicates with a lightweight companion script running on your computer to stay perfectly in sync.

1. **Input & Feedback:** When you interact with the hardware (rotating the knob or tapping a touch sensor), the ESP32 sends the command to the PC and instantly updates its own screen to reflect that action.
2. **Status Mirroring:** If you mute your microphone via the touch sensor, the PC script confirms the action, and the 1.8" LCD switches the UI to show a clear crossed-out microphone indicator. No more guessing if you are muted during Discord calls or online sessions.
3. **UI Rendering:** The screen acts as a dedicated hardware dashboard, drawing clean icons and progress bars based on the current state of your system audio and inputs.


## Building
1. **Printing**: Print the top.stl part with 0.20 layer height and add a pause on layer 14, then when the print pauses add your touch sensors with wires and pins removed and continue printing untill part is finished.
Then you can print rest of the parts (knob and bottom) normally with layer height of your choice
2. **Soldering**: Solder all of the components according to the schematics given above in *Gallery* and make sure to not burn the plastic while soldering the touch sensors. Solder the esp32 on the bottom of the perf board and solder the TFT screen on top of the esp32 as shown in the pictures above
3. **Assembly**: Slide in the perf board to the bottom part make sure the esp32 and the type c port hole allign and then slide in the rotary encoder on the left side of the bottom part. After all of the components are added simply click in the top and bottom parts together.

*The model was made with 0.4 tolerances in mind*


## Setup

### Prerequisites
* Arduino IDE 
* ESP32 Board Package (configured for 38-pin ESP32 Dev Module)
* Python 3.x (for the PC side volume/mic control script)
* Libraries: `Adafruit_GFX`, `Adafruit_ST7735`, `SPI`, `FreeSansBold18pt7b`, `FreeSans9pt7b` 

1. Upload `codes/deskbuddy.ino` to your ESP32 using the Arduino IDE (use a USB cable that supports data transfer).
2. Verify that all hardware components are working correctly.
3. Download `codes/sender.py` and install the required dependencies:

```bash
python -m pip install pyserial pycaw comtypes
```

4. Run:

```bash
python sender.py
```

Wait for the ESP32 to be detected, and you're ready to go.
