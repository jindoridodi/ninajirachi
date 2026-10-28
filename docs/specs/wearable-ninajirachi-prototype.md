# Wearable Ninajirachi Prototype

## Problem Statement

The project needs a battery-powered Ninajirachi wearable that can be worn as a necklace or hairclip, show an animated face, react to the wearer’s movement, and interact with another nearby wearable. The interaction should feel expressive and social without pretending that inexpensive radio and compass measurements can provide precise tracking.

## Solution

Build a two-wearable prototype around the Waveshare ESP32-S3 Touch LCD 1.28 board. Each wearable renders a parameterized Ninajirachi face, reads the built-in QMI8658 motion sensor and an external QMC5883P magnetometer, discovers peers over BLE, and exchanges direct Wi-Fi-side beacons through ESP-NOW for RSSI evidence.

BLE is authoritative for peer identity and nearby/stale status. ESP-NOW RSSI samples are used only as direction evidence while the wearer naturally turns. The selected target produces a coarse left, center, or right approximate gaze. If evidence becomes insufficient, the eyes return to center shortly afterward.

The first prototype supports one peer well. A peer begins with focused interaction and reaches excited interaction after sustained proximity. A jump produces a transient visual overlay that preserves the underlying interaction state. The touchscreen provides a read-only settings/battery screen.

## User Stories

1. As a wearer, I want to see an animated Ninajirachi face on the round display, so that the device feels alive while I wear it.
2. As a wearer, I want the face to be rendered from expression parameters, so that expressions can change without managing an image file for every state.
3. As a wearer, I want the face to blink periodically, so that idle animation feels natural.
4. As a wearer, I want pupils and the mouth to move during idle and interaction states, so that the device communicates emotion without sound.
5. As a wearer, I want the face to react to tilt and orientation, so that small movements produce expressive behavior.
6. As a wearer, I want a jump to trigger surprised eyes and a changing or flashing background, so that the device visibly reacts to movement.
7. As a wearer, I want one jump to produce one debounced reaction, so that sensor noise does not create repeated surprises.
8. As a wearer, I want the jump reaction to appear immediately over the current expression, so that the movement response feels timely.
9. As a wearer, I want the focused or excited interaction to resume after the jump reaction, so that a jump does not erase the peer interaction.
10. As a wearer, I want each wearable to have a stable unique identity, so that two wearables can recognize one another across reboots.
11. As a wearer, I want a wearable to discover another wearable over BLE, so that nearby status does not depend on an access point.
12. As a wearer, I want stale peer records to expire after a short absence, so that the face does not react forever to a peer that has left.
13. As a wearer, I want the wearable to select the strongest recent peer signal as its target, so that it has a simple notion of which peer is closest.
14. As a wearer, I want target selection to resist small RSSI fluctuations, so that the eyes do not flicker between targets.
15. As a wearer, I want the face to enter focused interaction when one peer is nearby, so that the wearable acknowledges another person’s wearable.
16. As a wearer, I want focused interaction to become excited interaction after sustained proximity, so that two wearables feel more engaged over time.
17. As a wearer, I want the wearable to estimate whether the selected peer is to my left, center, or right, so that the pupils can look generally toward it.
18. As a wearer, I want direction evidence to be collected while I naturally turn, so that compass heading and RSSI can produce a playful directional estimate.
19. As a wearer, I want the gaze to return to centered eyes soon after directional evidence stops being reliable, so that the face does not hold a misleading direction indefinitely.
20. As a wearer, I want the wearable to return to ordinary idle behavior when no peer is nearby, so that the device remains useful when worn alone.
21. As a builder, I want the QMC5883P module to be discovered and read over I²C, so that the wearable has an external compass heading.
22. As a builder, I want the QMC5883P to be powered and mounted correctly, so that battery wiring, enclosure metal, and magnets do not corrupt compass readings.
23. As a builder, I want each physical wearable to be calibrated after final assembly, so that its own enclosure and sensor placement are accounted for.
24. As a builder, I want QMC5883P raw readings and calibration to be inspectable during development, so that hardware faults can be separated from gaze logic.
25. As a wearer, I want a five-second touchscreen hold to open the settings/battery screen, so that I can inspect the device without a separate control.
26. As a wearer, I want a three-second hold on the settings/battery screen to return to the face, so that I can exit deliberately.
27. As a wearer, I want the settings/battery screen to return automatically after seven seconds without touch, so that the face comes back without a precise gesture.
28. As a wearer, I want the settings/battery screen to show battery voltage and charging state, so that I can understand the current power condition.
29. As a wearer, I want estimated battery percentage shown only when it is calibrated, so that the screen does not imply false precision.
30. As a builder, I want the settings/battery screen to show firmware/device identity, so that I can distinguish and diagnose multiple wearables.
31. As a builder, I want sensor, wireless, and interaction processing to continue while the settings/battery screen is visible, so that opening the screen does not create a hidden pause.
32. As a builder, I want the device to be programmable and testable over USB-C with Arduino C++, so that the prototype can be iterated quickly.
33. As a builder, I want the firmware to target `ESP32S3 Dev Module`, so that the documented upload process matches the intended Arduino setup.
34. As a builder, I want the prototype to remain usable for several hours, so that worn testing can reveal useful thresholds and interaction problems.
35. As a builder, I want all behavior thresholds to be easy to find and tune, so that jump, peer, gaze, settings, and battery behavior can be calibrated on real hardware.
36. As a face designer, I want a desktop GUI preview of the parameterized face at 240×240, so that I can tune expression presets before uploading firmware.
37. As a face designer, I want the desktop preview and wearable renderer to share the same expression preset model, so that a previewed face is the face shown on the wearable.

## Implementation Decisions

- Use Arduino C++ on the Waveshare ESP32-S3 Touch LCD 1.28 board with the `ESP32S3 Dev Module` target.
- Use a parameterized face renderer with logical background, face, eye/eyelid, pupil, mouth, and effects layers. Expression presets supply values instead of selecting separate expression image files.
- Provide a desktop GUI preview/editor for tuning the same expression presets at 240×240. It is an authoring tool, not a wearable runtime dependency.
- Reserve bitmap assets for optional decorative effects that cannot be drawn simply; validate their flash and RAM cost early.
- Keep sensor input, peer tracking, interaction-state selection, screen-mode selection, animation timing, and rendering as separate responsibilities behind the `WearableBehavior` seam.
- The `WearableBehavior` seam accepts sensor snapshots, peer/discovery snapshots, touch input, and elapsed time; it produces expression state, gaze direction, transient overlays, screen mode, and renderable animation state.
- Use the built-in QMI8658 accelerometer/gyroscope for jump detection and expressive tilt/orientation changes.
- Treat a jump as a transient overlay: it takes visual priority, preserves the focused or excited interaction underneath, and resumes that state when the reaction ends.
- Derive a stable unique peer ID from the ESP32 hardware identity for the prototype.
- Use BLE for peer identity, discovery, nearby status, and stale-peer expiration.
- Use ESP-NOW for direct Wi-Fi-side beacons and RSSI samples; no access point is required for peer interaction.
- Keep peer selection stable with a strength margin and/or hold time before switching to another peer.
- Use the QMC5883P breakout as an external 3-axis I²C magnetometer. Power it from the documented 3.3 V path, verify the physical module marking and I²C address with a scanner, and keep its axis orientation fixed relative to the display.
- Expect QMC5883P address `0x2C` only after verification; generic GY-271 boards may contain a different chip or use different labels.
- Calibrate each physical magnetometer after final enclosure assembly. Keep compass reads, calibration, and heading conversion separate from expression and gaze state.
- Produce only left, center, or right gaze. Treat RSSI/heading direction as approximate evidence, not tracking or localization.
- Collect direction evidence only while a selected peer is nearby and the wearer is naturally turning. Hold the last confident gaze briefly, then use centered-eye fallback after roughly 2 seconds without a fresh confident update.
- Start focused interaction with one nearby peer and transition to excited interaction after roughly 10 seconds of sustained proximity. Keep the duration as a named tuning constant.
- Use a short named stale-peer timeout, initially around 3 seconds, to tolerate brief wireless loss.
- Use named, easy-to-find timing constants for gaze fallback, stale peers, sustained proximity, jump cooldown, and the 5/3/7-second touchscreen behavior.
- Use two screen modes: face and settings/battery. The settings/battery screen is read-only initially.
- There is no physical power switch. USB is treated as disconnected during normal wearing; battery-connector disconnection is the service-level hard power-off. Bench charging/testing is allowed.
- Keep one electronics and behavior design for necklace and hairclip mounting. Require one fixed upright orientation for the first prototype; defer automatic display rotation.

## Testing Decisions

- Test the `WearableBehavior` seam through observable outputs, not private implementation details.
- Cover idle blinking and expression transitions, jump overlay/resume, tilt behavior, peer discovery state, stale-peer expiration, stable target selection, sustained proximity, gaze fallback, no-peer idle behavior, and touchscreen timing.
- Use independent known inputs for RSSI/heading aggregation tests; do not reproduce the implementation’s calculation to generate expected values.
- Test hardware adapters and integration through bench checks: I²C detection, verified address, nonzero QMC5883P X/Y/Z readings, calibration behavior, heading changes during rotation, BLE discovery, ESP-NOW beacon reception, display output, touch holds, battery readings, and USB behavior.
- Test with two physical wearables for the primary interaction path.
- Tune jump, stale-peer, sustained-proximity, gaze, settings, and battery thresholds while worn; desk-only validation is insufficient.
- Completion requires the parameterized face to render without layer artifacts, the desktop preview to use the same presets, one jump to produce one debounced overlay, two wearables to discover each other, direction to degrade to centered eyes when uncertain, and the QMC5883P to remain usable after final enclosure calibration.

## Out of Scope

- Precise peer localization, ranging, or guaranteed physical-distance measurement.
- More than two wearables as a first-prototype requirement, although the peer model should not prevent future expansion.
- IR left/center/right sensors.
- Automatic screen rotation.
- Editable settings on the wearable screen.
- Advanced wireless diagnostics on the wearable screen.
- Audio, names, personal data, or user accounts.
- A physical power switch.
- A production-ready enclosure, all-day battery life, or final power optimization.
- A complete QMC5883P driver implementation before the board’s exact pinout, module marking, address, and Arduino library are verified on the bench.
- A full on-device GUI editor or a general-purpose GUI framework for the animated face.

## Further Notes

The RSSI/compass approach is intentionally expressive rather than precise. Bodies, clothing, walls, antenna orientation, magnetic materials, current, and multipath can distort the result. When evidence is weak, the correct behavior is centered eyes rather than a confident-looking incorrect direction.

The linked magnetometer is treated as a GY-271/QMC5883P-style breakout, but generic GY-271 listings are not a reliable chip identity. Hardware setup must begin with an I²C scan and physical inspection.
