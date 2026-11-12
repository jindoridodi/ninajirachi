#!/bin/sh
set -eu
radio_test_dir=$(mktemp -d)
trap 'rm -f "$radio_test_dir/test"; rmdir "$radio_test_dir"' EXIT
c++ -std=c++11 -Wall -Wextra -Werror -I tests/radio_support tests/discovery_test.cpp -o "$radio_test_dir/test"
"$radio_test_dir/test"
