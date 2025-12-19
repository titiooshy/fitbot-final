// ================= LIBRARIES =================
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_LPS2X.h>
#include <Adafruit_Sensor.h>
#include "MAX30105.h"
#include "heartRate.h"

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= ESP32 BUTTON PINS =================
#define MODE_BUTTON_PIN   25   // cycles screens
#define START_BUTTON_PIN  26   // start/stop workout

// ================= BUTTON STATES =================
int modeButtonState = HIGH;
int lastModeButtonState = HIGH;

int startButtonState = HIGH;
int lastStartButtonState = HIGH;

// ================= SENSORS =================
Adafruit_LPS22 lps;
MAX30105 particleSensor;

// ================= HEART RATE =================
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
int BPM = 0;

// ================= TEMPERATURE =================
#define TEMP_SAMPLES 10
float tempBuffer[TEMP_SAMPLES];
int tempIndex = 0;
bool tempBufferFilled = false;
float avgTempC = 0;

// ================= BUZZER =================
int buzzerPin = 7;

// ================= WORKOUT =================
enum WorkoutPhase { WORK, COOLDOWN };
WorkoutPhase workoutPhase = WORK;

const char* workouts[] = {"PUSH-UPS", "SIT-UPS", "JUMP JACKS"};
int currentWorkout = 0;

const unsigned long WORK_TIME = 300000;      // 5 min
const unsigned long COOLDOWN_TIME = 300000;  // 5 min

unsigned long phaseStartTime = 0;
bool workoutRunning = false;

// ================= SCREENS =================
enum Screen { HOME, VITALS, EXERCISE };
int currentScreen = HOME;

// ================= WIFI =================
const char* ssid = "OLIN-DEVICES";
const char* password = "BestOval4Engineers!";

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  display.begin(0x3C, true);
  display.setRotation(4);
  display.setTextColor(SH110X_WHITE);

  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(START_BUTTON_PIN, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(200);
  configTime(-5 * 3600, 0, "pool.ntp.org");

  lps.begin_SPI(10, 13, 12, 11);
  lps.setDataRate(LPS22_RATE_10_HZ);

  particleSensor.begin(Wire, I2C_SPEED_FAST);
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);
}

// ================= TEMP AVERAGE =================
float getAveragedTemperature() {
  sensors_event_t temp, pressure;
  lps.getEvent(&pressure, &temp);

  tempBuffer[tempIndex++] = temp.temperature;
  if (tempIndex >= TEMP_SAMPLES) {
    tempIndex = 0;
    tempBufferFilled = true;
  }

  int count = tempBufferFilled ? TEMP_SAMPLES : tempIndex;
  float sum = 0;
  for (int i = 0; i < count; i++) sum += tempBuffer[i];
  return sum / count;
}

// ================= BUTTON HANDLERS =================
void handleModeButton() {
  modeButtonState = digitalRead(MODE_BUTTON_PIN);

  if (modeButtonState != lastModeButtonState) {
    if (modeButtonState == LOW) {
      currentScreen++;
      if (currentScreen > EXERCISE) currentScreen = HOME;

      Serial.print("Screen Mode: ");
      Serial.println(currentScreen);
    }
    delay(10);
  }
  lastModeButtonState = modeButtonState;
}

void handleStartButton() {
  startButtonState = digitalRead(START_BUTTON_PIN);

  if (startButtonState != lastStartButtonState) {
    if (startButtonState == LOW) {
      if (currentScreen == EXERCISE) {
        workoutRunning = !workoutRunning;
        workoutPhase = WORK;
        phaseStartTime = millis();

        if (workoutRunning)
          Serial.println("Workout STARTED");
        else
          Serial.println("Workout STOPPED");
      }
    }
    delay(10);
  }
  lastStartButtonState = startButtonState;
}

// ================= DISPLAY HELPERS =================
void drawHeader(const char* title) {
  display.fillRect(0, 0, 128, 14, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setCursor(2, 3);
  display.println(title);
  display.setTextColor(SH110X_WHITE);
}

void drawHome() {
  display.clearDisplay();
  drawHeader("HOME");
  display.setCursor(20, 60);
  display.println("MODE to switch");
  display.display();
}

void drawVitals(float tempC, float pressure, int bpm) {
  display.clearDisplay();
  drawHeader("VITALS");
  display.setCursor(5, 30);
  display.printf("Temp: %.1f C", tempC);
  display.setCursor(5, 50);
  display.printf("Press: %.1f", pressure);
  display.setCursor(5, 70);
  display.printf("BPM: %d", bpm);
  display.display();
}

void drawWorkoutScreen(int bpm) {
  display.clearDisplay();
  drawHeader(workoutPhase == WORK ? workouts[currentWorkout] : "COOLDOWN");

  unsigned long elapsed = millis() - phaseStartTime;
  unsigned long length = (workoutPhase == WORK) ? WORK_TIME : COOLDOWN_TIME;

  if (elapsed >= length) {
    phaseStartTime = millis();
    workoutPhase = (workoutPhase == WORK) ? COOLDOWN : WORK;
    if (workoutPhase == WORK)
      currentWorkout = (currentWorkout + 1) % 3;
  }

  unsigned long remaining = (length - elapsed) / 1000;
  display.setCursor(30, 55);
  display.setTextSize(2);
  display.printf("%d:%02d", remaining / 60, remaining % 60);

  display.setTextSize(1);
  display.setCursor(45, 95);
  display.printf("BPM %d", bpm);
  display.display();
}

// ================= LOOP =================
void loop() {
  handleModeButton();
  handleStartButton();

  long irValue = particleSensor.getIR();
  if (irValue > 7000 && checkForBeat(irValue)) {
    long delta = millis() - lastBeat;
    lastBeat = millis();
    float bpm = 60 / (delta / 1000.0);

    if (bpm > 20 && bpm < 255) {
      rates[rateSpot++] = bpm;
      rateSpot %= RATE_SIZE;
      BPM = 0;
      for (byte i = 0; i < RATE_SIZE; i++) BPM += rates[i];
      BPM /= RATE_SIZE;
    }
  }

  avgTempC = getAveragedTemperature();
  sensors_event_t temp, pressure;
  lps.getEvent(&pressure, &temp);

  switch (currentScreen) {
    case HOME:
      drawHome();
      break;

    case VITALS:
      drawVitals(avgTempC, pressure.pressure, BPM);
      break;

    case EXERCISE:
      if (workoutRunning) {
        drawWorkoutScreen(BPM);
      } else {
        display.clearDisplay();
        drawHeader("EXERCISE");
        display.setCursor(20, 60);
        display.println("Press START");
        display.display();
      }
      break;
  }
}
