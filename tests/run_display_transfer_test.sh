#!/bin/sh
set -eu
display_test_dir=$(mktemp -d)
trap 'rm -f "$display_test_dir/test"; rmdir "$display_test_dir"' EXIT
# Install official sources with tools/prepare_waveshare.py first.
c++ -std=c++11 -I tests/display_support tests/display_transfer_test.cpp \
  firmware/src/waveshare/DEV_Config.cpp firmware/src/waveshare/LCD_1in28.cpp -o "$display_test_dir/test"
"$display_test_dir/test"
python3 tests/prepare_waveshare_test.py
