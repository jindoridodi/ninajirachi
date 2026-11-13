#!/bin/sh
set -eu

python3 tools/generate_face_presets.py --check
python3 tools/face_geometry.py --check
c++ -std=c++17 -Wall -Wextra -Werror tests/face_renderer_test.cpp -o /tmp/ninajirachi-face-renderer-test
/tmp/ninajirachi-face-renderer-test

c++ -std=c++17 -O2 -Wall -Wextra -Werror tests/face_renderer_benchmark.cpp -o /tmp/ninajirachi-face-parity-test
/tmp/ninajirachi-face-parity-test
