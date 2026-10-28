# Firmware setup

Open `firmware.ino` in Arduino IDE and target `ESP32S3 Dev Module` with `esp32 by Espressif Systems` 2.0.12. Select 16 MB Flash and enabled QSPI PSRAM.

The first face renderer uses Waveshare's direct `ESP32-S3-Touch-LCD-1.28-Test` driver, not LVGL or TFT_eSPI. Copy the driver files listed in `src/waveshare/README.md` from the official demo package before compiling. Those files provide the verified GC9A01A display definition: SPI GPIO 8/9/10/11/12/14 with active-high backlight GPIO 2. The first boot needs only the display driver; touch and QMI8658 remain in the same official package for later tickets.

The driver allocates one 240×240 RGB565 frame buffer (115,200 bytes) with `ps_malloc`; confirm PSRAM on the physical board during bring-up.

Download the [official demo ZIP](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.28/ESP32-S3-Touch-LCD-1.28-Demo.zip), then install the required unmodified drivers from the repository root:

```sh
python3 tools/prepare_waveshare.py /path/to/ESP32-S3-Touch-LCD-1.28-Demo.zip
arduino-cli compile --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=enabled firmware
```

Installed vendor files are ignored by Git; the script retains their original copyright/license headers. It also installs the QMI8658 driver used to provide gravity measurements for compass tilt compensation.

Battery voltage uses GPIO1 and the documented 3:1 divider with calibrated ADC millivolts. Charging state is reported as unknown because no verified software-readable charging-status signal has been established. Percentage stays unavailable until a battery-specific calibration is supplied. Confirm readings against a meter before relying on them.

`shared/expression_presets.json` is the source of truth for expression presets. Run this from the repository root after changing it:

```sh
python3 tools/generate_face_presets.py
```

This regenerates the header consumed by Arduino firmware. Run `python3 tools/generate_face_presets.py --check` before committing preset changes.
