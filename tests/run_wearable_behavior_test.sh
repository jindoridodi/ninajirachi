#!/bin/sh
set -eu

python3 tools/generate_face_presets.py --check
c++ -std=gnu++11 -Wall -Wextra -Werror tests/wearable_behavior_test.cpp -o /tmp/ninajirachi-wearable-behavior-test
/tmp/ninajirachi-wearable-behavior-test
c++ -std=gnu++11 -Wall -Wextra -Werror tests/settings_battery_renderer_test.cpp -o /tmp/ninajirachi-settings-test
/tmp/ninajirachi-settings-test

c++ -std=gnu++11 -Wall -Wextra -Werror tests/nearby_interaction_test.cpp -o /tmp/ninajirachi-nearby-interaction-test
/tmp/ninajirachi-nearby-interaction-test

c++ -std=gnu++11 -Wall -Wextra -Werror tests/heading_history_test.cpp -o /tmp/ninajirachi-heading-history-test
/tmp/ninajirachi-heading-history-test

c++ -std=gnu++11 -Wall -Wextra -Werror tests/sleep_runtime_test.cpp -o /tmp/ninajirachi-sleep-runtime-test
/tmp/ninajirachi-sleep-runtime-test
