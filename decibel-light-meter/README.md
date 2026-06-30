# Decibel Light Meter

A ~1m tall floor tower that lights up an internal LED strip proportional to
ambient sound level, like a giant VU meter. Weighted base keeps it from
tipping.

## Parts

- `model.scad` — parametric OpenSCAD model. Set `PART` at the top to one of
  `"base"`, `"segment"`, `"cap"`, `"diffuser"`, then render and export STL.
  - **base**: weighted ballast cavity (fill with sand/lead shot after
    printing), an electronics bay, mic sound port, USB-C access slot.
  - **segment**: one ~220mm tower section. Print 5 of them to reach 1000mm
    (adjust `SEGMENT_H` if your D2X bed allows taller single-piece prints).
    Segments stack via a lap joint and are spined by an M8 threaded rod
    through the center for rigidity.
  - **cap**: top cap, doubles as the rod's top anchor nut pocket.
  - **diffuser**: thin frosted strip that clips into each segment's window
    cutout to diffuse the LEDs. Print in white/translucent PETG, 0 infill,
    0.6mm walls — skip vase mode here, you need both walls aligned with the
    window.

## Print settings

- Tower segments / base / cap: PETG or ASA for stiffness (PLA will do for a
  prototype). 4+ wall perimeters since `WALL = 2.4mm` assumes that.
- Diffuser strips: white/translucent PETG, low infill, thin walls.
- No supports needed — everything is designed to print base-down with
  vertical walls.

## Assembly

1. Print base, 5 segments, cap, and 5 diffuser strips.
2. Fill the base's ballast cavity with sand or lead shot, then it's
   permanently capped by the first segment seated on top (use a few drops
   of glue at the lap joint if you want it sealed).
3. Mount the ESP32, mic, and LED strip driver in the base's electronics bay.
   Run the LED strip data/power wires up through the segments alongside the
   center threaded rod.
4. Clip a diffuser strip into each segment's window before stacking.
5. Thread the M8 rod from the base, through all 5 segments, and secure with
   a nut in the cap's pocket — this is what keeps a 1m PLA/PETG tower from
   flexing or twisting.
6. Route the LED strip up the inside, one run spanning all segments, taped
   or clipped to the inner wall behind the diffuser windows.

## Electronics (default assumption — swap freely)

- ESP32 dev board
- INMP441 I2S MEMS microphone (digital, no analog noise floor issues)
- WS2812B addressable LED strip, length matched to `NUM_LEDS` in firmware
  (~90 LEDs/segment count for a 1m tower at 60 LED/m density — adjust to
  your strip)
- 5V supply sized for the LED strip's max current draw (count × ~60mA at
  full white; size down since the meter is rarely all-white)

## Firmware

`firmware/decibel_light_meter.ino` — reads the mic over I2S, computes a
smoothed relative dB level, and lights LEDs bottom-up green → yellow → red.

This gives *relative* loudness out of the box. For real calibrated dB SPL,
measure a known sound level with a reference meter and set `REF_DB` /
`REF_RMS` in the firmware to that reading (see the `CALIBRATION` comment in
the file).
