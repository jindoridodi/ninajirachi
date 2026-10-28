#!/bin/sh
set -eu

python3 tools/generate_face_presets.py --check
c++ -std=gnu++11 -Wall -Wextra -Werror tests/wearable_behavior_test.cpp -o /tmp/ninajirachi-wearable-behavior-test
/tmp/ninajirachi-wearable-behavior-test
c++ -std=gnu++11 -Wall -Wextra -Werror tests/settings_battery_renderer_test.cpp -o /tmp/ninajirachi-settings-test
/tmp/ninajirachi-settings-test
