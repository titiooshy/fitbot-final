// INCLUDED LIBRARIES (No Change)
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_LPS2X.h>
#include <Adafruit_Sensor.h>
#include "MAX30105.h"
#include "heartRate.h"

// OLED SETUP (No Change)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// PRESSURE + TEMP SENSOR (No Change)
Adafruit_LPS22 lps;

// MAX30102 HEART RATE (No Change)
MAX30105 particleSensor;

const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int BPM = 0;

// TEMPERATURE AVERAGING (No Change)
#define TEMP_SAMPLES 10
float tempBuffer[TEMP_SAMPLES];
int tempIndex = 0;
bool tempBufferFilled = false;
float avgTempC = 0.0;

// BUZZER (No Change)
int buzzerPin = 7;

// SAFE RANGES (No Change)
#define BPM_LOW   50
#define BPM_HIGH  120
#define TEMP_LOW  35.0
#define TEMP_HIGH 39.0

unsigned long lastAlertTime = 0;
unsigned long alertCooldown = 5000;

// WORKOUT SYSTEM (No Change)
enum WorkoutPhase { WORK, COOLDOWN };
WorkoutPhase workoutPhase = WORK;

const char* workouts[] = {"PUSH-UPS", "SIT-UPS", "JUMP JACKS"};
int currentWorkout = 0;

const unsigned long WORK_TIME = 300000;     // 5 min
const unsigned long COOLDOWN_TIME = 300000; // 5 min

unsigned long phaseStartTime = 0;
bool workoutRunning = false;

// SCREENS (No Change)
enum Screen { HOME, VITALS, EXERCISE, SCREEN_COUNT };
Screen currentScreen = HOME;
Screen lastScreen = HOME;

// **********************************************
// TIMER LOGIC FOR PANNING MODE (NEW/MODIFIED)
// **********************************************
unsigned long lastScreenSwitchTime = 0;
const unsigned long SCREEN_DISPLAY_DURATION = 5000; // Display each screen for 5 seconds

// BUTTONS (DELETED/UNUSED)
// const int screenButtonPin = 2;
// const int workoutButtonPin = 3;
// int lastScreenButtonState = HIGH;
// int lastWorkoutButtonState = HIGH;
// const unsigned long debounceDelay = 50;
// unsigned long lastScreenPress = 0;
// unsigned long lastWorkoutPress = 0;

// WIFI + TIME (No Change)
const char* ssid     = "OLIN-DEVICES";
const char* password = "BestOval4Engineers!";

// NOTES (No Change)
#define NOTE_C5 523
#define NOTE_A5 880
#define NOTE_C4 262
#define NOTE_E4 330
#define NOTE_G4 392
#define NOTE_C6 1047

// ALERT SOUNDS (No Change)
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

// HEADER (No Change)
void drawHeader(const char* title) {
  display.fillRect(0, 0, 128, 14, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setCursor(2, 3);
  display.setTextSize(1);
  display.println(title);
  display.setTextColor(SH110X_WHITE);
}

// TEMP AVERAGING FUNCTION (No Change)
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

  for (int i = 0; i < count; i++)
    sum += tempBuffer[i];

  return sum / count;
}

// HOME SCREEN (Removed "Press button" line)
void drawHome() {
  display.clearDisplay();
  drawHeader("HOME");

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
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
  // Removed button instruction to save screen space
  // display.setCursor(10, 95);
  // display.setTextSize(1);
  // display.println("Press button to switch");
  
  display.display();
}

// VITALS SCREEN (No Change)
void drawVitals(float tempC, float pressureHPA, int bpm) {
  display.clearDisplay();
  drawHeader("VITALS");

  display.setCursor(10, 20);
  display.println("Temp:");
  display.setCursor(10, 35);
  display.setTextSize(2);
  display.print(tempC, 1);
  display.println(" C");

  display.setTextSize(1);
  display.setCursor(10, 60);
  display.println("Pressure:");
  display.setCursor(10, 75);
  display.setTextSize(2);
  display.print(pressureHPA, 1);
  display.println(" hPa");

  display.setTextSize(1);
  display.setCursor(10, 100);
  display.println("Heart Rate:");
  display.setCursor(10, 115);
  display.setTextSize(2);
  display.print(bpm);
  display.println(" BPM");

  display.display();
}

// WORKOUT SCREEN (Modified to automatically start workout)
void drawWorkoutScreen(int bpm, bool running) {
  display.clearDisplay();
  
  // Since there's no button, we assume the workout is running 
  // or it will just display the COOLDOWN screen if phases are off.
  // We'll force the workout to start when this screen is active for simplicity.
  if (!running) {
    workoutRunning = true;
    phaseStartTime = millis();
    workoutPhase = WORK; // Ensure it starts on WORK phase
  }
  
  drawHeader((workoutPhase == WORK) ? workouts[currentWorkout] : "COOLDOWN");

  unsigned long now = millis();
  unsigned long elapsed = now - phaseStartTime;
  unsigned long phaseLength = (workoutPhase == WORK) ? WORK_TIME : COOLDOWN_TIME;
  unsigned long remaining = (phaseLength - elapsed) / 1000;

  int min = remaining / 60;
  int sec = remaining % 60;

  display.setCursor(25, 30);
  display.setTextSize(2);
  display.println("TIME");

  display.setCursor(25, 55);
  display.printf("%d:%02d", min, sec);

  display.setCursor(35, 85);
  display.setTextSize(1);
  display.println("BPM");

  display.setCursor(40, 100);
  display.setTextSize(2);
  display.println(bpm);

  if (elapsed >= phaseLength) {
    phaseStartTime = now;
    workoutPhase = (workoutPhase == WORK) ? COOLDOWN : WORK;
    if (workoutPhase == WORK)
      currentWorkout = (currentWorkout + 1) % 3;
  }
  
  display.display();
}

// SETUP (Modified to remove button initialization and add Wi-Fi timeout)
void setup() {
  Serial.begin(115200);

  display.begin(0x3C, true);
  display.setRotation(0);
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.display();

  pinMode(buzzerPin, OUTPUT);
  // Removed pinMode for screenButtonPin and workoutButtonPin

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  // Added a timeout for connection
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) { 
    delay(200);
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi!");
    configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  } else {
    Serial.println("WiFi connection failed (continuing program).");
  }


  if (!lps.begin_I2C()) Serial.println("LPS22 failed! (Continuing program)");
  lps.setDataRate(LPS22_RATE_10_HZ);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) Serial.println("MAX30105 failed! (Continuing program)");
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);
  
  lastScreenSwitchTime = millis(); // Initialize the screen timer
}

// LOOP (Modified heavily to remove button logic and implement screen cycling)
void loop() {
  unsigned long now = millis();

  // **********************************************
  // --- AUTOMATIC SCREEN SWITCHING ---
  // **********************************************
  if (now - lastScreenSwitchTime > SCREEN_DISPLAY_DURATION) {
    currentScreen = (Screen)((currentScreen + 1) % SCREEN_COUNT);
    lastScreenSwitchTime = now;
  }

  // --- HEART RATE --- (No Change)
  long irValue = particleSensor.getIR();
  if (irValue > 7000 && checkForBeat(irValue)) {
    long delta = millis() - lastBeat;
    lastBeat = millis();
    beatsPerMinute = 60 / (delta / 1000.0);
    if (beatsPerMinute > 20 && beatsPerMinute < 255) {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;
      BPM = 0;
      for (byte i = 0; i < RATE_SIZE; i++) BPM += rates[i];
      BPM /= RATE_SIZE;
    }
    tone(buzzerPin, NOTE_C5, 40);
  }

  // --- TEMPERATURE & PRESSURE --- (No Change)
  avgTempC = getAveragedTemperature();
  sensors_event_t temp, pressure;
  lps.getEvent(&pressure, &temp);

  // --- ALERTS --- (No Change)
  if (now - lastAlertTime > alertCooldown) {
    if (BPM > 0 && (BPM < BPM_LOW || BPM > BPM_HIGH)) playBPMAlert();
    if (avgTempC > 0 && (avgTempC < TEMP_LOW || avgTempC > TEMP_HIGH)) playTempAlert();
    lastAlertTime = now;
  }

  // --- DRAW SCREENS --- (No Change)
  switch (currentScreen) {
    case HOME: drawHome(); break;
    case VITALS: drawVitals(avgTempC, pressure.pressure, BPM); break;
    case EXERCISE: drawWorkoutScreen(BPM, workoutRunning); break;
  }

  lastScreen = currentScreen;
}
