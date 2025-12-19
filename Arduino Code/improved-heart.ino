// INCLUDED LIBRARIES (Adjusted)
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_Sensor.h>
// #include "MAX30105.h" // REMOVED: No longer needed
// #include "heartRate.h" // ASSUMED: Will use integrated logic

// --- PULSE SENSOR AMPED SETUP ---
int PulseSensorPIN = A0; // <<< IMPORTANT: Connect the Pulse Sensor to Analog Pin A0
volatile int Signal;     // Holds the raw analog signal value
volatile int Threshold = 550; // Manual threshold for beat detection (adjust as needed)

// Pulse Processing Variables (Integrated logic from PulseSensor library)
volatile int T = 250;     // Time interval between beats in mS (default is 250)
volatile int P = 512;     // Peak signal level
volatile int I = 512;     // Trough signal level
volatile boolean Pulse = false;  // True when pulse is found
volatile boolean QS = false;     // True when we have a new beat

// OLED SETUP (No Change)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// MAX30102 HEART RATE (Modified to use new sensor data)
// MAX30105 particleSensor; // REMOVED

const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int BPM = 0;

// BUZZER (No Change)
int buzzerPin = 7;

// --- BITMAPS (No Change) ---
static const unsigned char PROGMEM logo2_bmp[] =
{ 0x03, 0xC0, 0xF0, 0x06, 0x71, 0x8C, 0x0C, 0x1B, 0x06, 0x18, 0x0E, 0x02, 0x10, 0x0C, 0x03, 0x10, //Logo2 and Logo3 are two bmp pictures that display on the OLED if called
0x04, 0x01, 0x10, 0x04, 0x01, 0x10, 0x40, 0x01, 0x10, 0x40, 0x01, 0x10, 0xC0, 0x03, 0x08, 0x88,
0x02, 0x08, 0xB8, 0x04, 0xFF, 0x37, 0x08, 0x01, 0x30, 0x18, 0x01, 0x90, 0x30, 0x00, 0xC0, 0x60,
0x00, 0x60, 0xC0, 0x00, 0x31, 0x80, 0x00, 0x1B, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x04, 0x00, };

static const unsigned char PROGMEM logo3_bmp[] =
{ 0x01, 0xF0, 0x0F, 0x80, 0x06, 0x1C, 0x38, 0x60, 0x18, 0x06, 0x60, 0x18, 0x10, 0x01, 0x80, 0x08,
0x20, 0x01, 0x80, 0x04, 0x40, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x02, 0xC0, 0x00, 0x08, 0x03,
0x80, 0x00, 0x08, 0x01, 0x80, 0x00, 0x18, 0x01, 0x80, 0x00, 0x1C, 0x01, 0x80, 0x00, 0x14, 0x00,
0x80, 0x00, 0x14, 0x00, 0x80, 0x00, 0x14, 0x00, 0x40, 0x10, 0x12, 0x00, 0x40, 0x10, 0x12, 0x00,
0x7E, 0x1F, 0x23, 0xFE, 0x03, 0x31, 0xA0, 0x04, 0x01, 0xA0, 0xA0, 0x0C, 0x00, 0xA0, 0xA0, 0x08,
0x00, 0x60, 0xE0, 0x10, 0x00, 0x20, 0x60, 0x20, 0x06, 0x00, 0x40, 0x60, 0x03, 0x00, 0x40, 0xC0,
0x01, 0x80, 0x01, 0x80, 0x00, 0xC0, 0x03, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0x30, 0x0C, 0x00,
0x00, 0x08, 0x10, 0x00, 0x00, 0x06, 0x60, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x01, 0x80, 0x00 };

// SAFE RANGES (No Change)
#define BPM_LOW 50
#define BPM_HIGH 120

unsigned long lastAlertTime = 0;
unsigned long alertCooldown = 5000;

// WORKOUT SYSTEM (No Change)
enum WorkoutPhase { WORK, COOLDOWN };
WorkoutPhase workoutPhase = WORK;

const char* workouts[] = {"PUSH-UPS", "SIT-UPS", "JUMP JACKS"};
int currentWorkout = 0;

const unsigned long WORK_TIME = 300000; // 5 min
const unsigned long COOLDOWN_TIME = 300000; // 5 min

unsigned long phaseStartTime = 0;
bool workoutRunning = false;

// SCREENS (No Change)
enum Screen { HOME, VITALS, EXERCISE, SCREEN_COUNT };
Screen currentScreen = HOME;
Screen lastScreen = HOME;

// TIMER LOGIC FOR PANNING MODE (No Change)
unsigned long lastScreenSwitchTime = 0;
const unsigned long SCREEN_DISPLAY_DURATION = 5000; // Display each screen for 5 seconds

// WIFI + TIME (No Change)
const char* ssid = "OLIN-DEVICES";
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

// HEADER (No Change)
void drawHeader(const char* title) {
display.fillRect(0, 0, 128, 14, SH110X_WHITE);
display.setTextColor(SH110X_BLACK);
display.setCursor(2, 3);
display.setTextSize(1);
display.println(title);
display.setTextColor(SH110X_WHITE);
}

// HOME SCREEN (No Change)
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

display.display();
}

// VITALS SCREEN (No Change)
void drawVitals(int bpm) { 
display.clearDisplay();
drawHeader("VITALS");

display.setTextSize(2);
display.setCursor(25, 30);
display.println("Heart Rate");

display.setTextSize(4);
display.setCursor(25, 60);
display.print(bpm);
display.setTextSize(1);
display.setCursor(85, 90);
display.println("BPM");

display.display();
}

// WORKOUT SCREEN (No Change)
void drawWorkoutScreen(int bpm, bool running) {
display.clearDisplay();

if (!running) {
workoutRunning = true;
phaseStartTime = millis();
workoutPhase = WORK; 
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


// --- CORE PULSE SENSOR AMPED LOGIC (Integrated) ---
// This function mimics the simplified Pulse Sensor beat detection
boolean checkForBeat_Analog(int sensorValue) { 
static long lastTime = millis();
long now = millis();
int V = sensorValue;

// 1. Automatic Gain Control (Adjust P and I based on signal amplitude)
if (V < Threshold && V < I) I = V; // Capture the lowest trough
if (V > Threshold && V > P) P = V; // Capture the highest peak

// 2. Rising edge: Signal passes the threshold (Threshold is halfway between P and I)
if (V > Threshold && Pulse == false && now > lastTime + T) { 
Pulse = true; // Set flag
tone(buzzerPin, NOTE_C5, 40); // Sound a click on beat
lastTime = now;
return true; // A Beat IS detected
}

// 3. Falling edge: Signal falls below the threshold, resetting for the next beat
if (V < Threshold && Pulse == true) { 
Pulse = false; // Reset flag for next pulse
T = now - lastTime; // Store the time between beats (used for smooth thresholding)

// 4. Update Dynamic Threshold
Threshold = I + (P - I) / 2; // Set threshold to the middle of the signal range
// Reset P and I for the next beat detection
P = Threshold; 
I = Threshold; 
}

return false; // No Beat detected
}

// SETUP (Modified: Removed MAX30105 setup)
void setup() {
Serial.begin(115200);

display.begin(0x3C, true);
display.setRotation(0);
display.clearDisplay();
display.setTextColor(SH110X_WHITE);
display.display();

pinMode(buzzerPin, OUTPUT);
  pinMode(PulseSensorPIN, INPUT); // Initialize the analog pin

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

// MAX30105 setup removed:
// if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) Serial.println("MAX30105 failed! (Continuing program)");
// particleSensor.setup();
// particleSensor.setPulseAmplitudeRed(0x0A);
// particleSensor.setPulseAmplitudeGreen(0);

lastScreenSwitchTime = millis(); // Initialize the screen timer
}

// LOOP (MODIFIED: Analog sensor reading and logic replacement)
void loop() {
unsigned long now = millis();

// --- AUTOMATIC SCREEN SWITCHING ---
if (now - lastScreenSwitchTime > SCREEN_DISPLAY_DURATION) {
currentScreen = (Screen)((currentScreen + 1) % SCREEN_COUNT);
lastScreenSwitchTime = now;
}

// --- HEART RATE (Analog Pulse Sensor) --- 
Signal = analogRead(PulseSensorPIN); // Read the raw analog value

if (checkForBeat_Analog(Signal)) { // Check for a beat using the analog logic
long delta = millis() - lastBeat;
lastBeat = millis();

// Recalculate BPM based on the time delta
beatsPerMinute = 60 / (delta / 1000.0);

// Update the running average
if (beatsPerMinute > 20 && beatsPerMinute < 255) {
 rates[rateSpot++] = (byte)beatsPerMinute;
 rateSpot %= RATE_SIZE;
 BPM = 0;
 for (byte i = 0; i < RATE_SIZE; i++) BPM += rates[i];
 BPM /= RATE_SIZE;
}
// The buzzer tone is now handled inside checkForBeat_Analog for better timing
// tone(buzzerPin, NOTE_C5, 40); // Original MAX30105 tone removed here
}

// --- ALERTS --- 
if (now - lastAlertTime > alertCooldown) {
if (BPM > 0 && (BPM < BPM_LOW || BPM > BPM_HIGH)) playBPMAlert();
lastAlertTime = now;
}

// --- DRAW SCREENS --- 
switch (currentScreen) {
case HOME: drawHome(); break;
case VITALS: drawVitals(BPM); break; 
case EXERCISE: drawWorkoutScreen(BPM, workoutRunning); break;
}

lastScreen = currentScreen;
}
