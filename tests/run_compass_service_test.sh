#!/bin/sh
set -eu
compass_test_dir=$(mktemp -d)
trap 'rm -f "$compass_test_dir/test"; rmdir "$compass_test_dir"' EXIT
c++ -std=c++11 -Wall -Wextra -Werror -I tests/compass_support tests/compass_service_test.cpp -o "$compass_test_dir/test"
"$compass_test_dir/test"
