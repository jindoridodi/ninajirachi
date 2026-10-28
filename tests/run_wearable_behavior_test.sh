#!/bin/sh
set -eu

python3 tools/generate_face_presets.py --check
c++ -std=c++17 -Wall -Wextra -Werror tests/wearable_behavior_test.cpp -o /tmp/ninajirachi-wearable-behavior-test
/tmp/ninajirachi-wearable-behavior-test
