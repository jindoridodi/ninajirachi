#!/bin/sh
set -eu
motion_test_dir=$(mktemp -d)
trap 'rm -f "$motion_test_dir/test"; rmdir "$motion_test_dir"' EXIT
c++ -std=c++11 -Wall -Wextra -Werror -I tests/compass_support tests/motion_sampler_test.cpp -o "$motion_test_dir/test"
"$motion_test_dir/test"
