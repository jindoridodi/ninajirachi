# Implementation handoff

## Current baseline

`main` contains the parameterized reference face, movement behavior, compass service, expression/settings screen, BLE peer discovery, and ESP-NOW direction evidence. Software integration is present; physical operation still needs the checks below.

- `shared/face_geometry.json` defines the rounded-triangle outline, solid eyes, and double-U mouth. The direct wearable renderer and desktop preview share this geometry.
- `shared/expression_presets.json` defines colors, eye openness/offsets, mouth width/depth, and tongue extension. Generate the firmware headers with `tools/generate_face_presets.py` and `tools/face_geometry.py`.
- `NearbyInteraction` owns peer continuity, target selection, sustained proximity, and approximate gaze evidence. `WearableBehavior` advances it once per frame and coordinates expressions, touch holds, and the movement overlay. Tilt shifts the solid eyes; a jump temporarily overrides the expression and then resumes it.
- The Arduino sketch uses the official Waveshare direct driver.

Read `README.md`, `AGENTS.md`, `GLOSSARY.md`, ADR-0001, the prototype spec, and the current GitHub issue before changing firmware.

## Verification

Run `sh tests/run_all.sh` for the host checks. The display-adapter test requires the official Waveshare source installed with `tools/prepare_waveshare.py`; the headless face-rendering test requires Pillow. See [firmware setup](../firmware/README.md) for the actual Arduino compile and upload workflow.

Hardware acceptance remains separate from software tests:

- Inspect the physical QMC5883P marking and wiring, scan I²C, verify raw readings, and calibrate after final enclosure assembly. Use [the compass bench procedure](./compass-bench.md).
- Inspect all expressions, blinking, gaze movement, jump overlays, and touchscreen holds on the round display.
- Compare battery voltage against a meter. Charging state is unknown and battery percentage is uncalibrated.
- Use two physical wearables to check BLE discovery, sustained proximity, ESP-NOW reception, and centered-eye fallback when direction evidence becomes stale.
- Tune movement and radio thresholds while worn.

The historical shared-face, compass, movement, and expression/settings checkpoints remain in Git history. Completed feature branches are not required to build `main`. Keep dirty worktrees and saved testing changes until their owner decides how to retain them.
