/*
 * Decibel Sensor LED Tower
 *
 * Reads sound level from a MAX9814 mic amp module on an analog pin,
 * converts it to an approximate dB SPL reading, and drives a WS2812B
 * LED strip as a "tower": green -> yellow -> red.
 *
 * First yellow LED  = MIC_YELLOW_DB  (OSHA action level)
 * First red LED     = MIC_RED_DB     (NIOSH 100 dB / 15-min limit)
 *
 * See README.md for wiring and calibration instructions.
 */

#include <FastLED.h>

// ---- Hardware config ----
#define MIC_PIN        A0
#define LED_PIN        6
#define NUM_LEDS       10
#define LED_TYPE       WS2812B
#define COLOR_ORDER    GRB
#define BRIGHTNESS     150

// ---- Calibration (see README "Calibration" section) ----
// dB = REF_DB + 20 * log10(voltageRMS / REF_VOLTAGE)
// Defaults below are placeholders -- you MUST calibrate against a real
// SPL meter before trusting the yellow/red thresholds.
const float REF_VOLTAGE = 0.20;   // volts RMS measured at REF_DB
const float REF_DB      = 70.0;  // known SPL (dB) at REF_VOLTAGE

// ---- Safety thresholds (dB SPL) ----
const float MIC_YELLOW_DB = 85.0;   // OSHA action level
const float MIC_RED_DB    = 100.0;  // NIOSH 100 dB / 15-min exposure limit
const float MIC_MIN_DB    = 50.0;   // bottom of the tower (all green below this)
const float MIC_MAX_DB    = 110.0;  // top of the tower (full red at/above this)

// ---- Sampling ----
const unsigned int SAMPLE_WINDOW_MS = 50; // RMS window, ~20Hz update rate
const float ADC_REF_VOLTAGE = 5.0;        // Arduino Uno/Nano ADC reference
const int   ADC_MAX_VALUE   = 1023;       // 10-bit ADC

#define DEBUG_PRINT_RAW false

CRGB leds[NUM_LEDS];

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
}

// Sample the mic for SAMPLE_WINDOW_MS and return the RMS voltage swing
// around the mic's DC bias.
float sampleSoundRMS() {
  unsigned long startTime = millis();
  long sumSquares = 0;
  long sumValues = 0;
  unsigned int sampleCount = 0;

  while (millis() - startTime < SAMPLE_WINDOW_MS) {
    int raw = analogRead(MIC_PIN);
    sumValues += raw;
    sampleCount++;
    sumSquares += (long)raw * raw;
  }

  float meanValue = (float)sumValues / sampleCount;
  float meanSquare = (float)sumSquares / sampleCount;
  float variance = meanSquare - (meanValue * meanValue);
  if (variance < 0) variance = 0;
  float rmsCounts = sqrt(variance);

  return rmsCounts * (ADC_REF_VOLTAGE / ADC_MAX_VALUE);
}

float voltageToDb(float voltageRMS) {
  if (voltageRMS <= 0.0001) voltageRMS = 0.0001; // avoid log(0)
  return REF_DB + 20.0 * log10(voltageRMS / REF_VOLTAGE);
}

// Map a dB reading onto the LED tower, with green up to MIC_YELLOW_DB,
// yellow between MIC_YELLOW_DB and MIC_RED_DB, red above MIC_RED_DB.
void renderTower(float db) {
  float clamped = constrain(db, MIC_MIN_DB, MIC_MAX_DB);
  float fraction = (clamped - MIC_MIN_DB) / (MIC_MAX_DB - MIC_MIN_DB);
  int litCount = round(fraction * NUM_LEDS);

  float yellowFraction = (MIC_YELLOW_DB - MIC_MIN_DB) / (MIC_MAX_DB - MIC_MIN_DB);
  float redFraction = (MIC_RED_DB - MIC_MIN_DB) / (MIC_MAX_DB - MIC_MIN_DB);
  int yellowStartLed = round(yellowFraction * NUM_LEDS);
  int redStartLed = round(redFraction * NUM_LEDS);

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i >= litCount) {
      leds[i] = CRGB::Black;
    } else if (i >= redStartLed) {
      leds[i] = CRGB::Red;
    } else if (i >= yellowStartLed) {
      leds[i] = CRGB::Yellow;
    } else {
      leds[i] = CRGB::Green;
    }
  }
  FastLED.show();
}

void loop() {
  float voltageRMS = sampleSoundRMS();
  float db = voltageToDb(voltageRMS);

  if (DEBUG_PRINT_RAW) {
    Serial.print("voltageRMS=");
    Serial.print(voltageRMS, 4);
    Serial.print("  dB=");
    Serial.println(db, 1);
  }

  renderTower(db);
}
