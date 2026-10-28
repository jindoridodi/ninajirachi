# QMC5883P bench procedure

The compass adapter only accepts an I²C bus and address supplied by its caller. It does not select GPIOs, call `Wire.begin`, or assume a breakout address. This is deliberate: generic GY-271 boards are not a trustworthy chip identity.

## Before firmware configuration

1. Inspect and record the breakout's printed chip marking and `VCC`, `GND`, `SDA`, and `SCL` labels.
2. Power the confirmed QMC5883P breakout from the 3.3 V path and connect ground.
3. Choose physically verified I²C pins. The Waveshare board documentation says its onboard I²C bus uses GPIO 6 (SDA) and GPIO 7 (SCL) for the IMU and touch controller; do not assume those signals are exposed at the intended enclosure connection point.
4. Configure the chosen `TwoWire` bus in the board setup, then run an I²C scan. Record the responding address before constructing `Qmc5883p`.
5. Call `Qmc5883p::begin(bus, scannedAddress)`. It verifies the QMC5883P chip ID (`0x80`) before enabling normal measurement mode. A scan response with a different ID is not a QMC5883P configuration success.

Sources: [Waveshare board documentation](https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.28) and the [QST QMC5883P datasheet](https://www.qstcorp.com/upload/pdf/202512/13-52-19%20QMC5883P%20Datasheet%20Rev.E.pdf).

## Calibration and heading

With the sensor mounted in its final enclosure, read and log raw X/Y/Z values while slowly rotating the complete wearable through every orientation. The test is successful only when the values are nonzero, vary as it moves, and do not report overflow.

Feed those samples to `CompassCalibration::observe`. Persist the returned `CompassCalibrationData` for that physical wearable after the observed ranges cover all three axes. Set its heading offset by placing the display in the agreed forward direction and comparing the resulting heading with the chosen reference direction. Recalibrate after changing the enclosure, battery position, attachment hardware, sensor orientation, or nearby wiring.

`CompassCalibration` removes the per-axis magnetic center. It has no dependency on expression presets, rendering, peers, or gaze. Its public `headingDegrees` result is therefore the only compass value that later approximate-gaze work should consume.

The first calibration is axis-aligned min/max compensation. If the worn test shows heading distortion that varies by orientation, replace it with a documented ellipsoid calibration before relying on it for gaze.
