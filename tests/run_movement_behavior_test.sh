#!/bin/sh
set -eu

c++ -std=c++17 -Wall -Wextra -Werror -Wno-missing-braces tests/movement_behavior_test.cpp -o /tmp/ninajirachi-movement-behavior-test
/tmp/ninajirachi-movement-behavior-test
