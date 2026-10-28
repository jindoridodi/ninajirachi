#!/bin/sh
set -eu

c++ -std=c++17 -Wall -Wextra -Werror tests/compass_calibration_test.cpp -o /tmp/ninajirachi-compass-calibration-test
/tmp/ninajirachi-compass-calibration-test
