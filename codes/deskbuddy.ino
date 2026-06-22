#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

// ─── Screen Pin Definitions ───────────────────────────────────────────────────────
#define TFT_CS     5
#define TFT_DC     2
#define TFT_RST    4
#define TFT_MOSI   19
#define TFT_SCLK   18

const int TOUCH_PIN1 = 26;
const int TOUCH_PIN2 = 23;
const int TOUCH_PIN  = TOUCH_PIN1;
const int CLK_PIN   = 25;
const int DT_PIN    = 32;
const int SW_PIN    = 27;

// ─── Display Setup ─────────────────────────────────────────────────────────
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 canvas(160, 128);

// ─── State Variables ───────────────────────────────────────────────────────
int   volume        = 50;
bool isMicMuted     = false;
bool isIncreasing   = true; 

// Idle & Time System
unsigned long lastActivityTime = 0;
const unsigned long IDLE_TIMEOUT = 10000; 
int hr = 12, mn = 0, sc = 0;              
unsigned long lastClockTick = 0;

// Encoder
volatile int encoderDelta = 0;
int lastClkState;

// Button debounce / double-click
bool          lastButtonState  = HIGH;
unsigned long firstClickTime   = 0;
bool          waitingForDouble = false;
const unsigned long DOUBLE_CLICK_MS = 500;

// Touch
bool lastTouchState  = false;
bool lastTouch2State = false;

// ─── Forward Declarations ──────────────────────────────────────────────────
void drawScreen();
void pushCanvas();
void playM1Animation();
void playMuteAnimation();
void sendVolume();
void sendMicState();
void handleSerialInput();
void IRAM_ATTR encoderISR();

// ─── Drawing ───────────────────────────────────────────────────────────────

void drawScreen() {
  canvas.fillScreen(ST7735_BLACK);

  // 1) IDLE SCREEN
  if (millis() - lastActivityTime > IDLE_TIMEOUT) {
    canvas.setFont(&FreeSansBold18pt7b);
    canvas.setTextColor(ST7735_WHITE);
    
    char timeStr[9];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", hr, mn);
    
    int16_t x1, y1;
    uint16_t tw, th;
    canvas.getTextBounds(timeStr, 0, 0, &x1, &y1, &tw, &th);
    canvas.setCursor((160 - tw) / 2, 58);
    canvas.print(timeStr);
    
    canvas.drawFastHLine(15, 74, 130, 0x4444);
    
    canvas.setFont(&FreeSans9pt7b);
    
    char volStr[12];
    snprintf(volStr, sizeof(volStr), "VOL: %d%%", volume);
    canvas.setTextColor(ST7735_CYAN);
    canvas.setCursor(15, 104);
    canvas.print(volStr);
    
    if (isMicMuted) {
      canvas.setTextColor(ST7735_RED);
      canvas.setCursor(95, 104);
      canvas.print("MUTED");
    } else {
      canvas.setTextColor(ST7735_GREEN);
      canvas.setCursor(105, 104);
      canvas.print("LIVE");
    }
    return;
  }

  // 2) MIC MUTED SCREEN
  if (isMicMuted) {
    int16_t x1, y1;
    uint16_t tw, th;

    canvas.setFont(&FreeSansBold18pt7b);
    canvas.setTextColor(ST7735_RED);
    canvas.getTextBounds("MIC OFF", 0, 0, &x1, &y1, &tw, &th);
    canvas.setCursor((160 - tw) / 2, 58);
    canvas.print("MIC OFF");

    canvas.setFont(&FreeSans9pt7b);
    canvas.setTextColor(0x7800);
    canvas.getTextBounds("(muted)", 0, 0, &x1, &y1, &tw, &th);
    canvas.setCursor((160 - tw) / 2, 88);
    canvas.print("(muted)");
  }
  // 3) VOLUME CONTROL SCREEN
  else {
    canvas.setFont(&FreeSans9pt7b);
    canvas.setTextColor(isIncreasing ? ST7735_GREEN : ST7735_RED);
    canvas.setCursor(135, 22);
    canvas.print(isIncreasing ? "+" : "-");

    canvas.setFont(&FreeSansBold18pt7b);
    canvas.setTextColor(ST7735_WHITE);

    int16_t  x1, y1;
    uint16_t tw, th;
    char volStr[8];
    snprintf(volStr, sizeof(volStr), "%d", volume);
    canvas.getTextBounds(volStr, 0, 0, &x1, &y1, &tw, &th);

    int numX = (160 - (int)tw - 20) / 2;
    canvas.setCursor(numX, 68);
    canvas.print(volStr);

    canvas.setFont(&FreeSans9pt7b);
    canvas.setTextColor(0xC618);
    canvas.print("%");

    uint16_t barColor = isIncreasing ? ST7735_GREEN : ST7735_RED;
    canvas.drawRoundRect(14, 85, 132, 22, 6, ST7735_WHITE);
    int barW = map(volume, 0, 100, 0, 128);
    if (barW > 0) {
      canvas.fillRoundRect(16, 87, barW, 18, 4, barColor);
    }
  }
}

void pushCanvas() {
  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), 160, 128);
}

void updateDisplay() {
  drawScreen();
  pushCanvas();
}

// ─── M1 Animation ──────────────────────────────────────────────────────────
void playM1Animation() {
  for (int i = 0; i < 3; i++) {
    canvas.fillScreen(ST7735_BLACK);
    pushCanvas();
    delay(25);
  }

  canvas.setFont(&FreeSansBold18pt7b);
  for (int r = 0; r <= 31; r += 3) {
    canvas.fillScreen(ST7735_BLACK);
    canvas.setTextColor(((uint16_t)r) << 11);
    canvas.setCursor(50, 75);
    canvas.print("M1");
    pushCanvas();
    delay(12);
  }

  delay(500);

  for (int r = 31; r >= 0; r -= 3) {
    canvas.fillScreen(ST7735_BLACK);
    canvas.setTextColor(((uint16_t)r) << 11);
    canvas.setCursor(50, 75);
    canvas.print("M1");
    pushCanvas();
    delay(12);
  }

  updateDisplay();
}

void playMuteAnimation() {
  for (int i = 0; i < 3; i++) {
    canvas.fillScreen(ST7735_BLACK);
    pushCanvas();
    delay(25);
  }

  canvas.setFont(&FreeSansBold18pt7b);
  for (int r = 0; r <= 31; r += 3) {
    canvas.fillScreen(ST7735_BLACK);
    canvas.setTextColor(((uint16_t)r) << 11);
    canvas.setCursor(50, 75);
    canvas.print("MUTED");
    pushCanvas();
    delay(12);
  }

  delay(500);

  for (int r = 31; r >= 0; r -= 3) {
    canvas.fillScreen(ST7735_BLACK);
    canvas.setTextColor(((uint16_t)r) << 11);
    canvas.setCursor(50, 75);
    canvas.print("MUTED");
    pushCanvas();
    delay(12);
  }

  updateDisplay();
}

// ─── Serial Communication & Parsing ────────────────────────────────────────
void sendVolume() {
  Serial.print("VOL:");
  Serial.println(volume);
}

void sendMicState() {
  Serial.println(isMicMuted ? "MIC:OFF" : "MIC:ON");
}

void handleSerialInput() {
  while (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    // Expects "TIME:HH:MM:SS"
    if (input.startsWith("TIME:")) {
      int firstColon = input.indexOf(':');
      int secondColon = input.indexOf(':', firstColon + 1);
      int thirdColon = input.indexOf(':', secondColon + 1);
      
      if (firstColon != -1 && secondColon != -1) {
        String hrStr = input.substring(firstColon + 1, secondColon);
        String mnStr = (thirdColon != -1) ? input.substring(secondColon + 1, thirdColon) : input.substring(secondColon + 1);
        String scStr = (thirdColon != -1) ? input.substring(thirdColon + 1) : "0";
        
        hr = hrStr.toInt();
        mn = mnStr.toInt();
        sc = scStr.toInt();
        
        lastClockTick = millis();
        
        if (millis() - lastActivityTime > IDLE_TIMEOUT) {
          updateDisplay();
        }
      }
    }
    else if (input.startsWith("VOL:")) {
      String volStr = input.substring(4);
      int newVolume = volStr.toInt();
      newVolume = constrain(newVolume, 0, 100);
      if (newVolume != volume) {
        volume = newVolume;
        lastActivityTime = millis();
        updateDisplay();
      }
    }
  }
}

// ─── Encoder ISR ───────────────────────────────────────────────────────────
void IRAM_ATTR encoderISR() {
  int clkNow = digitalRead(CLK_PIN);
  if (clkNow != lastClkState && clkNow == LOW) {
    encoderDelta = (digitalRead(DT_PIN) != clkNow) ? 1 : -1;
  }
  lastClkState = clkNow;
}

// ─── Setup ─────────────────────────────────────────────────────────────────
void setup() {
  pinMode(TFT_CS,   OUTPUT);
  pinMode(TFT_DC,   OUTPUT);
  pinMode(TFT_RST,  OUTPUT);
  delay(500);

  Serial.begin(115200);

  pinMode(TOUCH_PIN, INPUT);
  pinMode(TOUCH_PIN2, INPUT);
  pinMode(CLK_PIN,   INPUT_PULLUP);
  pinMode(DT_PIN,    INPUT_PULLUP);
  pinMode(SW_PIN,    INPUT_PULLUP);

  lastClkState = digitalRead(CLK_PIN);
  attachInterrupt(digitalPinToInterrupt(CLK_PIN), encoderISR, CHANGE);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3); 

  canvas.fillScreen(ST7735_BLACK);
  lastActivityTime = millis();
  lastClockTick = millis();
  updateDisplay();

  Serial.println("DeskBuddy ready.");
}

// ─── Loop ──────────────────────────────────────────────────────────────────
void loop() {

  // Update clock in background
  if (millis() - lastClockTick >= 1000) {
    lastClockTick += 1000;
    sc++;
    if (sc >= 60) { sc = 0; mn++; }
    if (mn >= 60) { mn = 0; hr++; }
    if (hr >= 24) { hr = 0; }
    
    if (millis() - lastActivityTime > IDLE_TIMEOUT) {
      updateDisplay();
    }
  }

  handleSerialInput();

  // Handle Encoder
  if (encoderDelta != 0) {
    lastActivityTime = millis();
    int step = 2;

    if (isMicMuted) {
      isMicMuted  = false;
      isIncreasing = true;
      sendMicState();
    } else {
      if (isIncreasing) {
        volume = constrain(volume + (encoderDelta > 0 ? step : -step), 0, 100);
      } else {
        volume = constrain(volume + (encoderDelta > 0 ? -step : step), 0, 100);
      }
      sendVolume();
    }

    encoderDelta = 0;
    updateDisplay();
  }

  // Handle Button
  bool btnNow = digitalRead(SW_PIN);


    // Handle Touch
  bool touch2Now = (digitalRead(TOUCH_PIN2) == HIGH);

  if (touch2Now && !lastTouch2State) {
    lastActivityTime = millis();
    Serial.println("MUTE:RUN");
    isMicMuted = !isMicMuted;
    playMuteAnimation();
    sendMicState();
  }

  lastTouch2State = touch2Now;

  delay(5); 


  if (btnNow == LOW && lastButtonState == HIGH) {
    lastActivityTime = millis();
    unsigned long now = millis();

    volume = 0;
    sendVolume();

    updateDisplay();
  }

  if (waitingForDouble && (millis() - firstClickTime > DOUBLE_CLICK_MS)) {
    isIncreasing    = !isIncreasing;
    waitingForDouble = false;
    updateDisplay();
  }

  lastButtonState = btnNow;

  // Handle Touch
  bool touchNow = (digitalRead(TOUCH_PIN) == HIGH);

  if (touchNow && !lastTouchState) {
    lastActivityTime = millis();
    Serial.println("MACRO:RUN");
    playM1Animation();
  }

  lastTouchState = touchNow;

  delay(5); 
}
