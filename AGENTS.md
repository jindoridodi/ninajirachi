# Agent Instructions

When changing the Ninajirachi wearable firmware, read [README.md](./README.md) for the current hardware contract, interaction behavior, wiring constraints, and implementation order. Read [GLOSSARY.md](./GLOSSARY.md) when changing domain terms such as `nearby`, `peer`, `direction evidence`, or `transient overlay`.

For board-specific display, touch, QMI8658, battery, or flashing work, consult the [official Waveshare ESP32-S3-Touch-LCD-1.28 documentation](https://docs.waveshare.com/ESP32-S3-Touch-LCD-1.28) before changing firmware configuration.

## QMC5883P work

When adding or changing magnetometer code:

1. Confirm the physical breakout marking and pin labels.
2. Verify the I²C bus and detected address with a scanner before choosing the driver configuration.
3. Power the module from the documented 3.3 V logic/supply path.
4. Keep the sensor orientation fixed and calibrate it after final enclosure assembly.
5. Keep compass reads, calibration, and heading conversion separate from gaze and animation state.

QMC5883P work is complete only when the device is detected, raw X/Y/Z readings are nonzero and stable enough to inspect, calibration data is available for that physical wearable, and the resulting heading can be consumed by the approximate-gaze behavior without exposing sensor-specific details to the animation states.

## Face rendering

The face screen uses the parameterized face model defined in `README.md` and ADR-0001. Add or tune expression presets and renderer parameters rather than creating image files for each expression. Keep the desktop face preview aligned with the wearable parameter model; reserve bitmap assets for decorative effects that cannot be drawn simply.

## Agent skills

### Issue tracker

Work is tracked in GitHub Issues for `jindoridodi/ninajirachi`. See `docs/agents/issue-tracker.md`.

### Triage labels

Use the default labels: `needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, and `wontfix`. See `docs/agents/triage-labels.md`.

### Domain docs

This is a single-context repository. Read root `GLOSSARY.md` and relevant ADRs under `docs/adr/`. See `docs/agents/domain.md`.
