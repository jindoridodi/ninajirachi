#!/bin/sh
set -eu
surface_test_dir=$(mktemp -d)
trap 'rm -f "$surface_test_dir/test" "$surface_test_dir/paint.o"; rmdir "$surface_test_dir"' EXIT
# Install official sources with tools/prepare_waveshare.py before running this test.
c++ -std=c++11 -include tests/waveshare_support/Platform.h -c firmware/src/waveshare/GUI_Paint.cpp -o "$surface_test_dir/paint.o"
c++ -std=c++11 -Wall -Wextra -Werror -include tests/waveshare_support/Platform.h tests/waveshare_surface_test.cpp "$surface_test_dir/paint.o" -o "$surface_test_dir/test"
"$surface_test_dir/test"
