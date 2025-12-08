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
#define PULSE_PIN 0   // will change to 34 on esp-32
int pulseSignal;
int pulseThreshold = 550;

unsigned long lastBeatTime = 0;
int BPM = 0;
bool beatDetected = false;

// Initializing the Speaker 
int buzzerPin = 12;

// Safe Ranges for the Sensors or Speaker will play 
#define BPM_LOW   50      // lowest bpm rate 
#define BPM_HIGH  120     // highest bpm rate

#define TEMP_LOW  35.0    // lowest temp rate 
#define TEMP_HIGH 39.0    // highest temp rate 

unsigned long lastAlertTime = 0;
unsigned long alertCooldown = 5000;  // 5 seconds between alerts
//------------------------------------------------------------------------------


// Screen Layout 
enum Screen { HOME, VITALS, EXERCISE, SCREEN_COUNT };
Screen currentScreen = HOME;

unsigned long lastSwitch = 0;
unsigned long interval = 2500;

// WIFI and Time Setup getting RTC
const char* ssid     = "OLIN-DEVICES"; // Put in wifi name
const char* password = "BestOval4Engineers!"; // Put in wifi password

bool getRTC(struct tm * timeinfo) {
  return getLocalTime(timeinfo);
} // Taking in time data

// Speaker Musical Note Initialization 
#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_G5 784
#define NOTE_A5 880

#define NOTE_C4 262
#define NOTE_E4 330
#define NOTE_G4 392
#define NOTE_C6 1047

// Alert Noise for Irregular Recorded Sensors 
void playBPMAlert() { // Plays sound for irregular bpm rate
  tone(buzzerPin, NOTE_A5, 150);
  delay(200);
  tone(buzzerPin, NOTE_A5, 150);
  delay(200);
  noTone(buzzerPin);
}

void playTempAlert() { // Plays sound for irregular temp recording 
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
// -----------------------------------------------------------------

// Screen Header
void drawHeader(const char* title) {
  display.fillRect(0, 0, 128, 14, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setCursor(2, 3);
  display.setTextSize(1);
  display.println(title);
  display.setTextColor(SH110X_WHITE);
}

// Home Screen 
void drawHome() {
  display.clearDisplay();
  drawHeader("HOME");

  // displaying the time 
  struct tm timeinfo;
  if (getRTC(&timeinfo)) {
    char timeStr[8];
    strftime(timeStr, sizeof(timeStr), "%I:%M %p", &timeinfo);

    if (timeStr[0] == '0') {
      for (int i = 0; i < 7; i++) timeStr[i] = timeStr[i + 1];
    }

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

// Vitals Screen
void drawVitals(float tempC, float pressureHPA, int bpm) {
  display.clearDisplay();
  drawHeader("VITALS");

  // displays the sensor outputs on this screen 
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

// Exercise Screen 
void drawExercise() {
  display.clearDisplay();

  // Preloaded Workouts(will make these into buttons to select from)
  drawHeader("EXERCISE");

  display.setCursor(10, 35);
  display.setTextSize(1);
  display.println("> Push-Ups");

  display.setCursor(10, 55);
  display.println("  Sit-Ups");

  display.setCursor(10, 75);
  display.println("  Jumping Jacks");

  display.display();
}

// Code Setup 
void setup() {
  Serial.begin(115200);

  // start up OLED display
  display.begin(0x3C, true);
  display.setRotation(4);
  display.setTextColor(SH110X_WHITE);
  display.display(); 

  // initialize the speaker pin setup 
  pinMode(buzzerPin, OUTPUT);

  // retrieve the wifi data to get rtc
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(200);

  const long gmtOffset_sec = -5 * 3600;
  configTime(gmtOffset_sec, 0, "pool.ntp.org", "time.nist.gov");

  if (!lps.begin_SPI(LPS_CS, LPS_SCK, LPS_MISO, LPS_MOSI)) {
    while (1);
  }
  lps.setDataRate(LPS22_RATE_10_HZ);
}

// Watch Loop 
void loop() {
  unsigned long now = millis();

  if (now - lastSwitch > interval) {
    currentScreen = (Screen)((currentScreen + 1) % SCREEN_COUNT);
    lastSwitch = now;
  }

  // Retrieving the BPM Rate
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

  // Getting the Temperature and Pressure
  sensors_event_t temp, pressure;
  lps.getEvent(&pressure, &temp);

  // Alert Setting for Speaker 
  if (now - lastAlertTime > alertCooldown) {

    if (BPM < BPM_LOW || BPM > BPM_HIGH) {
      playBPMAlert();
      lastAlertTime = now;
    }

    if (temp.temperature < TEMP_LOW || temp.temperature > TEMP_HIGH) {
      playTempAlert();
      lastAlertTime = now;
    }
  }

  // Draw out the screen
  switch (currentScreen) {
    case HOME:
      drawHome();
      break;

    case VITALS:
      drawVitals(temp.temperature, pressure.pressure, BPM);
      break;

    case EXERCISE:
      drawExercise();
      break;
  }
}
