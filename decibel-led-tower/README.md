# Decibel Sensor LED Tower

A practice-room loudness monitor: a microphone module measures sound
pressure level (SPL) in dB, and a strip of addressable LEDs lights up
like a tower — green (safe) → yellow (getting loud) → red (hearing
damage range) — so the band can see when to back off without anyone
needing to watch a meter.

## Hardware

- Microcontroller: Arduino Uno/Nano or ESP32 (code below targets either;
  ESP32 recommended if you want Wi-Fi/logging later)
- Sound sensor: **MAX9814** electret mic amplifier module (analog out,
  built-in AGC — disable AGC by leaving the AR pin floating/grounded per
  datasheet for a more linear response) or an **INMP441** I2S mic if you
  want better accuracy (different code path, see note at bottom)
- LED tower: **WS2812B** addressable LED strip, 10 LEDs is a good size
  for a visible tower (one LED per "rung")
- 5V power supply sized for the strip (10x WS2812B ≈ 600mA worst case)
- 470Ω resistor in series with the LED data line, 1000µF cap across the
  LED strip's 5V/GND (standard WS2812B protection)

## Wiring

```
MAX9814 OUT  -> Arduino A0
MAX9814 VCC  -> 5V
MAX9814 GND  -> GND

WS2812B DIN  -> Arduino D6 (through 470Ω resistor)
WS2812B 5V   -> 5V (shared supply, common ground with Arduino)
WS2812B GND  -> GND
```

## Why these thresholds

This isn't a calibrated SPL meter — it's a practical "are we too loud"
indicator. The two thresholds are based on standard hearing-safety
guidance (OSHA/NIOSH):

- **First yellow LED at 85 dB** — OSHA's "action level," the point
  where prolonged exposure starts to matter. This is your early warning.
- **First red LED at 100 dB** — NIOSH's recommended exposure limit at
  100 dB is about 15 minutes/day. This is the "you are doing ear damage
  if this keeps up" line.

The LEDs below 85 dB fill in proportionally as a green gradient, and the
LEDs between 85–100 dB fill in as yellow, so the tower also shows *how
close* you are to each line, not just a single on/off light.

These numbers assume your mic is roughly calibrated (see Calibration
below) — uncalibrated electret modules can be off by 10+ dB, which
matters a lot at these levels, so calibration is not optional if you
actually care about ear safety.

## Calibration (do this once)

Cheap electret mic modules don't report true dB SPL out of the box —
you need to anchor the analog reading to a real measurement:

1. Get a reference reading with a phone SPL meter app (e.g. NIOSH Sound
   Level Meter, which is itself calibrated) or a real SPL meter.
2. Play pink noise or have the band play at a few different volumes,
   noting the phone's dB(A) reading next to the Arduino's raw analog
   value (print it over Serial — see `DEBUG_PRINT_RAW` in the sketch).
3. Fit those points to get `voltage -> dB` constants. The sketch uses a
   simple linear model: `dB = REF_DB + 20 * log10(voltageRMS / REF_VOLTAGE)`.
   Adjust `REF_DB` and `REF_VOLTAGE` in the sketch until your measured
   points line up with the phone meter's readings.
4. Re-check at both your yellow (85 dB) and red (100 dB) thresholds
   specifically, since that's where accuracy matters most.

## Files

- `decibel_led_tower.ino` — main sketch (mic sampling, dB conversion,
  LED tower rendering)

## Using an I2S mic (INMP441) instead

The MAX9814's AGC and analog noise floor make it mediocre for anything
beyond "rough loudness indicator." If you want more accurate SPL,
swap to an I2S MEMS mic (INMP441) and read raw 24-bit samples via the
ESP32's I2S peripheral instead of `analogRead`, computing RMS over a
window the same way. The rest of the pipeline (dB conversion, LED
mapping) stays the same — only `sampleSound()` changes.
