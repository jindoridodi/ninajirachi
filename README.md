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

- Power the QMC5883P module from 3.3 V.
- Connect the module's `SDA`, `SCL`, and `GND` to the ESP32-S3 I²C bus; use the board's documented 3.3 V supply pin for sensor power.
- Confirm the exact Waveshare GPIO mapping, module pin labels, pull-ups, and detected I²C address before wiring.
- Mount the magnetometer away from metal, permanent magnets, the battery, display speaker or motor if added later, and high-current battery wiring. Keep its sensing face and axis orientation fixed relative to the display.
- Calibrate each physical wearable after the magnetometer is installed in its final enclosure. Repeat calibration if the enclosure, battery location, attachment hardware, or nearby wiring changes.
- Keep the battery protected, mechanically restrained, and inaccessible to the wearer during normal use. The enclosure must not pinch or short the battery.
- There is no physical power switch. Disconnect USB during normal wearing; disconnect the battery at the connector for service-level power-off.

## Arduino setup

Use Arduino C++ and upload through USB-C.

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
3. Eyes and eyelids
4. Pupils
5. Mouth
6. Effects

An expression preset supplies values such as eye openness, pupil offset, mouth shape/opening, colors, effects, and animation speed. Interpolate between presets for smooth blinking, gaze, tilt, jump overlays, and interaction changes.

### Face authoring

Use a desktop GUI preview/editor to tune expression presets at 240×240. It should show the same parameterized face model as the wearable and export preset values, not a collection of raster expression files. The desktop preview is an authoring tool only; it does not run on the wearable.

Reserve bitmap assets for occasional decorative effects that are substantially harder to draw procedurally. Test their flash and RAM cost early.

The animation loop should keep input sampling, peer tracking, state selection, and rendering separate. A small expression state machine is sufficient:

- `idle`: gentle pupil motion and occasional blink
- `curious`: target-directed gaze and slightly raised eyelids
- `happy`: bright eyes, animated mouth, and positive effects
- `surprised`: wide eyes and an open mouth
- `excited`: heightened animation after one peer remains nearby for a sustained period

Blink periodically, but let jump and excited reactions temporarily override the idle blink schedule. Keep animation timing non-blocking so sensor and wireless updates continue while frames are drawn.

The touch display has two screen modes:

- `face`: the normal animated Ninajirachi screen
- `settings/battery`: opened by holding the touchscreen for 5 seconds; a 3-second hold returns to the face, and 7 seconds without touch returns automatically

The settings/battery screen is read-only initially. Show battery voltage, charging state, estimated percentage only when calibrated, and the firmware/device ID. Continue sensor and wireless processing while this screen is visible.

## Movement behavior

Use the built-in QMI8658 accelerometer and gyroscope:

- Detect a jump from a short-lived acceleration pattern followed by a change in vertical motion or orientation.
- On a jump, flash or change the background and show surprised eyes.
- Use gyroscope and accelerometer readings for tilt/orientation behavior, such as small expression or pupil adjustments.
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
5. Convert the relative direction into `left`, `center`, or `right` and move the pupils accordingly.

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
2. Add periodic blinking, moving pupils, and animated mouth states.
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
- A single jump causes one debounced surprised reaction.
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
