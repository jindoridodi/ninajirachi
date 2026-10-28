# Waveshare ESP32-S3-Touch-LCD-1.28 Notes

Primary sources: [board documentation](https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.28), [Arduino guide](https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.28/Arduino), [official example package](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.28/ESP32-S3-Touch-LCD-1.28-Demo.zip), and [schematic](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.28/ESP32-S3-Touch-LCD-1.28-Sch.pdf).

## First firmware baseline

Use Waveshare's `ESP32-S3-Touch-LCD-1.28-Test` example as the board baseline. It supplies direct GC9A01A display drawing, CST816S touch, QMI8658 IMU, and primitive drawing APIs. This is the intended first route for the parameterized face; no LVGL dependency is required for initial bring-up.

Waveshare documents `esp32 by Espressif Systems` 2.0.12 for the initial Arduino baseline. Use the project-required `ESP32S3 Dev Module` target and USB-C for flashing/logging.

## Confirmed board interfaces

| Function | GPIO |
| --- | ---: |
| LCD backlight | 2 |
| LCD D/C, CS, SCLK, MOSI, MISO, reset | 8, 9, 10, 11, 12, 14 |
| I²C SDA, SCL | 6, 7 |
| Touch reset, interrupt | 13, 5 |
| Battery ADC | 1 |
| QMI8658 interrupts | 4, 3 |

The display is a 240×240 GC9A01A SPI panel. Touch is CST816S over I²C. Touch and the QMI8658 share the GPIO6/GPIO7 I²C bus. The QMC5883P will also use that bus after an I²C scan verifies every device.

## Initialization guidance

1. Start serial at 115200.
2. Initialize the CST816S with the documented GPIO6/GPIO7/13/5 wiring.
3. Initialize the Waveshare display module and allocate its 240×240 RGB565 frame buffer in PSRAM when available.
4. Initialize and probe QMI8658. The official driver tries `0x6A`, then `0x6B`, and checks `WHO_AM_I == 0x05`.
5. Render the parameterized face from a named expression preset.

The published schematic labels the IMU as QMI8658A while Waveshare links QMI8658C documentation. Preserve the official probe-based driver for initial bring-up and log the detected device identity; do not substitute a generic driver before bench verification.

## Bench checks

- Upload through USB-C and confirm a visible 240×240 idle face.
- Confirm at least one touch coordinate/event.
- Confirm changing QMI8658 motion samples.
- Run an I²C scan before attaching/using the external QMC5883P.
- Confirm PSRAM before relying on the 115,200-byte RGB565 frame buffer.
