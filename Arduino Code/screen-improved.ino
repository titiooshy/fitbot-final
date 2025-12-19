// This code Works:
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

enum Screen {
  HOME,
  VITALS,
  EXERCISE,
  SCREEN_COUNT
};

Screen currentScreen = HOME;
unsigned long lastSwitch = 0;
unsigned long interval = 2500;

// ---------- WIFI + RTC ----------
const char* ssid     = "OLIN-DEVICES";
const char* password = "BestOval4Engineers!";

// Get time as a struct
bool getRTC(struct tm * timeinfo) {
  if (!getLocalTime(timeinfo)) return false;
  return true;
}

// Draw HEADER
void drawHeader(const char* title) {
  display.fillRect(0, 0, 128, 14, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setCursor(2, 3);
  display.setTextSize(1);
  display.println(title);

  display.setCursor(100, 3);
  display.println("~]");
  display.setTextColor(SH110X_WHITE);
}

// HOME SCREEN WITH REAL TIME
void drawHome() {
  display.clearDisplay();
  drawHeader("HOME");

  struct tm timeinfo;
  if (getRTC(&timeinfo)) {

    // Format HH:MM AM/PM for 12-hour clock
    char timeStr[8];  // HH:MM + space + AM/PM + null terminator
    strftime(timeStr, sizeof(timeStr), "%I:%M %p", &timeinfo);

    // Remove leading zero from hour
    if (timeStr[0] == '0') {
      for (int i = 0; i < 7; i++) timeStr[i] = timeStr[i + 1];
      timeStr[7] = '\0';
    }

    display.setCursor(10, 30);
    display.setTextSize(2);
    display.println(timeStr);

    // Format Thu Nov 21
    char dateStr[20];
    strftime(dateStr, sizeof(dateStr), "%a %b %d", &timeinfo);

    display.setCursor(15, 55);
    display.setTextSize(1);
    display.println(dateStr);
  } else {
    display.setCursor(10, 40);
    display.println("No RTC");
  }

  display.drawLine(0, 80, 128, 80, SH110X_WHITE);

  display.setCursor(10, 95);
  display.setTextSize(1);
  display.println("Swipe or wait to change");

  display.display();
}

// VITALS SCREEN
void drawVitals() {
  display.clearDisplay();
  drawHeader("VITALS");

  display.setTextColor(SH110X_WHITE);

  display.setCursor(10, 30);
  display.setTextSize(2);
  display.println("72 bpm");

  display.setCursor(10, 60);
  display.setTextSize(1);
  display.println("Body Temp: 98.6 F");

  display.setCursor(10, 80);
  display.println("Oxygen: 98%");

  display.display();
}

// EXERCISE SCREEN
void drawExercise() {
  display.clearDisplay();
  drawHeader("EXERCISE");

  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);

  display.setCursor(10, 35);
  display.println("> Push-Ups");

  display.setCursor(10, 55);
  display.println("  Sit-Ups");

  display.setCursor(10, 75);
  display.println("  Jumping Jacks");

  display.display();
}

// SETUP
void setup() {
  Serial.begin(115200);

  // OLED
  display.begin(0x3C, true);
  display.clearDisplay();
  display.setRotation(4);
  display.setTextColor(SH110X_WHITE);
  display.display();

  // WIFI
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
  }

  // RTC via NTP
  const long gmtOffset_sec = -5 * 3600;  // -5 hours for EST
  const long daylightOffset_sec = 0;     // no DST
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
  delay(1000);
}

// LOOP
void loop() {
  unsigned long now = millis();

  if (now - lastSwitch > interval) {
    currentScreen = (Screen)((currentScreen + 1) % SCREEN_COUNT);
    lastSwitch = now;
  }

  switch (currentScreen) {
    case HOME:     drawHome(); break;
    case VITALS:   drawVitals(); break;
    case EXERCISE: drawExercise(); break;
  }
}
