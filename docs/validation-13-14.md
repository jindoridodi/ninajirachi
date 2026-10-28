# Compass and expression/settings verification

Scope: tickets #13 and #14 only. Firmware uses ESP32 Arduino 2.0.12, the `ESP32S3 Dev Module` target, 16 MB flash, QSPI PSRAM, and the official Waveshare drivers installed with `tools/prepare_waveshare.py`.

## Repeatable software checks

From the repository root:

```sh
sh tests/run_face_renderer_test.sh
sh tests/run_waveshare_surface_test.sh
sh tests/run_wearable_behavior_test.sh
sh tests/run_compass_calibration_test.sh
sh tests/run_compass_service_test.sh
sh tests/run_gravity_estimator_test.sh
sh tests/run_movement_behavior_test.sh
python3 tests/preview_editor_test.py
python3 tools/generate_face_presets.py --check
arduino-cli compile --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=enabled firmware
```

Host tests verify behavior and persistence using synthetic input. The compile check uses the actual ESP32 toolchain and official vendor source. Neither establishes physical operation.

## Hardware checks still required

- Record the QMC5883P chip marking, wiring, detected I²C address, and raw readings on the actual board.
- Confirm sensor axes, calibrate in the final enclosure, reboot, and compare headings during rotation. Use the procedure in `compass-bench.md`.
- Confirm five-second entry, release, three-second return, and seven-second inactivity on the physical touchscreen.
- Check all five expression presets and blink/eyelid appearance on the round display.
- Compare displayed voltage with a meter. Charging state remains unknown; percentage remains uncalibrated until supported by actual measurements.

No connected ESP32 was available during this run. These physical results are unverified and the tickets must not be closed solely on the basis of software checks.
