# Implementation handoff

## Compass verification, 2026-11-12

Both peer-direction screens alternated between NO HEADING and TURN TO SAMPLE. A temporary diagnostic on port ending 1891 showed successful magnetometer reads but frequent zero gravity estimates during a controlled, flat turn; tilt rotation repeatedly crossed the 10-degree/second gate and restarted the 250 ms settling window. With the wearable stationary, three consecutive two-second windows reported valid headings throughout. The normal firmware was restored after testing. Investigate motion handling without weakening the rejection of unreliable gravity; peer-direction sampling remains unverified. To identify a wearable for the user, send serial `t` to trigger its jump reaction before asking them to move or inspect it.

The normal-turn trace is `tests/fixtures/2026-11-12-normal-turn.txt`. Each line is `nowMs sensorTimestamp axMg ayMg azMg gxDps gyDps gzDps recordedFlag`. `sh tools/replay_gravity_trace.sh` replays that file through `GravityEstimator` and reports 347 of 478 samples without gravity (exit 1). The trailing flag is from the capture and is ignored. Tilt rate exceeded 10 degrees/second in 188 samples; 13 acceleration magnitudes were outside the accepted 0.9–1.1 g range. Some tilt rates were much higher than the threshold. The diagnostic firmware was removed from the wearable. The bias and tilt-average experiments below were offline checks of this trace; the replay tool does not repeat them.

The final 96 recorded samples, after the user set the wearable down, averaged gyro X=5.27, Y=-6.18, Z=-0.01 degrees/second (standard deviations 0.62, 0.65, 0.71). Subtracting an approximate fixed bias (5, -6, 0) for an offline experiment reduced unavailable gravity from 347/478 to 310/478; gyro offset contributes but does not explain the whole failure. A two- to eight-sample tilt-rate average also left at least 66.5% unavailable. Neither is a sufficient fix; avoid hard-coding this board's bias or weakening motion rejection based on this trace alone.

The user replaced a faulty QMC5883P. Both wearables scanned at 0x2C and produced nonzero raw readings. After earlier rejected saves, the replacement on port ending 0621 was calibrated successfully: 836 samples, spans X=2574, Y=2967, Z=2772, and save status ok. Once stationary it reported a heading near 70 degrees, with 28 of 31 readings valid. The other wearable on port ending 1891 previously produced 27 valid heading samples. Heading accuracy, intermittent validity, and two-wearable gaze remain to be checked.

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
