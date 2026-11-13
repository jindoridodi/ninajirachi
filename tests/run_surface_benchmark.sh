#!/bin/sh
set -eu
benchmark_dir=$(mktemp -d)
trap 'rm -f "$benchmark_dir/benchmark" "$benchmark_dir/benchmark.exe" "$benchmark_dir/paint.o" "$benchmark_dir/font12.o"; rmdir "$benchmark_dir"' EXIT
# Install the official sources with tools/prepare_waveshare.py first.
c++ -O2 -std=c++11 -include tests/waveshare_support/Platform.h -c firmware/src/waveshare/GUI_Paint.cpp -o "$benchmark_dir/paint.o"
c++ -O2 -std=c++11 -include tests/waveshare_support/Platform.h -c firmware/src/waveshare/font12.cpp -o "$benchmark_dir/font12.o"
c++ -O2 -std=c++11 -Wall -Wextra -Werror -include tests/waveshare_support/Platform.h tests/surface_benchmark.cpp "$benchmark_dir/paint.o" "$benchmark_dir/font12.o" -o "$benchmark_dir/benchmark"
"$benchmark_dir/benchmark"
