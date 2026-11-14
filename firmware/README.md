# Firmware setup

Open `firmware.ino` in Arduino IDE and target `ESP32S3 Dev Module` with `esp32 by Espressif Systems` 2.0.12. Select 16 MB Flash, QSPI PSRAM, and partition scheme `16M Flash (3MB APP/9.9MB FATFS)`.

The first face renderer uses Waveshare's direct `ESP32-S3-Touch-LCD-1.28-Test` driver, not LVGL or TFT_eSPI. Copy the driver files listed in `src/waveshare/README.md` from the official demo package before compiling. Those files provide the verified GC9A01A display definition: SPI GPIO 8/9/10/11/12/14 with active-high backlight GPIO 2. The runtime also reads the touch count for tap gestures and uses the QMI8658 driver for gravity measurements in compass tilt compensation.

## Face authoring

The Arduino sketch's `FaceRenderer` draws the reference icon through the verified direct Waveshare path: a wide rounded triangle with a flat top, short vertical rounded-rectangle eyes, and a continuous double-U mouth with rounded ends. The reference aspect ratio is preserved inside the 240×240 screen. `shared/face_geometry.json` contains the outline and mouth cubic control points in reference-image coordinates, eye rectangles, corner radius, and stroke widths. The firmware renderer and Tk preview use this geometry; after editing it, run `python3 tools/face_geometry.py` to regenerate `src/face/FaceGeometry.generated.h` (`--check` verifies it).

Run `python3 preview/face_preview.py` to tune eye width/openness, `eyeOffsetX/Y`, mouth width/depth, tongue extension (`tongueOut`, 0–20), and colors. Gaze shifts the solid eyes. There is no decorative dot above the face; the jump background flashes through bright colors every 100 ms, changing the starting color and order on each jump. The jump face opens smoothly even during a blink, with a gentler eye shape and a short, wide black tongue that grows out over 400 ms over the unchanged mouth; the face and background ease back to the current expression over the final quarter of the overlay. Export writes only the current expression parameters. The reference idle values are eyeWidth=36, eyeOpen=42, mouthWidth=42, mouthHeight=14; these scale the traced geometry. Re-export saved presets made with the previous model before using them with this version.

For a headless visual check, `python3 tools/render_face_preview.py /tmp/face.png` draws the same preview to PNG (requires Pillow). Use `--preset excited` to inspect another preset. Run `python3 tests/face_geometry_test.py` to check the reference proportions, valid canvas options, and generated geometry.

The driver allocates one 240×240 RGB565 frame buffer (115,200 bytes) with `ps_malloc`; confirm PSRAM on the physical board during bring-up. The renderer caches the fixed face outline as horizontal spans using 960 bytes of RAM; expression parameters and colors remain dynamic.

Movement uses the driver's configured ±8 g and ±512°/s IMU scales. Tilt follows a settled gravity estimate and holds its last value while movement prevents a reliable estimate. A low-acceleration phase followed by an impact triggers a jump overlay. When gravity is unavailable, jump detection uses a stricter acceleration magnitude threshold to reduce walking reactions; other movement may still trigger it or gentler jumps may be missed. Tune the thresholds on the assembled wearable in #15.

Download the [official demo ZIP](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.28/ESP32-S3-Touch-LCD-1.28-Demo.zip), then install the required drivers from the repository root:

```sh
python3 tools/prepare_waveshare.py /path/to/ESP32-S3-Touch-LCD-1.28-Demo.zip
arduino-cli compile --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=enabled,PartitionScheme=app3M_fat9M_16MB firmware
```

With both boards connected, compile once and upload the same binary to both ports:

```sh
tools/upload_both.sh
```

The helper finds the two Waveshare USB serial ports automatically. Uploads run sequentially; a failed first upload stops before the second board is changed.

Flash size and partition scheme are separate settings. Without the explicit partition scheme, this board package defaults to a 1.2 MB app partition and reports a different program-storage percentage.

Installed vendor files are ignored by Git; the script retains their original copyright/license headers and adapts the multi-byte SPI transfer to a write-only call so presentation preserves the framebuffer. The script fails before installing files if the expected vendor call changes. It also installs the QMI8658 driver used to provide gravity measurements for compass tilt compensation.

Battery voltage uses GPIO1 and the documented 3:1 divider with calibrated ADC millivolts. Readings are cached for one second and refreshed immediately when the settings/battery screen opens. Charging state remains unknown: the [board documentation](https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.28) documents battery voltage on GPIO1 but does not identify a software-readable charger status GPIO; the [schematic](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.28/ESP32-S3-Touch-LCD-1.28-Sch.pdf) labels the charger STAT pin but does not establish a documented firmware input. Confirm the actual board revision and wiring before adding charge detection. Percentage stays unavailable until a battery-specific calibration is supplied. Confirm readings against a meter before relying on them.

## Runtime controls

Tap the face 5 times quickly to open battery/settings, then tap once to cycle between battery/settings and peer direction or hold for 3 seconds to return to the face. Each tap and the gap between taps must be no longer than 500 ms. Seven seconds without touch on either screen returns to the face. Hold the screen for 7 seconds to enter deep sleep. After releasing it, hold for 7 seconds to wake; a shorter touch returns to sleep. The loop continues sampling while settings is visible. Send `b` over Serial to open settings immediately. Serial commands `0`, `1`, and `2` select idle, curious, and excited presets for testing.

At 115200 baud, send `t` to trigger the jump reaction without moving the wearable. Send `j` to toggle temporary jump diagnostic output. Each `[DEBUG-jump]` sample contains loop time, behavior time, sensor timestamp, fresh and motion-valid flags, vertical acceleration in g, raw acceleration in mg, gyro in degrees per second, and overlay state. Logging can slow sampling, so leave it off outside testing.

The external compass starts disabled until verified. The compass uses a separate I²C controller on GPIO16 (SDA) and GPIO15 (SCL), while the onboard touch controller and IMU remain on GPIO6/GPIO7. At 115200 baud, send `s` to scan the compass bus. After checking the physical QMC5883P marking, 3.3 V wiring, and sensor-axis alignment, send `v` to enable the detected `0x2C` device. The adapter also verifies chip identity. Successful verification is saved in NVS; on later boots the firmware probes the address and checks the chip again before enabling it. Send `u` to forget verification and calibration before replacing or remounting the sensor, then physically inspect, scan, and verify again. If storage reports a failure, do not replace the sensor until forgetting verification succeeds. Send `c` to collect calibration while rotating the assembled wearable through all axes, `w` to save, `x` to cancel, and `r` to toggle raw/heading logs. Save reports the sample count, axis spans, and failure reason; raw logging reports read failures at the same rate. Collection requires available calibration storage. Calibration is stored per board in NVS. Recalibrate after changing the sensor, mounting, or enclosure.

The IMU and magnetometer gravity vectors must use the same axes. Confirm the external module mounting before enabling; a rotated mounting needs an explicit axis transform. Invalid or stale headings stay unavailable. See `../docs/compass-bench.md` for the calibration limits and acceptance checklist.

The gravity estimate rejects rotation that changes tilt above 10°/s, but permits rotation around gravity so a steady turn can supply direction evidence. Acceleration magnitude, direction stability, freshness, and the 250 ms settling window still gate validity. Turns that do not cover enough headings within the five-second evidence window keep the centered-eye fallback. Verify this distinction while worn; synthetic turn tests do not establish physical heading accuracy.

## Compass bench setup

`src/compass/CompassService.h` provides the C++11 Arduino API for verified QMC5883P setup, raw updates, tilt-compensated heading, and calibration stored through ESP32 Preferences. It is disabled by default and requires a caller-configured `TwoWire` bus plus explicit user verification of the chip and scanned address. Main owns serial commands. Follow [the QMC5883P API and bench procedure](../docs/compass-bench.md) before enabling it.

`shared/expression_presets.json` is the source of truth for expression presets. Run this from the repository root after changing it:

```sh
python3 tools/generate_face_presets.py
```

This regenerates the header consumed by Arduino firmware. Run `python3 tools/generate_face_presets.py --check` before committing preset changes.

## BLE discovery

Each wearable advertises without accepting connections or scan requests and passively scans at the same time; there is no leader or access point. The 48-bit ID is derived from `ESP.getEfuseMac()`, also shown on the settings screen. Manufacturer data uses the experimental `FF FF` company identifier followed by `NINA`, protocol version `01`, discovery type `01`, and the six ID bytes in big-endian order. This prototype identifier is not an assigned Bluetooth company ID and must be replaced before a product release. Other lengths, versions, types, invalid RSSI and the local ID are ignored.

The behavior retains up to four peers; additional new IDs are ignored until a slot expires. A peer expires after 3 seconds without a valid observation. The strongest recent BLE RSSI becomes the target, and a different peer must be at least 6 dB stronger to switch. A selected peer produces curious/focused interaction, changing to excited after 10 seconds of uninterrupted selection. Expiry returns to the manually selected expression (idle by default). `src/behavior/NearbyInteraction.h` owns peer continuity, target selection, sustained proximity, and target-scoped direction evidence. Its durations and switching margin are named constants for worn tuning. Discovery continues on the settings screen. The callback queue holds eight observations; excess observations are dropped rather than blocking the radio task.

The BLE protocol is unauthenticated and is for prototype discovery only. Physical identity uniqueness, two-way discovery, radio coexistence, packet loss, and worn timing need the two-wearable checks in issue #15.

Observations carry their callback receive time through the queue, so delayed or older packets cannot extend a peer's lifetime or replace newer signal evidence. Each drained batch is considered together before peer selection, preserving continuity when a later queued observation bridges the grace period. Advertising configuration and start complete asynchronously: immediate request failures and completion failures are printed to serial, while `begin()` reports queue, configuration-request, and scan-start failures. A successful return does not guarantee advertising has started; missing completion events and actual over-the-air operation require hardware verification.
# Approximate gaze radio

BLE remains authoritative for the current nearby peer. ESP-NOW sends a 12-byte `NINA`, version 1, type 2 beacon containing the same six-byte big-endian eFuse identity advertised over BLE. Both wearables use Wi-Fi station mode on channel 1, with no access point. Broadcasts are unauthenticated and must not be treated as proof of identity.

ESP32 Arduino 2.0.12 / ESP-IDF 4.4 does not expose packet RSSI in the ESP-NOW callback. The Wi-Fi promiscuous callback reads RSSI and the identity from the same validated, unencrypted ESP-NOW v1 broadcast frame. The callback copies observations into a bounded queue. `HeadingHistory` pairs each observation with the nearest recorded compass heading within 40 ms and rejects packets more than 250 ms old; observations without a nearby valid heading are discarded. Radio/compass timing, frame layout, and BLE coexistence require physical verification in #15. Wi-Fi sniffer reception alongside BLE scanning can be unstable; dropped observations must lead to centered eyes.

Gaze tuning lives in `src/behavior/DirectionEvidence.h`: eight 45-degree heading buckets, at least two readings in each of three buckets, a maximum 15 dB bucket spread, minimum -85 dBm signal, minimum 6 dB contrast, 5-second bucket lifetime, and a 2-second fallback after the last confident turn. A target change or expiry clears evidence. These are initial thresholds for worn testing, not measured guarantees.

For a bench visual, send `d` at 115200 baud to toggle the peer direction display. The white center is this wearable, the top is its front, and the green square shows the same approximate relative direction used for gaze. `NO PEER`, `NO HEADING`, or `TURN TO SAMPLE` replaces the square when the estimate is unavailable. The bottom line shows calibration status: `CAL SAVED`, `NO CAL SAVED`, `CALIBRATING` while collecting (heading stays invalid until `w` or `x`), `CAL INVALID` when a stored record was read and rejected, or `CAL UNKNOWN` when the compass is off or the stored calibration could not be read. Turn through several headings while the other wearable is nearby; the estimate expires when evidence becomes stale. Sensor and radio processing continue while the display is active. Send `d` again to return to the normal screen; seven seconds without touch also returns to the face. This is a rough RSSI estimate, not a measured position.

## Battery-life tuning

BLE scans with a 40% configured window and advertises every 160–180 ms. ESP-NOW starts after BLE discovers a peer and stops when the last peer expires (after the existing three-second grace period); failed starts retry every three seconds while a peer remains. The compass runs at 50 Hz. The settings screen redraws when its displayed text changes, while sampling and discovery continue.

Before relying on these settings, compare battery current with and without a nearby peer, and test two wearables for discovery latency, missed observations, repeated radio restarts, and gaze after worn compass calibration. Confirm 50 Hz raw readings and heading stability on each assembled wearable; the host tests cannot establish physical sensor or radio performance.

Deep sleep uses the touch controller interrupt on GPIO5 for wake and turns off the backlight. The touch controller remains in standby, and the external compass remains powered. The processor briefly wakes on a timer while waiting for the sleep-triggering touch to be released, then arms touch wake. On the assembled board, verify sustained touch reporting, wake after brief and long touches, no immediate re-wake on release, and battery current; neither wake reliability nor total sleep current can be confirmed by host tests.

## Display transfer comparison

The baseline is commit ddf1bfdda41b52dc3620844e715d063f10b2a03f; compare it with this revision using the same official demo ZIP and ESP32 Arduino 2.0.12. In separate clean worktrees, run `python3 tools/prepare_waveshare.py /path/to/demo.zip` and compile the firmware with the documented board options. For a host check, install the baseline-prepared driver in the current tree and run `sh tests/run_display_transfer_test.sh`: its buffer-preservation assertion fails because the simulated SPI transfer overwrites its input. Re-run preparation with this revision and the same ZIP, then run the test again: it checks all 240 writes of 480 bytes, their row order and byte values, and the unchanged frame. It also checks preparation on a fresh synthetic archive and rejects an unexpected upstream driver before replacing installed files.

Physical timing and current remain unmeasured. Under #15, measure repeated presentations of the same frame and board configuration for both revisions, separately recording SPI transfer duration (median, p95 and worst), panel behavior, and battery current. Reapply this revision's preparation after any baseline comparison; do not infer hardware FPS or current from the host test.
For physical comparison, run two wearables with identical firmware and settings while a phone actively scans and attempts to connect; compare peer discovery continuity and battery current against the previous connectable advertising build. Record results under #15; host checks do not establish physical compatibility or power savings.

## Renderer comparison

`tests/face_renderer_benchmark.cpp` renders 54 combinations of presets, mouth width and height extremes, tongue states, overlapping eyes, and contrasting colors. Its default run checks the corrected renderer's pixel hash. To compare revisions, compile the same benchmark with `c++ -std=c++17 -O2 tests/face_renderer_benchmark.cpp -o /tmp/face-bench` in this revision and in a clean worktree at `a55e197` after copying this benchmark file into that worktree. Run `/tmp/face-bench 20` in each worktree on the same machine. The hash must match; elapsed time includes hashing and is host-only, not a wearable FPS or current measurement.

On one Windows x64 GCC 16.2 run, the corrected baseline took 3.28 s and 2.70 s for 1,080 renders; the optimized renderer took 1.55 s and 1.32 s. Results vary with host load. On ESP32 the 48 cached mouth segments and per-column pointers use approximately 1,536 additional stack bytes while rendering; the outline still uses the existing 960-byte static span cache. The actual device speed and stack margin remain to be checked on hardware.
## Input and display timing

The runtime samples touch, QMI motion, compass and radio queues at most once every 20 ms (50 Hz), near the configured compass output rate, and schedules drawing independently at a minimum interval of 33 ms. The sensor and processor clocks can drift, so a poll may find no new compass conversion. That status keeps the previous still-fresh heading without changing its timestamp or adding direction evidence; errors and invalid gravity still invalidate it. It does not attempt catch-up reads when rendering or other work overruns; the next read uses its actual time. The QMI output configuration is unchanged. A display transfer still blocks the single loop while it runs, so worn latency and missed motion require measurement under #15.

For a repeatable host comparison, run `sh tests/run_wearable_behavior_test.sh`; the injected-time runtime check records five input updates and three display calls over 100 ms, then injects a 55 ms render and checks that the next update uses the actual 88 ms time without invented intermediate samples. The baseline at `ddf1bfdda41b52dc3620844e715d063f10b2a03f` gated inputs behind 33 ms display deadlines, giving three updates in that interval. These simulated counts are not physical performance measurements.

For the physical comparison under #15, build both revisions with the same pinned core and board settings. Under idle, animated face, nearby turning and settings workloads, timestamp input starts, render starts and render completion with the same logging or external instrument; compare input intervals and render durations (median, p95, p99 and worst), and count intervals exceeding 20 ms and frame starts more than 33 ms apart. Record whether logging itself changes those results, and verify touch, jump, tilt, peer expiry and gaze while worn. Physical timing, missed events and radio coexistence remain unverified.

## Unchanged-frame replay

Run `sh tests/run_wearable_behavior_test.sh` to replay idle behavior at 33 ms intervals for 5.6 seconds. At baseline `ddf1bfdda41b52dc3620844e715d063f10b2a03f`, all 170 face frames were presented; the same replay with frame suppression presents 20 and skips 150. This is an output-count comparison for one host workload, not a measured frame rate or battery saving. The input scheduler and peer processing continue even when a frame is skipped. Panel timing and current remain pending under #15.
