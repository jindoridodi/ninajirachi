#!/bin/sh
set -eu

python3 tools/generate_face_presets.py --check
c++ -std=c++17 -Wall -Wextra -Werror tests/face_renderer_test.cpp -o /tmp/ninajirachi-face-renderer-test
/tmp/ninajirachi-face-renderer-test
