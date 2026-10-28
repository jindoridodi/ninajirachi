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

## Resumed tickets #13 and #14

`codex/finish-wearable-prototype` integrates the compass and expression/settings software. The firmware uses a nonblocking loop, a real battery-voltage input, serial calibration controls, and the touchscreen settings gestures. See `firmware/README.md` for setup and commands. The entries below identify the original preserved checkpoints, not the latest implementation.

## Original WIP checkpoints

### Compass: `codex/issue-13-compass`

Commit `e2b76ec` preserves QMC5883P I²C/configuration and calibration scaffolding, a compass bench guide, and passing host-side calibration tests. It is unmerged WIP: it still needs real-board I²C detection, raw readings, calibration capture/storage, and integration with the wearable runtime.

### Expressions/settings: `codex/issue-14-expressions-settings`

Commit `89583f1` preserved expanded expression presets, wearable behavior/settings scaffolding, and dedicated tests. Its idle-blink failure is addressed in the resumed #14 work.

## Hardware handoff

Read [Waveshare board notes](./research/waveshare-board-notes.md) before display, touch, battery, or QMI8658 work. The external QMC5883P is not bench-verified; begin with a physical module inspection and I²C scan.

## Current limits

Peer discovery, ESP-NOW gaze, and the final worn test are outside the resumed #13/#14 work. Compass detection and calibration in the physical enclosure, touch hold behavior on the actual controller, battery measurement against a meter, and display appearance still require a connected board. Host tests and compilation cannot substitute for that evidence.
