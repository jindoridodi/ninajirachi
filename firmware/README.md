# Firmware setup

Open `firmware.ino` in Arduino IDE and target `ESP32S3 Dev Module` with `esp32 by Espressif Systems` 2.0.12. Select 16 MB Flash and enabled QSPI PSRAM.

The first face renderer uses Waveshare's direct `ESP32-S3-Touch-LCD-1.28-Test` driver, not LVGL or TFT_eSPI. Copy the driver files listed in `src/waveshare/README.md` from the official demo package before compiling. Those files provide the verified GC9A01A display definition: SPI GPIO 8/9/10/11/12/14 with active-high backlight GPIO 2. The runtime also reads the touch count for hold gestures and uses the QMI8658 driver for gravity measurements in compass tilt compensation.

## LVGL face authoring

`src/face/LvglFace.h` renders the reference icon in LVGL v8: a wide rounded triangle with a flat top, short vertical rounded-rectangle eyes, and a continuous double-U mouth with rounded ends. The reference aspect ratio is preserved inside the 240x240 screen.

```cpp
#include "src/face/LvglFace.h"

nina::LvglFace face;
face.create(lv_scr_act(), nina::idlePreset());
// Later, from the animation loop:
face.render(nina::expressionPreset(nina::ExpressionName::Happy));
```

The Arduino sketch's `FaceRenderer` draws this same face through the verified direct Waveshare path; the placeholder renderer has been removed. `LvglFace` is available for LVGL display integrations. `shared/face_geometry.json` contains the outline and mouth cubic control points in reference-image coordinates, eye rectangles, corner radius, and stroke widths. Both firmware renderers and the Tk preview use this geometry; after editing it, run `python3 tools/face_geometry.py` to regenerate `src/face/FaceGeometry.generated.h` (`--check` verifies it). LVGL draws the sampled curves with rounded line segments. Objects and point buffers persist across expression updates.

Run `python3 preview/face_preview.py` to tune eye width/openness, `eyeOffsetX/Y`, mouth width/depth, and colors. Gaze shifts the solid eyes. There is no decorative dot above the face; the jump background cycles through a fixed palette. Export writes only the current expression parameters. The reference idle values are eyeWidth=36, eyeOpen=42, mouthWidth=42, mouthHeight=14; these scale the traced geometry. Re-export saved presets made with the previous model before using them with this version.

For a headless visual check, `python3 tools/render_face_preview.py /tmp/face.png` draws the same preview to PNG (requires Pillow). Use `--preset happy` to inspect another preset. Run `python3 tests/face_geometry_test.py` to check the reference proportions, valid canvas options, and generated geometry.

The driver allocates one 240×240 RGB565 frame buffer (115,200 bytes) with `ps_malloc`; confirm PSRAM on the physical board during bring-up.

Movement uses the driver's configured ±8 g and ±512°/s IMU scales. Tilt follows a settled gravity estimate and holds its last value while movement prevents a reliable estimate. A low-acceleration phase followed by a vertical impact triggers a jump overlay; tune the thresholds on the assembled wearable in #15.

Download the [official demo ZIP](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.28/ESP32-S3-Touch-LCD-1.28-Demo.zip), then install the required unmodified drivers from the repository root:

```sh
python3 tools/prepare_waveshare.py /path/to/ESP32-S3-Touch-LCD-1.28-Demo.zip
arduino-cli compile --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=enabled firmware
```

Installed vendor files are ignored by Git; the script retains their original copyright/license headers. It also installs the QMI8658 driver used to provide gravity measurements for compass tilt compensation.

Battery voltage uses GPIO1 and the documented 3:1 divider with calibrated ADC millivolts. Charging state remains unknown: the [board documentation](https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.28) documents battery voltage on GPIO1 but does not identify a software-readable charger status GPIO; the [schematic](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.28/ESP32-S3-Touch-LCD-1.28-Sch.pdf) labels the charger STAT pin but does not establish a documented firmware input. Confirm the actual board revision and wiring before adding charge detection. Percentage stays unavailable until a battery-specific calibration is supplied. Confirm readings against a meter before relying on them.

## Runtime controls

Hold the face for 5 seconds to open battery/settings. Release, then hold for 3 seconds to return. Seven seconds without touch returns automatically. The loop continues sampling while settings is visible. Serial commands `0`–`4` select idle, curious, happy, surprised, and excited presets for testing.

The external compass starts disabled until verified. The compass uses a separate I²C controller on GPIO15 (SDA) and GPIO16 (SCL), while the onboard touch controller and IMU remain on GPIO6/GPIO7. At 115200 baud, send `s` to scan the compass bus. After checking the physical QMC5883P marking, 3.3 V wiring, and sensor-axis alignment, send `v` to enable the detected `0x2C` device. The adapter also verifies chip identity. Successful verification is saved in NVS; on later boots the firmware probes the address and checks the chip again before enabling it. Send `u` to forget verification and calibration before replacing or remounting the sensor, then physically inspect, scan, and verify again. If storage reports a failure, do not replace the sensor until forgetting verification succeeds. Send `c` to collect calibration while rotating the assembled wearable through all axes, `w` to save, `x` to cancel, and `r` to toggle raw/heading logs. Calibration is stored per board in NVS. Recalibrate after changing the sensor, mounting, or enclosure.

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

The BLE protocol is unauthenticated and is for prototype discovery only. Physical identity uniqueness, two-way discovery, radio coexistence, packet loss, and worn timing need the two-wearable checks in issue #15.

Observations carry their callback receive time through the queue, so delayed or older packets cannot extend a peer's lifetime or replace newer signal evidence. Each drained batch is considered together before peer selection, preserving continuity when a later queued observation bridges the grace period. Advertising configuration and start complete asynchronously: immediate request failures and completion failures are printed to serial, while `begin()` reports queue, configuration-request, and scan-start failures. A successful return does not guarantee advertising has started; missing completion events and actual over-the-air operation require hardware verification.
# Approximate gaze radio

BLE remains authoritative for the current nearby peer. ESP-NOW sends a 12-byte `NINA`, version 1, type 2 beacon containing the same six-byte big-endian eFuse identity advertised over BLE. Both wearables use Wi-Fi station mode on channel 1, with no access point. Broadcasts are unauthenticated and must not be treated as proof of identity.

ESP32 Arduino 2.0.12 / ESP-IDF 4.4 does not expose packet RSSI in the ESP-NOW callback. The Wi-Fi promiscuous callback reads RSSI and the identity from the same validated, unencrypted ESP-NOW v1 broadcast frame. The callback copies observations into a bounded queue; samples more than 250 ms old are discarded rather than paired with a much newer compass heading. Radio/compass timing, frame layout, and BLE coexistence require physical verification in #15. Wi-Fi sniffer reception alongside BLE scanning can be unstable; dropped observations must lead to centered eyes.

Gaze tuning lives in `src/behavior/DirectionEvidence.h`: eight 45-degree heading buckets, at least two readings in each of three buckets, a maximum 15 dB bucket spread, minimum -85 dBm signal, minimum 6 dB contrast, 5-second bucket lifetime, and a 2-second fallback after the last confident turn. A target change or expiry clears evidence. These are initial thresholds for worn testing, not measured guarantees.
