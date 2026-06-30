// Decibel light meter firmware
// Board: ESP32 (any dev board with I2S + enough GPIO)
// Mic:   INMP441 I2S MEMS microphone (digital, no analog noise issues)
// LEDs:  WS2812B strip running the length of the tower
//
// Behavior: reads the mic over I2S, computes an RMS-based relative dB level,
// smooths it, and lights a proportional number of LEDs bottom-up with a
// green -> yellow -> red gradient (like a VU meter).
//
// NOTE: this gives RELATIVE dB (good for a reactive light show / loudness
// indicator). True calibrated dB SPL requires a calibrated reference mic
// and a known reference level — see CALIBRATION below if you want that.

#include <driver/i2s.h>
#include <FastLED.h>
#include <math.h>

// ---- pin config ----
#define I2S_WS_PIN   25   // L/R clock
#define I2S_SD_PIN   33   // data in
#define I2S_SCK_PIN  26   // bit clock
#define LED_PIN      27
#define NUM_LEDS     90   // match the physical strip length inside the tower

// ---- audio config ----
#define SAMPLE_RATE     16000
#define SAMPLES_PER_READ 512
static int32_t i2s_buf[SAMPLES_PER_READ];

// ---- calibration ----
// Set REF_DB to the SPL (in dB) you measured with a reference meter at a
// known loudness, and REF_RMS to this firmware's raw RMS reading at that
// same moment. Leave at defaults for a relative-only display.
const float REF_DB  = 70.0f;
const float REF_RMS = 4000.0f;

CRGB leds[NUM_LEDS];
float smoothedDb = 0;

void setupI2S() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = SAMPLES_PER_READ,
    .use_apll = false,
  };
  i2s_pin_config_t pins = {
    .bck_io_num = I2S_SCK_PIN,
    .ws_io_num = I2S_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD_PIN,
  };
  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
}

float readRms() {
  size_t bytesRead = 0;
  i2s_read(I2S_NUM_0, (void*)i2s_buf, sizeof(i2s_buf), &bytesRead, portMAX_DELAY);
  int samples = bytesRead / sizeof(int32_t);

  double sumSq = 0;
  for (int i = 0; i < samples; i++) {
    // INMP441 gives 24-bit data left-justified in 32-bit words
    int32_t sample = i2s_buf[i] >> 11;
    sumSq += (double)sample * (double)sample;
  }
  return sqrt(sumSq / samples);
}

float rmsToDb(float rms) {
  if (rms < 1.0f) rms = 1.0f;
  return REF_DB + 20.0f * log10f(rms / REF_RMS);
}

void setup() {
  Serial.begin(115200);
  setupI2S();
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(180);
}

void loop() {
  float rms = readRms();
  float db = rmsToDb(rms);

  // exponential smoothing so the display doesn't flicker
  const float alpha = 0.25f;
  smoothedDb = smoothedDb * (1 - alpha) + db * alpha;

  // map smoothed dB (assume useful range ~40-100 dB) to LED count
  const float DB_MIN = 40.0f, DB_MAX = 100.0f;
  float t = (smoothedDb - DB_MIN) / (DB_MAX - DB_MIN);
  t = constrain(t, 0.0f, 1.0f);
  int litCount = round(t * NUM_LEDS);

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i >= litCount) {
      leds[i] = CRGB::Black;
      continue;
    }
    float pos = (float)i / (NUM_LEDS - 1); // 0 = bottom, 1 = top
    if (pos < 0.6f) leds[i] = CRGB::Green;
    else if (pos < 0.85f) leds[i] = CRGB::Yellow;
    else leds[i] = CRGB::Red;
  }
  FastLED.show();

  Serial.printf("RMS=%.1f  dB=%.1f\n", rms, smoothedDb);
  delay(20);
}
