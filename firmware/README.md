# Firmware setup

Open `Ninajirachi.ino` in Arduino IDE and target `ESP32S3 Dev Module` with `esp32 by Espressif Systems` 2.0.12.

The first face renderer uses Waveshare's direct `ESP32-S3-Touch-LCD-1.28-Test` driver, not LVGL or TFT_eSPI. Copy the driver files listed in `src/waveshare/README.md` from the official demo package before compiling. Those files provide the verified GC9A01A display definition: SPI GPIO 8/9/10/11/12/14 with active-high backlight GPIO 2. The first boot needs only the display driver; touch and QMI8658 remain in the same official package for later tickets.

The driver allocates one 240×240 RGB565 frame buffer (115,200 bytes) with `ps_malloc`; confirm PSRAM on the physical board during bring-up.

## Compass bench setup

`src/compass/Qmc5883p.h` uses Arduino's built-in `Wire` API and has no external driver dependency. It deliberately requires a caller-configured `TwoWire` bus and an address found by an I²C scanner; it does not choose final GPIO pins or assume `0x2C`. Follow [the QMC5883P bench procedure](../docs/compass-bench.md) before connecting it to the wearable interaction loop.

`shared/expression_presets.json` is the source of truth for expression presets. Run this from the repository root after changing it:

```sh
python3 tools/generate_face_presets.py
```

This regenerates the header consumed by Arduino firmware. Run `python3 tools/generate_face_presets.py --check` before committing preset changes.
