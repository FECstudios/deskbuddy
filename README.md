
# DESK-BUDDY

A desktop tool for quickly accesing some of the most commonly used media features on computers, right on your desk.

We built this device because checking your media controls while you are working on something or while gaming is anoyying. It's a compact, 3D-printed desktop companion featuring a 1.8" LCD screen, a rotary encoder, and two touch sensors. 

Instead of cluttering your computer screen, this device mirrors your actions instantly. Rotate the encoder to set the  volume or mute sounds, 
Two 14-year-olds from Ankara, Turkey made this. It sits right under your monitor.

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


## What it does
* **Rotate the encoder** -> Adjust system volume. The 1.8" LCD instantly shows a visual volume slider updating in real-time.
* **Click the encoder** -> Mute audio. The screen status switches to a muted speaker graphic.
* **Touch Sensor 1 (Microphone)** -> Hardware mute toggle for your mic. The screen instantly displays a crossed-out microphone icon so you always know your exact status.
* **Touch Sensor 2 (Macro)** -> Triggers a custom PC macro (such as a screenshot or clip saving) with a visual confirmation popup on the LCD.

## Hardware
| Component | What it's for |
| :--- | :--- |
| **ESP32 (38-pin)** | The brain. Handles inputs, updates the screen state, and talks to the PC via serial. |
| **1.8" ST7735 LCD Screen** | UI Display. Shows active status icons (mic status, volume bar, mute states). |
| **Rotary Encoder (EC11)** | Digital volume knob + push button for master mute. |
| **TTP223 Touch Sensors (x2)** | Capacitive touch buttons for microphone mute and custom macro shortcuts. |
| **3D-Printed Case** | Custom designed enclosure to fit perfectly into a desktop setup. |

## How it works
The ESP32 connects to your PC via USB and communicates with a lightweight companion script running on your computer to stay perfectly in sync.

1. **Input & Feedback:** When you interact with the hardware (rotating the knob or tapping a touch sensor), the ESP32 sends the command to the PC and instantly updates its own screen to reflect that action.
2. **Status Mirroring:** If you mute your microphone via the touch sensor, the PC script confirms the action, and the 1.8" LCD switches the UI to show a clear crossed-out microphone indicator. No more guessing if you are muted during Discord calls or online sessions.
3. **UI Rendering:** The screen acts as a dedicated hardware dashboard, drawing clean icons and progress bars based on the current state of your system audio and inputs.

## Setup

### Prerequisites
* Arduino IDE 
* ESP32 Board Package (configured for 38-pin ESP32 Dev Module)
* Python 3.x (for the PC side volume/mic control script)
* Libraries: `Adafruit_GFX`, `Adafruit_ST7735`
