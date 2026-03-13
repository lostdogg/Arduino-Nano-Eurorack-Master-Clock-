#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Pin Definitions ---
#define POT_BPM     A0   
#define POT_SWING   A1   
#define START_STOP  12   
const int straightOuts[] = {6, 7, 8, 9}; 
const int swungOuts[] = {10, 11}; 

// --- Timing Variables ---
int bpm = 120;
int swingPercent = 0; 
bool isRunning = false;

unsigned long prevStraightMillis = 0;
unsigned long prevSwungMillis = 0;
unsigned long pulseTimerStraight = 0;
unsigned long pulseTimerSwung = 0;

const int pulseWidth = 10; 
bool straightActive = false;
bool swungActive = false;
bool isOffbeatSwung = false; 

void setup() {
  for(int i=0; i<4; i++) pinMode(straightOuts[i], OUTPUT);
  for(int i=0; i<2; i++) pinMode(swungOuts[i], OUTPUT);
  pinMode(START_STOP, INPUT_PULLUP);
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  updateDisplay();
}

void loop() {
  // 1. Start/Stop Logic
  bool switchState = (digitalRead(START_STOP) == LOW);
  if (switchState != isRunning) {
    isRunning = switchState;
    if (isRunning) {
      unsigned long m = millis();
      prevStraightMillis = m;
      prevSwungMillis = m;
      isOffbeatSwung = false;         
    } else {
      allOutputsLow();
    }
    updateDisplay();
  }

  // 2. Read Control Pots
  int rawBpm = analogRead(POT_BPM);
  int rawSwing = analogRead(POT_SWING);
  int newBpm = map(rawBpm, 0, 1023, 30, 300);
  int newSwing = map(rawSwing, 0, 1023, 0, 75);

  if (abs(newBpm - bpm) > 1 || abs(newSwing - swingPercent) > 1) {
    bpm = newBpm;
    swingPercent = newSwing;
    updateDisplay();
  }

  if (isRunning) {
    unsigned long currentMillis = millis();
    unsigned long baseInterval = 60000 / bpm; 

    // --- STRAIGHT CLOCK ENGINE (Pins 6-9) ---
    if (currentMillis - prevStraightMillis >= baseInterval) {
      prevStraightMillis = currentMillis;
      for(int i=0; i<4; i++) digitalWrite(straightOuts[i], HIGH);
      straightActive = true;
      pulseTimerStraight = currentMillis;
    }

    // --- SWUNG CLOCK ENGINE (Pins 10-11) ---
    float swingOffset = (baseInterval * (swingPercent / 100.0)) / 2.0;
    unsigned long currentSwungInterval = isOffbeatSwung ? (baseInterval - swingOffset) : (baseInterval + swingOffset);

    if (currentMillis - prevSwungMillis >= currentSwungInterval) {
      prevSwungMillis = currentMillis;
      for(int i=0; i<2; i++) digitalWrite(swungOuts[i], HIGH);
      swungActive = true;
      pulseTimerSwung = currentMillis;
      isOffbeatSwung = !isOffbeatSwung; 
    }

    // --- PULSE WIDTH CLEANUP ---
    if (straightActive && (currentMillis - pulseTimerStraight >= pulseWidth)) {
      for(int i=0; i<4; i++) digitalWrite(straightOuts[i], LOW);
      straightActive = false;
    }
    if (swungActive && (currentMillis - pulseTimerSwung >= pulseWidth)) {
      for(int i=0; i<2; i++) digitalWrite(swungOuts[i], LOW);
      swungActive = false;
    }
  }
}

void allOutputsLow() {
  for(int i=0; i<4; i++) digitalWrite(straightOuts[i], LOW);
  for(int i=0; i<2; i++) digitalWrite(swungOuts[i], LOW);
  straightActive = false;
  swungActive = false;
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(isRunning ? ">> RUNNING" : "|| STOPPED");
  display.setTextSize(3);
  display.setCursor(0, 15);
  display.print(bpm);
  display.setTextSize(1);
  display.print(" BPM");
  display.setCursor(0, 45);
  display.print("SWING: ");
  display.print(swingPercent);
  display.print("%");
  display.drawRect(0, 56, 128, 6, WHITE);
  display.fillRect(0, 56, map(swingPercent, 0, 75, 0, 128), 6, WHITE);
  display.display();
}