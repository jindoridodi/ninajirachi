# QMC5883P bench procedure

The compass adapter only accepts an I²C bus and address supplied by its caller. It does not select GPIOs, call `Wire.begin`, or assume a breakout address. This is deliberate: generic GY-271 boards are not a trustworthy chip identity.

## Before firmware configuration

1. Inspect and record the breakout's printed chip marking and `VCC`, `GND`, `SDA`, and `SCL` labels.
2. Power the confirmed QMC5883P breakout from the 3.3 V path and connect ground.
3. Choose physically verified I²C pins. The Waveshare board documentation says its onboard I²C bus uses GPIO 6 (SDA) and GPIO 7 (SCL) for the IMU and touch controller; do not assume those signals are exposed at the intended enclosure connection point.
4. Configure the chosen `TwoWire` bus in the board setup, then run an I²C scan. Record the responding address before constructing `Qmc5883p`.
5. `CompassService::begin(bus)` is disabled by default and performs no I²C transactions. Only after physical inspection and the scan, call `begin(bus, CompassConfig{true, scannedAddress})`. It checks chip ID `0x80`, writes the QST Rev E section 7.1 normal-mode setup (axis signs `0x29=0x06`, set/reset on and ±8 G `0x0B=0x08`, normal 200 Hz `0x0A=0xCD`), and verifies each write by reading it back. A responding address alone is insufficient.

Sources: [Waveshare board documentation](https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.28) and the [QST QMC5883P datasheet](https://www.qstcorp.com/upload/pdf/202512/13-52-19%20QMC5883P%20Datasheet%20Rev.E.pdf).

## Calibration and heading

With the sensor mounted in its final enclosure, read and log raw X/Y/Z values while slowly rotating the complete wearable through every orientation. The test is successful only when the values are nonzero, vary as it moves, and do not report overflow.

Call `startCalibration()`, then continue calling `update`. Slowly rotate through all orientations, not just a flat circle. `saveCalibration(offsetDegrees)` requires at least 120 accepted samples and a raw span of at least 200 counts on every axis. These named constants are sanity gates, not proof of adequate orientation coverage. Invalid/all-zero/saturated/overflow samples are excluded. Calibration remains invalid while collecting. Saving writes one versioned record to ESP32 Preferences namespace `nina-compass`, key `cal-v1`; only a successful write replaces active calibration. `cancelCalibration()` preserves the previous calibration. Reboot and check that calibration restores. Storage failure leaves raw inspection available but cannot manufacture a calibrated heading.

Calibration removes the per-axis magnetic center and normalizes each half-span, providing diagonal soft-iron compensation. Set heading offset by comparing the mounted wearable's forward axis with a reference direction. Recalibrate after changing enclosure, battery, attachment, wiring, sensor axes, or driver range/sign configuration.

The min/max approach cannot remove cross-axis distortion. If worn testing shows orientation-dependent heading error, use a documented ellipsoid fit before relying on it for gaze.

## Runtime API (Arduino C++11)

Include `src/compass/CompassService.h` and keep one `nina::CompassService` instance. The service uses Arduino `TwoWire` and ESP32 `Preferences`, with no external driver dependency. It never calls `Wire.begin` or selects pins.

- `bool begin(TwoWire&, CompassConfig = {})`: disabled by default; `CompassConfig{true, scannedAddress}` is the explicit user-verified enable gate. Returns sensor configuration success; raw mode can succeed without saved calibration or working storage.
- `bool update(uint32_t nowMs, MagneticVector gravityInG)`: poll for a new accepted XYZ sample. Returns raw-read success, independently of heading validity.
- `CompassHeading heading(uint32_t nowMs)`: `{valid, degrees, sampledAtMs}`. Consume degrees only when valid. Heading expires after 250 ms, including across `millis()` wraparound, and becomes invalid immediately on a failed read or during calibration.
- `MagneticSample raw()`: last accepted signed XYZ; log only after `update` returns true to avoid presenting old values as fresh.
- `bool startCalibration()`, `bool saveCalibration(float headingOffsetDegrees = 0)`, `void cancelCalibration()`.
- `bool calibrated()`, `bool collecting()`, `uint32_t calibrationSamples()`: inspect calibration status. `calibrated()` reports an installed calibration; heading is still invalid during collection.

Gravity must be a current, filtered gravity/down estimate in **g**, transformed to the same signed sensor axes as the magnetometer: +X forward, +Y right, +Z down. Level input is `{0,0,1}`. Convert QMI mg to g by dividing by 1000, then verify axis mapping/signs against the mounted sensors. Tilt compensation projects the calibrated field onto the horizontal plane. It rejects nonfinite or near-zero inputs, gravity magnitude outside 0.8–1.2 g, near-vertical forward orientation, and degenerate horizontal field. Fast movement can still corrupt a nominally 1 g measurement; the caller must supply a trustworthy filtered gravity estimate or an invalid vector (e.g. zero) when unavailable. The service cannot infer IMU freshness from a vector alone.

Main owns serial parsing: `s` scans, `v` explicitly enables the verified module, `c` starts collection, `w` saves, `x` cancels, `r` toggles raw logging. Those commands call this API; the compass service does not read Serial itself.

## Required hardware checks and software evidence

Host tests: `sh tests/run_compass_calibration_test.sh` and `sh tests/run_compass_service_test.sh`. Both compile as C++11. The service test substitutes only I²C and Preferences and covers disabled configuration, setup readback, signed raw data, rejected reads, capture/save/reboot, storage failure, invalid records, stale headings, and timer wraparound. Geometry tests include a known tilted heading and invalid gravity.

Still required on each physical wearable: verify markings/address/pins and 3.3 V; inspect changing nonzero raw readings without overflow; collect full-orientation calibration in the final enclosure; save/reboot/restore; verify forward/right heading and sign conventions while pitching and rolling; check battery/radio/display-current interference and heading freshness while worn. No physical tests have been performed. Main performs the integrated ESP32 core 2.0.12 compile; host adapters do not replace that check.

Register provenance: [QST Rev E sections 7.1, 7.5 and 9](https://www.qstcorp.com/upload/pdf/202512/13-52-19%20QMC5883P%20Datasheet%20Rev.E.pdf). Chip identity, data byte order, status bits and range encoding were cross-checked against [Adafruit's driver](https://github.com/adafruit/Adafruit_QMC5883P/blob/main/Adafruit_QMC5883P.cpp) and [register definitions](https://github.com/adafruit/Adafruit_QMC5883P/blob/main/Adafruit_QMC5883P.h). Heading compensation and calibration are project math, not sensor register behavior.
