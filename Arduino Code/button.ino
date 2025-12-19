// INCLUDED LIBRARIES
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_LPS2X.h>
#include <Adafruit_Sensor.h>
#include "MAX30105.h"
#include "heartRate.h" // Assuming this is a local utility file or library

// OLED SETUP
// Check your display's dimensions and I2C address (0x3C or 0x3D)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// PRESSURE + TEMP SENSOR
Adafruit_LPS22 lps;

// MAX30102 HEART RATE
MAX30105 particleSensor;

const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int BPM = 0;

// TEMPERATURE AVERAGING
#define TEMP_SAMPLES 10
float tempBuffer[TEMP_SAMPLES];
int tempIndex = 0;
bool tempBufferFilled = false;
float avgTempC = 0.0;

// BUZZER
int buzzerPin = 7;

// SAFE RANGES
#define BPM_LOW   -0
#define BPM_HIGH  120
#define TEMP_LOW  35.0
#define TEMP_HIGH 150.0

unsigned long lastAlertTime = 0;
unsigned long alertCooldown = 5000;

// WORKOUT SYSTEM
enum WorkoutPhase { WORK, COOLDOWN };
WorkoutPhase workoutPhase = WORK;

const char* workouts[] = {"PUSH-UPS", "SIT-UPS", "JUMP JACKS"};
int currentWorkout = 0;

const unsigned long WORK_TIME = 300000;     // 5 min (300,000 ms)
const unsigned long COOLDOWN_TIME = 300000; // 5 min (300,000 ms)

unsigned long phaseStartTime = 0;
bool workoutRunning = false;

// SCREENS
enum Screen { HOME, VITALS, EXERCISE, SCREEN_COUNT };
Screen currentScreen = HOME;
Screen lastScreen = HOME;

// BUTTONS
const int screenButtonPin = 2;
const int workoutButtonPin = 3;
int lastScreenButtonState = HIGH;
int lastWorkoutButtonState = HIGH;
const unsigned long debounceDelay = 50;
unsigned long lastScreenPress = 0;
unsigned long lastWorkoutPress = 0;

// WIFI + TIME (Update these for your local network)
const char* ssid     = "OLIN-DEVICES";
const char* password = "BestOval4Engineers!";

// NOTES
#define NOTE_C5 523
#define NOTE_A5 880
#define NOTE_C4 262
#define NOTE_E4 330
#define NOTE_G4 392
#define NOTE_C6 1047

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

// HEADER
void drawHeader(const char* title) {
  display.fillRect(0, 0, 128, 14, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setCursor(2, 3);
  display.setTextSize(1);
  display.println(title);
  display.setTextColor(SH110X_WHITE);
}

// TEMP AVERAGING FUNCTION
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

// HOME SCREEN
void drawHome() {
  display.clearDisplay();
  drawHeader("HOME");

  // Time-based drawing requires successful Wi-Fi and NTP sync
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char timeStr[8];
    // Use %I:%M %p for 12-hour clock (e.g., 08:17 PM)
    strftime(timeStr, sizeof(timeStr), "%I:%M %p", &timeinfo);
    
    // Remove leading zero for single-digit hours (e.g., 08:00 PM -> 8:00 PM)
    if (timeStr[0] == '0') for (int i = 0; i < 7; i++) timeStr[i] = timeStr[i + 1];

    display.setCursor(10, 30);
    display.setTextSize(2);
    display.println(timeStr);

    char dateStr[20];
    strftime(dateStr, sizeof(dateStr), "%a %b %d", &timeinfo); // e.g., Tue Dec 16

    display.setCursor(15, 55);
    display.setTextSize(1);
    display.println(dateStr);
  }

  display.drawLine(0, 80, 128, 80, SH110X_WHITE);
  display.setCursor(10, 95);
  display.setTextSize(1);
  display.println("Press button to switch");
  display.display();
}

// VITALS SCREEN
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

// WORKOUT SCREEN
void drawWorkoutScreen(int bpm, bool running) {
  display.clearDisplay();
  
  // Set the header based on the phase
  drawHeader((running && workoutPhase == WORK) ? workouts[currentWorkout] : "COOLDOWN");

  if (running) {
    unsigned long now = millis();
    unsigned long elapsed = now - phaseStartTime;
    unsigned long phaseLength = (workoutPhase == WORK) ? WORK_TIME : COOLDOWN_TIME;
    
    // Check for phase change
    if (elapsed >= phaseLength) {
      phaseStartTime = now;
      workoutPhase = (workoutPhase == WORK) ? COOLDOWN : WORK;
      if (workoutPhase == WORK)
        currentWorkout = (currentWorkout + 1) % 3; // Cycle workouts
      // Re-calculate remaining time after phase switch
      elapsed = 0; 
    }
    
    unsigned long remaining = (phaseLength - elapsed) / 1000;

    int min = remaining / 60;
    int sec = remaining % 60;

    display.setCursor(25, 30);
    display.setTextSize(2);
    display.println("TIME");

    display.setCursor(25, 55);
    // Print time remaining (e.g., 0:05, 4:59)
    display.printf("%d:%02d", min, sec); 

    display.setCursor(35, 85);
    display.setTextSize(1);
    display.println("BPM");

    display.setCursor(40, 100);
    display.setTextSize(2);
    display.println(bpm);

  } else {
    display.setCursor(10, 50);
    display.setTextSize(1);
    display.println("Press workout button to start");
  }

  display.display();
}

// SETUP
void setup() {
  Serial.begin(115200);

  // --- Display Init ---
  display.begin(0x3C, true); // Use 0x3C or 0x3D
  display.setRotation(0);
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.display();

  // --- Pin Init ---
  pinMode(buzzerPin, OUTPUT);
  pinMode(screenButtonPin, INPUT_PULLUP);
  pinMode(workoutButtonPin, INPUT_PULLUP);

  // --- Wi-Fi Init (Warning: This part can block!) ---
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  // Add a timeout for connection
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) { // 30 attempts * 200ms = 6 seconds timeout
    delay(200);
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi!");
    // Configure Time (NTP)
    configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  } else {
    Serial.println("WiFi connection failed.");
  }


  // --- Sensor Init (I2C) ---
  Serial.println("Initializing LPS22 (Pressure/Temp)...");
  if (!lps.begin_I2C()) {
    Serial.println("LPS22 failed! Check wiring.");
    // Do NOT halt the program here (avoid while(1);)
  } else {
    lps.setDataRate(LPS22_RATE_10_HZ);
    Serial.println("LPS22 initialized.");
  }

  Serial.println("Initializing MAX30105 (Heart Rate)...");
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 failed! Check wiring.");
    // Do NOT halt the program here (avoid while(1);)
  } else {
    particleSensor.setup();
    particleSensor.setPulseAmplitudeRed(0x0A);
    particleSensor.setPulseAmplitudeGreen(0);
    Serial.println("MAX30105 initialized.");
  }
}

// LOOP
void loop() {
  unsigned long now = millis();

  // --- BUTTONS ---
  int screenButtonState = digitalRead(screenButtonPin);
  int workoutButtonState = digitalRead(workoutButtonPin);

  // Screen button (Change screen)
  if (screenButtonState == LOW && lastScreenButtonState == HIGH && now - lastScreenPress > debounceDelay) {
    currentScreen = (Screen)((currentScreen + 1) % SCREEN_COUNT);
    lastScreenPress = now;
  }
  lastScreenButtonState = screenButtonState;

  // Workout button (Start/Stop/Next workout)
  if (workoutButtonState == LOW && lastWorkoutButtonState == HIGH && now - lastWorkoutPress > debounceDelay) {
    if (currentScreen == EXERCISE) {
      workoutRunning = !workoutRunning;
      if (workoutRunning) phaseStartTime = now;
    }
    lastWorkoutPress = now;
  }
  lastWorkoutButtonState = workoutButtonState;

  // --- HEART RATE ---
  long irValue = particleSensor.getIR();
  
  // checkForBeat is a function assumed to be in the "heartRate.h" file
  if (irValue > 7000 && checkForBeat(irValue)) {
    long delta = millis() - lastBeat;
    lastBeat = millis();
    beatsPerMinute = 60 / (delta / 1000.0);
    
    // Basic plausibility check
    if (beatsPerMinute > 20 && beatsPerMinute < 255) {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;
      BPM = 0;
      for (byte i = 0; i < RATE_SIZE; i++) BPM += rates[i];
      BPM /= RATE_SIZE;
    }
    tone(buzzerPin, NOTE_C5, 40); // Audible confirmation of beat
  }

  // --- TEMPERATURE & PRESSURE ---
  // Read Temperature (and average it)
  avgTempC = getAveragedTemperature(); 
  
  // Read Pressure
  sensors_event_t temp, pressure;
  lps.getEvent(&pressure, &temp); // Note: Temp reading is also done in getAveragedTemperature

  // --- ALERTS ---
  if (now - lastAlertTime > alertCooldown) {
    if (BPM > 0 && (BPM < BPM_LOW || BPM > BPM_HIGH)) playBPMAlert();
    if (avgTempC > 0 && (avgTempC < TEMP_LOW || avgTempC > TEMP_HIGH)) playTempAlert();
    lastAlertTime = now;
  }

  // --- DRAW SCREENS ---
  // The screen is redrawn every loop to update the sensor values and timers
  switch (currentScreen) {
    case HOME: drawHome(); break;
    case VITALS: drawVitals(avgTempC, pressure.pressure, BPM); break;
    case EXERCISE: drawWorkoutScreen(BPM, workoutRunning); break;
  }

  lastScreen = currentScreen;
}
