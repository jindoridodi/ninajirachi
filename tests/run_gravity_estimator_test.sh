#!/bin/sh
set -eu
gravity_test_dir=$(mktemp -d)
trap 'rm -f "$gravity_test_dir/test"; rmdir "$gravity_test_dir"' EXIT
c++ -std=c++11 -Wall -Wextra -Werror tests/gravity_estimator_test.cpp -o "$gravity_test_dir/test"
"$gravity_test_dir/test"
