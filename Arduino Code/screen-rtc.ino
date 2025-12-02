#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <RTClib.h>

// Display config
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
Adafruit_SH1107 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// RTC
RTC_DS3231 rtc;

// Screen enum
enum Screen {
  HOME,
  VITALS,
  EXERCISE,
  SCREEN_COUNT
};
Screen currentScreen = HOME;

unsigned long lastSwitch = 0;
unsigned long interval = 2500;  // auto-cycle every 2.5s

// --------------------
// Helper for weekday names
// --------------------
const char* dayShortStr(uint8_t day) {
  const char* days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
  return days[day];
}

// Draw HEADER
void drawHeader(const char* title) {
  display.fillRect(0, 0, 128, 14, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setCursor(2, 3);
  display.setTextSize(1);
  display.println(title);

  // Fake icons (battery, wifi)
  display.setCursor(100, 3);
  display.println("~]");
  display.setTextColor(SH110X_WHITE);
}

// HOME SCREEN
void drawHome() {
  display.clearDisplay();
  drawHeader("HOME");

  DateTime now = rtc.now();

  // Time
  display.setCursor(10, 30);
  display.setTextSize(2);
  char timeBuf[6];
  sprintf(timeBuf, "%02d:%02d", now.hour(), now.minute());
  display.println(timeBuf);

  // Date
  display.setCursor(15, 55);
  display.setTextSize(1);
  char dateBuf[12];
  sprintf(dateBuf, "%s %02d", dayShortStr(now.dayOfTheWeek()), now.day());
  display.println(dateBuf);

  // Divider line
  display.drawLine(0, 80, 128, 80, SH110X_WHITE);

  // Footer text
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
  Serial.begin(9600);

  // Display init
  if (!display.begin(0x3C, true)) {  // check your I2C address
    Serial.println("Display not found");
    while (1);
  }
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.display();

  // RTC init
  if (!rtc.begin()) {
    Serial.println("RTC not found");
    while (1);
  }

  // Uncomment ONLY ONCE to set RTC to compile time
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

// LOOP
void loop() {
  unsigned long nowMillis = millis();

  // Auto-cycle screens
  if (nowMillis - lastSwitch > interval) {
    currentScreen = (Screen)((currentScreen + 1) % SCREEN_COUNT);
    lastSwitch = nowMillis;
  }

  // Draw current screen
  switch (currentScreen) {
    case HOME:     drawHome(); break;
    case VITALS:   drawVitals(); break;
    case EXERCISE: drawExercise(); break;
  }

  delay(200);  // small delay to reduce flicker
}
