#include <SPI.h>
#include <Wire.h>
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
unsigned long interval = 2500;  // auto-cycle every 2.5s

// Draw HEADER (matches screenshot style)
void drawHeader(const char* title) {
  display.fillRect(0, 0, 128, 14, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setCursor(2, 3);
  display.setTextSize(1);
  display.println(title);

  // fake icons (battery, wifi)
  display.setCursor(100, 3);
  display.println("~]");
  display.setTextColor(SH110X_WHITE);
}

// HOME SCREEN
void drawHome() {
  display.clearDisplay();
  drawHeader("HOME");

  // Time
  display.setCursor(10, 30);
  display.setTextSize(2);
  display.println("12:45");

  // Date
  display.setCursor(15, 55);
  display.setTextSize(1);
  display.println("Thu Nov 21");

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
  display.begin(0x3C, true);
  display.clearDisplay();
  display.setRotation(4);  // matches your sketch orientation
  display.setTextColor(SH110X_WHITE);
  display.display();
}

// LOOP
void loop() {
  unsigned long now = millis();

  // Auto-cycle screens
  if (now - lastSwitch > interval) {
    currentScreen = (Screen)((currentScreen + 1) % SCREEN_COUNT);
    lastSwitch = now;
  }

  // Draw current screen
  switch (currentScreen) {
    case HOME:     drawHome(); break;
    case VITALS:   drawVitals(); break;
    case EXERCISE: drawExercise(); break;
  }
}
