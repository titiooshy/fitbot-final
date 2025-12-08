// Included Libraries for the code 
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_LPS2X.h>
#include <Adafruit_Sensor.h>

// Initializing the OLED Screen 
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Initializing the Pressure and Temperature Sensor
#define LPS_CS   10
#define LPS_SCK  13
#define LPS_MISO 12
#define LPS_MOSI 11
Adafruit_LPS22 lps;

// Initializing the Pulse Sensor
#define PULSE_PIN 0   // change to 34 on ESP32
int pulseSignal;
int pulseThreshold = 550;

unsigned long lastBeatTime = 0;
int BPM = 0;
bool beatDetected = false;

// Initializing the Speaker 
int buzzerPin = 7;

// Safe Ranges
#define BPM_LOW   50
#define BPM_HIGH  120

#define TEMP_LOW  35.0
#define TEMP_HIGH 39.0

unsigned long lastAlertTime = 0;
unsigned long alertCooldown = 5000;
//------------------------------------------------------------------------------

// WORKOUT SYSTEM
enum WorkoutPhase { WORK, COOLDOWN };
WorkoutPhase workoutPhase = WORK;

const char* workouts[] = {"PUSH-UPS", "SIT-UPS", "JUMP JACKS"};
int currentWorkout = 0;

const unsigned long WORK_TIME = 300000;     // 5 min
const unsigned long COOLDOWN_TIME = 300000; // 5 min

unsigned long phaseStartTime = 0;
bool workoutRunning = false;
//------------------------------------------------------------------------------

// Screen Layout 
enum Screen { HOME, VITALS, EXERCISE, SCREEN_COUNT };
Screen currentScreen = HOME;

unsigned long lastSwitch = 0;
unsigned long interval = 2500;

// WIFI and Time Setup
const char* ssid     = "OLIN-DEVICES";
const char* password = "BestOval4Engineers!";

bool getRTC(struct tm * timeinfo) {
  return getLocalTime(timeinfo);
}

// Speaker Musical Notes
#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_G5 784
#define NOTE_A5 880

#define NOTE_C4 262
#define NOTE_E4 330
#define NOTE_G4 392
#define NOTE_C6 1047
//------------------------------------------------------------------------------

// ALERT SOUNDS
void playBPMAlert() {
  tone(buzzerPin, NOTE_A5, 150);
  delay(200);
  tone(buzzerPin, NOTE_A5, 150);
  delay(200);
  noTone(buzzerPin);
}

void playTempAlert() {
  tone(buzzerPin, NOTE_C4, 200);
  delay(250);
  tone(buzzerPin, NOTE_E4, 200);
  delay(250);
  tone(buzzerPin, NOTE_G4, 200);
  delay(250);
  tone(buzzerPin, NOTE_C6, 400);
  delay(400);
  noTone(buzzerPin);
}
//------------------------------------------------------------------------------

// Screen Header
void drawHeader(const char* title) {
  display.fillRect(0, 0, 128, 14, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setCursor(2, 3);
  display.setTextSize(1);
  display.println(title);
  display.setTextColor(SH110X_WHITE);
}
//------------------------------------------------------------------------------

// HOME SCREEN
void drawHome() {
  display.clearDisplay();
  drawHeader("HOME");

  struct tm timeinfo;
  if (getRTC(&timeinfo)) {
    char timeStr[8];
    strftime(timeStr, sizeof(timeStr), "%I:%M %p", &timeinfo);
    if (timeStr[0] == '0') for (int i = 0; i < 7; i++) timeStr[i] = timeStr[i + 1];

    display.setCursor(10, 30);
    display.setTextSize(2);
    display.println(timeStr);

    char dateStr[20];
    strftime(dateStr, sizeof(dateStr), "%a %b %d", &timeinfo);

    display.setCursor(15, 55);
    display.setTextSize(1);
    display.println(dateStr);
  }

  display.drawLine(0, 80, 128, 80, SH110X_WHITE);
  display.setCursor(10, 95);
  display.setTextSize(1);
  display.println("Swipe or wait");
  display.display();
}
//-----------------------------------------------------------------------------

// VITALS SCREEN
void drawVitals(float tempC, float pressureHPA, int bpm) {
  display.clearDisplay();
  drawHeader("VITALS");

  display.setCursor(10, 20);
  display.setTextSize(1);
  display.println("Temp:");
  display.setCursor(10, 35);
  display.setTextSize(2);
  display.print(tempC, 1);
  display.println(" C");

  display.setCursor(10, 60);
  display.setTextSize(1);
  display.println("Pressure:");
  display.setCursor(10, 75);
  display.setTextSize(2);
  display.print(pressureHPA, 1);
  display.println(" hPa");

  display.setCursor(10, 100);
  display.setTextSize(1);
  display.println("Heart Rate:");
  display.setCursor(10, 115);
  display.setTextSize(2);
  display.print(bpm);
  display.println(" BPM");

  display.display();
}

//------------------------------------------------------------------------------

// WORKOUT + COOLDOWN SCREEN
void drawWorkoutScreen(int bpm) {
  display.clearDisplay();

  drawHeader(workoutPhase == WORK ? workouts[currentWorkout] : "COOLDOWN");

  unsigned long now = millis();
  unsigned long elapsed = now - phaseStartTime;
  unsigned long phaseLength = (workoutPhase == WORK) ? WORK_TIME : COOLDOWN_TIME;
  unsigned long remaining = (phaseLength - elapsed) / 1000;

  int min = remaining / 60;
  int sec = remaining % 60;

  display.setCursor(25, 30);
  display.setTextSize(2);
  display.print("TIME");

  display.setCursor(25, 55);
  if (sec < 10)
    display.printf("%d:0%d", min, sec);
  else
    display.printf("%d:%d", min, sec);

  display.setCursor(35, 85);
  display.setTextSize(1);
  display.println("BPM");

  display.setCursor(40, 100);
  display.setTextSize(2);
  display.println(bpm);

  display.display();

  // AUTO SWITCH
  if (elapsed >= phaseLength) {
    phaseStartTime = now;

    if (workoutPhase == WORK) {
      workoutPhase = COOLDOWN;
    } else {
      workoutPhase = WORK;
      currentWorkout = (currentWorkout + 1) % 3;
    }
  }
}
//------------------------------------------------------------------------------

// SETUP
void setup() {
  Serial.begin(115200);

  display.begin(0x3C, true);
  display.setRotation(4);
  display.setTextColor(SH110X_WHITE);
  display.display();

  pinMode(buzzerPin, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(200);

  const long gmtOffset_sec = -5 * 3600;
  configTime(gmtOffset_sec, 0, "pool.ntp.org", "time.nist.gov");

  if (!lps.begin_SPI(LPS_CS, LPS_SCK, LPS_MISO, LPS_MOSI)) while (1);
  lps.setDataRate(LPS22_RATE_10_HZ);
}
//------------------------------------------------------------------------------

// LOOP
void loop() {
  unsigned long now = millis();

  if (now - lastSwitch > interval) {
    currentScreen = (Screen)((currentScreen + 1) % SCREEN_COUNT);
    lastSwitch = now;
  }

  pulseSignal = analogRead(PULSE_PIN);

  if (pulseSignal > pulseThreshold && !beatDetected) {
    unsigned long beatNow = millis();
    unsigned long delta = beatNow - lastBeatTime;
    lastBeatTime = beatNow;

    if (delta > 300) {
      BPM = 60000 / delta;
      beatDetected = true;
    }
  }

  if (pulseSignal < pulseThreshold) {
    beatDetected = false;
  }

  sensors_event_t temp, pressure;
  lps.getEvent(&pressure, &temp);

  if (now - lastAlertTime > alertCooldown) {
    if (BPM < BPM_LOW || BPM > BPM_HIGH) playBPMAlert();
    if (temp.temperature < TEMP_LOW || temp.temperature > TEMP_HIGH) playTempAlert();
    lastAlertTime = now;
  }

  switch (currentScreen) {
    case HOME:   drawHome(); break;
    case VITALS: drawVitals(temp.temperature, pressure.pressure, BPM); break;

    case EXERCISE:
      if (!workoutRunning) {
        workoutRunning = true;
        phaseStartTime = millis();
      }
      drawWorkoutScreen(BPM);
      break;
  }
}
