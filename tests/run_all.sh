#!/bin/sh
set -eu
cd "$(dirname "$0")/.."

for check in tests/run_face_renderer_test.sh tests/run_waveshare_surface_test.sh \
  tests/run_display_transfer_test.sh tests/run_wearable_behavior_test.sh tests/run_movement_behavior_test.sh \
  tests/run_compass_calibration_test.sh tests/run_compass_service_test.sh \
  tests/run_gravity_estimator_test.sh tests/run_motion_sampler_test.sh \
  tests/run_discovery_test.sh; do
  printf 'Running %s\n' "$check"
  sh "$check"
done

python3 tests/preview_editor_test.py
python3 tests/face_geometry_test.py
