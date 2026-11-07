# Firmware setup

Open `firmware.ino` in Arduino IDE and target `ESP32S3 Dev Module` with `esp32 by Espressif Systems` 2.0.12. Select 16 MB Flash and enabled QSPI PSRAM.

The first face renderer uses Waveshare's direct `ESP32-S3-Touch-LCD-1.28-Test` driver, not LVGL or TFT_eSPI. Copy the driver files listed in `src/waveshare/README.md` from the official demo package before compiling. Those files provide the verified GC9A01A display definition: SPI GPIO 8/9/10/11/12/14 with active-high backlight GPIO 2. The runtime also reads the touch count for hold gestures and uses the QMI8658 driver for gravity measurements in compass tilt compensation.

The driver allocates one 240×240 RGB565 frame buffer (115,200 bytes) with `ps_malloc`; confirm PSRAM on the physical board during bring-up.

Download the [official demo ZIP](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.28/ESP32-S3-Touch-LCD-1.28-Demo.zip), then install the required unmodified drivers from the repository root:

```sh
python3 tools/prepare_waveshare.py /path/to/ESP32-S3-Touch-LCD-1.28-Demo.zip
arduino-cli compile --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=enabled firmware
```

Installed vendor files are ignored by Git; the script retains their original copyright/license headers. It also installs the QMI8658 driver used to provide gravity measurements for compass tilt compensation.

Battery voltage uses GPIO1 and the documented 3:1 divider with calibrated ADC millivolts. Charging state is reported as unknown because no verified software-readable charging-status signal has been established. Percentage stays unavailable until a battery-specific calibration is supplied. Confirm readings against a meter before relying on them.

## Runtime controls

Hold the face for 5 seconds to open battery/settings. Release, then hold for 3 seconds to return. Seven seconds without touch returns automatically. The loop continues sampling while settings is visible. Serial commands `0`–`4` select idle, curious, happy, surprised, and excited presets for testing.

The external compass starts disabled. At 115200 baud, send `s` to scan I²C. After checking the physical QMC5883P marking, 3.3 V wiring, and sensor-axis alignment, send `v` to enable the detected `0x2C` device. The adapter also verifies chip identity. Send `c` to collect calibration while rotating the assembled wearable through all axes, `w` to save, `x` to cancel, and `r` to toggle raw/heading logs. Calibration is stored per board in NVS and reloaded when enabled after reboot.

The IMU and magnetometer gravity vectors must use the same axes. Confirm the external module mounting before enabling; a rotated mounting needs an explicit axis transform. Invalid or stale headings stay unavailable. See `../docs/compass-bench.md` for the calibration limits and acceptance checklist.

## Compass bench setup

`src/compass/CompassService.h` provides the C++11 Arduino API for verified QMC5883P setup, raw updates, tilt-compensated heading, and calibration stored through ESP32 Preferences. It is disabled by default and requires a caller-configured `TwoWire` bus plus explicit user verification of the chip and scanned address. Main owns serial commands. Follow [the QMC5883P API and bench procedure](../docs/compass-bench.md) before enabling it.

`shared/expression_presets.json` is the source of truth for expression presets. Run this from the repository root after changing it:

```sh
python3 tools/generate_face_presets.py
```

This regenerates the header consumed by Arduino firmware. Run `python3 tools/generate_face_presets.py --check` before committing preset changes.

## BLE discovery

Each wearable advertises and passively scans at the same time; there is no leader or access point. The 48-bit ID is derived from `ESP.getEfuseMac()`, also shown on the settings screen. Manufacturer data uses the experimental `FF FF` company identifier followed by `NINA`, protocol version `01`, discovery type `01`, and the six ID bytes in big-endian order. This prototype identifier is not an assigned Bluetooth company ID and must be replaced before a product release. Other lengths, versions, types, invalid RSSI and the local ID are ignored.

The behavior retains up to four peers; additional new IDs are ignored until a slot expires. A peer expires after 3 seconds without a valid observation. The strongest recent BLE RSSI becomes the target, and a different peer must be at least 6 dB stronger to switch. A selected peer produces curious/focused interaction, changing to excited after 10 seconds of uninterrupted selection. Expiry returns to the manually selected expression (idle by default). All durations and the switching margin are named constants in `WearableBehavior.h` for worn tuning. Discovery continues on the settings screen. The callback queue holds eight observations; excess observations are dropped rather than blocking the radio task.

The BLE protocol is unauthenticated and is for prototype discovery only. Physical identity uniqueness, two-way discovery, radio coexistence, packet loss, and worn timing need the two-wearable checks in issue #15. ESP-NOW direction evidence remains separate work.
