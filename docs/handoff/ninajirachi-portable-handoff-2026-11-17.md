# Ninajirachi portable handoff — 2026-11-17

Continue the two-wearable compass and approximate peer-direction investigation. The user wants a direction that is physically meaningful, not merely a permissive confidence label.

## Workspace and recovery

- Repository: https://github.com/jindoridodi/ninajirachi
- Captured branch: `codex/compass-handoff-2026-11-17`, commit `6dad6569c033ab093834adffcd0e50b0acc956d5`; the source worktree was clean at handoff. The accompanying Git bundle contains the complete history through this commit and can be cloned offline: `git clone ninajirachi-portable-2026-11-17.bundle ninajirachi`. Check out the captured commit and create a working branch as needed.
- Begin with `docs/handoff/2026-11-17-current.md` and the current section of `docs/handoff.md`. They reference the captured evidence and remaining work. The older `ninajirachi-agent-handoff-2026-11-17` patch is historical; do not apply it to this revision.
- Read `AGENTS.md`, `README.md`, `GLOSSARY.md`, the relevant ADRs, and the compass bench procedure before firmware changes. Follow the QMC5883P hardware requirements in `AGENTS.md`.
- The ignored Waveshare vendor drivers are not in Git or the bundle. Restore them using `tools/prepare_waveshare.py` and the official demo archive as described in `firmware/README.md`. The previous Windows environment used Arduino CLI 1.6.0 at `%LOCALAPPDATA%\Arduino CLI\arduino-cli.exe`, ESP32 core 2.0.12, and the documented 16 MB partition setting. Other machines must set up their own toolchain.

## Current hardware and objective

- Both physical wearables were last flashed with the compass-axis correction built before commit `63bfaf7`; they retain per-device NVS calibrations. The branch includes that later serial peer-screen timeout change, but it has not been flashed or physically verified. Port assignments COM4 and COM5 were valid on the previous Windows machine only; rediscover them. Preserve calibration: do not erase flash or send `u` without a specific reason.
- Both mounts were reported alike. The axis correction passed controlled flat quarter turns on each wearable; physical tilt and worn behavior are still unverified. The peer-direction marker was wrong in two ground-truth positions, and free-hand motion caused prolonged `NO HEADING`. Details, uncertainties, experiment results, and trace filenames are in `docs/handoff.md`.
- Temporary diagnostic firmware was removed by restoring the normal corrected firmware on COM4. Captures are committed under `docs/handoff/`; no temporary diagnostic instrumentation belongs in the working firmware.
- Before asking the user to move or inspect a particular wearable, ask them to watch both face screens and say ready; then send serial `t` to the intended port and confirm which physical wearable flashed. The reaction is brief; repeat the ready/flash sequence if missed. The serial monitor may need closing to free the port. State directions plainly: the user found “heading” and positional descriptions ambiguous.

## Validation and next steps

- Reproduce the reported `NO HEADING` on the captured free-motion trace, investigate gravity reliability, and compare predicted headings against physical truth. The existing 250 ms gyro-only experiment was not enough; do not claim success solely by allowing a heading through.
- Use the ground-truth radio captures to investigate sparse RSSI buckets, antenna orientation, and multipath before changing confidence rules. A single fixed 180-degree correction is unsupported. The user observed a green marker pointing elsewhere.
- The compass service, calibration, gravity, and behavior host checks passed in the previous session using Windows-compatible tooling; the firmware compiled. The full host suite did not pass in that environment: it encountered Windows compiler portability, symlink privileges, and missing Pillow. Re-run appropriate checks on the new host; hardware verification is separate.

## Suggested skills

- `mattpocock-skills:diagnosing-bugs` for the two remaining observable failures and trace-driven feedback loops.
- `ponytail:ponytail` for a minimal complete fix while preserving real hardware calibration.
