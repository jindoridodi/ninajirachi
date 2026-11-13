#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
trace=${1:-tests/fixtures/2026-11-12-normal-turn.txt}
replay_dir=$(mktemp -d)
trap 'rm -f "$replay_dir/replay"; rmdir "$replay_dir"' EXIT
c++ -std=c++11 -Wall -Wextra -Werror tools/replay_gravity_trace.cpp -o "$replay_dir/replay"
"$replay_dir/replay" < "$trace"
