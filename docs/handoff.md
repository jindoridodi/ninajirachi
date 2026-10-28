# Implementation Handoff

## Published baseline

`main` contains the merged shared-face and movement work from [PR #16](https://github.com/jindoridodi/ninajirachi/pull/16):

- Parameterized face renderer and generated expression-preset contract
- 240×240 desktop face preview
- Arduino entry point plus Waveshare direct-renderer adapter boundary
- Host-side face-renderer and movement-behavior checks
- Official Waveshare documentation references
- Movement behavior with tilt pupil adjustment, debounced jump overlay, and resume

Read `README.md`, `AGENTS.md`, `GLOSSARY.md`, ADR-0001, the prototype spec, and the current GitHub issue before changing firmware.

## Active GitHub tickets

- [#13 Make the compass usable on the wearable](https://github.com/jindoridodi/ninajirachi/issues/13)
- [#14 Animate expressions and expose battery settings](https://github.com/jindoridodi/ninajirachi/issues/14)
- [#11 Make two wearables recognize and react to each other](https://github.com/jindoridodi/ninajirachi/issues/11), blocked by #14
- [#10 Make face gaze respond to a peer](https://github.com/jindoridodi/ninajirachi/issues/10), blocked by #13 and #11
- [#15 Validate the worn two-wearable experience](https://github.com/jindoridodi/ninajirachi/issues/15)

## Published WIP branches

### Compass: `codex/issue-13-compass`

Commit `e2b76ec` preserves QMC5883P I²C/configuration and calibration scaffolding, a compass bench guide, and passing host-side calibration tests. It is unmerged WIP: it still needs real-board I²C detection, raw readings, calibration capture/storage, and integration with the wearable runtime.

### Expressions/settings: `codex/issue-14-expressions-settings`

Commit `89583f1` preserves expanded expression presets, wearable behavior/settings scaffolding, and dedicated tests. It is unmerged WIP. `sh tests/run_wearable_behavior_test.sh` currently fails at `idle face closes its eyes during a blink`; fix that before merging or continuing #14.

## Hardware handoff

Read [Waveshare board notes](./research/waveshare-board-notes.md) before display, touch, battery, or QMI8658 work. The external QMC5883P is not bench-verified; begin with a physical module inspection and I²C scan.

## Current limits

The complete peer-discovery, ESP-NOW gaze, battery/settings UI, compass runtime, and worn-test flows are not finished. Do not present the current firmware as wearable-ready until the open tickets and real-hardware validation are complete.
