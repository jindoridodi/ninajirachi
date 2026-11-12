# Wearable Ninajirachi Icon

This project is a battery-powered, wearable digital Ninajirachi icon. Each unit can be worn as a necklace or hairclip, animates a face on a round display, detects wearer movement, and interacts with nearby Ninajirachi units.

The interaction model is intentionally playful: a unit identifies nearby units, chooses the strongest signal as its current target, and uses compass-guided signal sampling to make a rough left/center/right gaze guess.

## Hardware

Each wearable contains:

- Waveshare ESP32-S3 Touch LCD 1.28 board
  - 240×240 round touch display
  - ESP32-S3 Wi-Fi and Bluetooth LE
  - built-in QMI8658 accelerometer/gyroscope
  - 3.7 V Li-Po connector and charging circuit
- Protected 3.7 V Li-Po battery
- External GY-271/QMC5883P 3-axis magnetometer/compass breakout module
- Wearable enclosure

The linked [GY-271/QMC5883P module](https://www.amazon.com/dp/B0G1RSJBVX?ref=ppx_yo2ov_dt_b_fed_asin_title) is treated as a QMC5883P I²C breakout. The QMC5883P sensor itself is a 3-axis, 16-bit magnetometer with a 2.5–3.6 V supply range and selectable magnetic-field ranges; the [QST datasheet](https://www.qstcorp.com/upload/pdf/202512/13-52-19%20QMC5883P%20Datasheet%20Rev.E.pdf) is the electrical reference. Breakout-board labels and regulators vary, so verify the actual board marking and pin labels before powering it.

The expected QMC5883P I²C address is `0x2C` ([Adafruit reference](https://learn.adafruit.com/adafruit-qmc5883p-triple-axis-magnetometer/pinouts)), but generic GY-271 boards are sometimes sold with different chips or labels. Run an I²C scanner first and confirm that the detected device and library match the physical module. Do not assume the older QMC5883L address or driver merely because the board is called GY-271.

### Wiring and mechanical constraints

- Connect the QMC5883P module's `SDA` to GPIO16 and `SCL` to GPIO15, `VCC` to the board's 3.3 V supply, and `GND` to board ground.
- Confirm the exact Waveshare GPIO mapping, module pin labels, pull-ups, and detected I²C address before wiring.
- Mount the magnetometer away from metal, permanent magnets, the battery, display speaker or motor if added later, and high-current battery wiring. Keep its sensing face and axis orientation fixed relative to the display.
- Calibrate each physical wearable after the magnetometer is installed in its final enclosure. Repeat calibration if the enclosure, battery location, attachment hardware, or nearby wiring changes.
- Keep the battery protected, mechanically restrained, and inaccessible to the wearer during normal use. The enclosure must not pinch or short the battery.
- There is no physical power switch. Disconnect USB during normal wearing; disconnect the battery at the connector for service-level power-off.

## Arduino setup

Use Arduino C++ and upload through USB-C.

The [official Waveshare ESP32-S3-Touch-LCD-1.28 documentation](https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.28) is the source of truth for the board schematic, Arduino examples, pinout, display/touch/IMU details, battery measurement, and flashing procedure. Check it before changing board-specific code.

1. Install the ESP32 Arduino board package.
2. Select `ESP32S3 Dev Module` as the board target.
3. Select the connected USB-C port.
4. Add the display, touch, QMI8658, and QMC5883P libraries required by the board and sensor examples.
5. Start from the Waveshare display and QMI8658 examples to verify the board before combining features.

The first firmware milestone should render a static face and print sensor readings before adding wireless interaction.

During bench testing, USB charging/testing is acceptable. Treat the wearable as disconnected from USB while worn.

## Face rendering and animation

Render the face from parameters rather than maintaining separate images for every expression. The renderer has logical layers, but draws them from values at runtime:

1. Code-drawn background
2. Face shape
3. Solid rounded-rectangle eyes
4. Continuous double-U mouth

The face is a white filled rounded triangle with black eyes and mouth on a black background, with its wide aspect ratio preserved inside 240×240. The previous blue face color was `#0000EE`, saved here for future reference. `shared/face_geometry.json` supplies the curves and eye proportions used by the firmware renderer and desktop preview. After geometry edits, run `python3 tools/face_geometry.py`.

An expression preset supplies eye openness, `eyeOffsetX/Y` gaze offsets, mouth width/depth, optional tongue extension (`tongueOut`), and colors. Jump overlays cycle the background through several colors. The face has no decorative dot above it. Interpolate between presets for smooth blinking, gaze, tilt, jump overlays, and interaction changes. Exported presets use only these current parameters; re-export any saved presets made with the placeholder model.

### Face authoring

Use a desktop GUI preview/editor to tune expression presets at 240×240. It should show the same parameterized face model as the wearable and export preset values, not a collection of raster expression files. The desktop preview is an authoring tool only; it does not run on the wearable.

Launch the editor with `python3 preview/face_preview.py`. Tk (`tkinter`) is required only to launch the GUI; preset export and headless tests (`python3 tests/preview_editor_test.py`) do not require it.

Run `sh tests/run_all.sh` for all host checks (Pillow and the installed official Waveshare source are required). These checks cover rendering, preview export, movement, peer behavior, settings, and compass calculations; hardware acceptance still requires the assembled wearables.

Reserve bitmap assets for occasional decorative effects that are substantially harder to draw procedurally. Test their flash and RAM cost early.

The animation loop should keep input sampling, peer tracking, state selection, and rendering separate. A small expression state machine is sufficient:

- `idle`: gentle eye motion and occasional blink
- `curious`: target-directed gaze and slightly wider eyes
- `excited`: heightened animation after one peer remains nearby for a sustained period

Blink periodically, but let jump and excited reactions temporarily override the idle blink schedule. Keep animation timing non-blocking so sensor and wireless updates continue while frames are drawn.

The touch display has two screen modes:

- `face`: the normal animated Ninajirachi screen
- `settings/battery`: opened by 10 quick taps; 5 quick taps return to the face, and 7 seconds without touch returns automatically. Each tap and the gap between taps must be no longer than 500 ms

The settings/battery screen is read-only initially. Show battery voltage, charging state, estimated percentage only when calibrated, and the firmware/device ID. Continue sensor and wireless processing while this screen is visible.

## Movement behavior

Use the built-in QMI8658 accelerometer and gyroscope:

- Detect a jump from a short-lived acceleration pattern followed by a change in vertical motion or orientation.
- On a jump, flash or change the background and show wider eyes.
- Use gyroscope and accelerometer readings for tilt/orientation behavior, such as small expression or eye-offset adjustments.
- Tune thresholds while the device is worn; a desk test is not enough.

Jump detection should have a cooldown/debounce period so one jump produces one reaction rather than a burst of reactions.

## Nearby-wearable interaction

Every unit needs:

- A persistent unique device ID, derived from the ESP32 identity or generated and stored once.
- A discovery/beacon message containing the Ninajirachi device identity.
- A peer table containing recently seen IDs, signal strength, and last-seen time.
- Expiration of peers that have not been seen for a timeout.
- A target selector that chooses the strongest recent signal as the approximate closest peer.

The unit should use focused interaction when one valid peer is nearby, then enter excited interaction after sustained proximity. Multiple peers remain a future extension rather than a first-prototype requirement.

### Wireless implementation note

BLE is authoritative for peer identity and nearby/stale status. Use ESP-NOW for direct Wi-Fi-side beaconing and RSSI samples; no access point should be required. Wi-Fi RSSI is only useful if the units expose a discoverable mechanism that reports a meaningful signal level; ordinary Wi-Fi scanning does not automatically discover arbitrary ESP32 peers.

The protocol should include a version, device ID, message type, and optional animation/event fields. Ignore malformed packets and expire stale entries. The current target should remain stable unless another peer is clearly stronger.

## Approximate direction and gaze

This is a playful approximation, not precise tracking.

1. Read the QMC5883P compass heading.
2. While the wearer naturally turns, record the target peer's Wi-Fi RSSI together with the current compass heading.
3. Smooth several samples for each heading window while the wearer naturally turns.
4. Treat the heading with the strongest averaged RSSI as the target's approximate direction.
5. Convert the relative direction into `left`, `center`, or `right` and shift the eyes accordingly.

The estimate works best when the wearer turns naturally. Bodies, clothing, walls, reflections, antenna orientation, and multipath can make RSSI misleading. If the signal is too weak, too unstable, or has not been sampled across enough headings, keep the eyes centered rather than pretending the direction is known. Hold the last confident gaze briefly, then use the centered-eye fallback after roughly 2 seconds without a fresh confident update.

Calibrate the magnetometer after final installation. Calibration must account for hard-iron and soft-iron distortion from the enclosure and nearby hardware; repeat it if the enclosure or battery position changes.

## Suggested firmware structure

Keep the first implementation small and explicit:

```text
setup()
  initialize display and touch
  initialize QMI8658
  initialize QMC5883P
  initialize BLE identity and discovery
  initialize ESP-NOW beaconing

loop()
  read sensors
  update jump and orientation state
  receive/advertise peer beacons
  expire stale peers
  update target-scoped RSSI/heading estimate
  select expression preset and overlays
  update animation timers
  render layered frame
```

Useful data records are a peer record (`id`, `rssi`, `lastSeen`, and optional direction estimate), a sensor snapshot, and an expression state. Avoid adding a general-purpose framework until the behavior requires it.

## Recommended implementation order

1. Render a parameterized face and expression presets.
2. Add periodic blinking, moving eyes, and animated mouth states.
3. Add jump detection and a flashing background.
4. Add magnetometer readings and compass calibration.
5. Build and test a second wearable.
6. Add Wi-Fi/BLE discovery, unique IDs, and peer tracking.
7. Implement compass-guided ESP-NOW RSSI sampling for approximate gaze direction.
8. Build the enclosure, connect the battery, and tune thresholds while worn.

## Validation checklist

- The parameterized face renders correctly at 240×240 without visible layer artifacts.
- The desktop face preview can inspect the same expression presets used by the wearable.
- Blinking and expression transitions do not block sensor or wireless processing.
- A single jump causes one debounced jump reaction.
- Tilt changes behavior without causing unstable jitter.
- A unit sees its own stable ID and ignores malformed or stale peer data.
- Two units discover each other and select the stronger recent signal.
- One peer transitions from focused to excited after sustained proximity.
- Compass readings remain usable after the final enclosure is installed and calibrated.
- Direction estimates degrade gracefully to centered eyes when evidence is insufficient.
- Battery charging, USB disconnect, battery-connector service access, strain relief, and enclosure closure are safe before wearing.

## Known limits

- Wi-Fi RSSI is a rough proximity and direction signal, not a ranging or localization system.
- Compass readings are distorted by nearby metal, magnets, current, and enclosure materials.
- A protected Li-Po battery and charging circuit still require careful mechanical and electrical handling.
- Generic GY-271 listings may not identify the actual magnetometer consistently; verify the chip, address, and driver on every module batch.
