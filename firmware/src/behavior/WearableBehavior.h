#pragma once

#include <stdint.h>

#include "../face/ExpressionPreset.h"

namespace nina {

constexpr uint32_t kFaceSettingsHoldMs = 5000;
constexpr uint32_t kSettingsFaceHoldMs = 3000;
constexpr uint32_t kSettingsInactivityMs = 7000;

enum class ScreenName { Face, SettingsBattery };

struct BatterySnapshot {
  uint16_t millivolts;
  bool charging;
  bool hasEstimatedPercent;
  uint8_t estimatedPercent;
  const char* deviceId;
};

struct SettingsBatteryDisplay {
  BatterySnapshot battery;
};

struct WearableInput {
  uint32_t elapsedMs;
  bool touchDown;
  ExpressionName expression;
  BatterySnapshot battery;
};

struct WearableOutput {
  ScreenName screen;
  ExpressionPreset face;
  SettingsBatteryDisplay settings;
};

class WearableBehavior {
 public:
  WearableOutput update(const WearableInput& input) {
    updateScreen(input.elapsedMs, input.touchDown);
    ExpressionPreset face = expressionPreset(input.expression);
    animate(face, input.elapsedMs);
    return {screen_, face, {input.battery}};
  }

 private:
  void updateScreen(uint32_t elapsedMs, bool touchDown) {
    if (touchDown) {
      lastTouchMs_ = elapsedMs;
      if (!wasTouchDown_) {
        touchStartMs_ = elapsedMs;
        gestureScreen_ = screen_;
      }

      if (!awaitTouchRelease_) {
        const uint32_t heldMs = elapsedMs - touchStartMs_;
        if (gestureScreen_ == ScreenName::Face && heldMs >= kFaceSettingsHoldMs) {
          screen_ = ScreenName::SettingsBattery;
          awaitTouchRelease_ = true;
        } else if (gestureScreen_ == ScreenName::SettingsBattery &&
                   heldMs >= kSettingsFaceHoldMs) {
          screen_ = ScreenName::Face;
          awaitTouchRelease_ = true;
        }
      }
    } else {
      awaitTouchRelease_ = false;
      if (screen_ == ScreenName::SettingsBattery &&
          elapsedMs - lastTouchMs_ >= kSettingsInactivityMs) {
        screen_ = ScreenName::Face;
      }
    }
    wasTouchDown_ = touchDown;
  }

  static void animate(ExpressionPreset& face, uint32_t elapsedMs) {
    const uint32_t phaseMs = elapsedMs % 4000;
    const bool blinking = face.name != ExpressionName::Surprised &&
                          face.name != ExpressionName::Excited && phaseMs >= 3800;
    if (blinking) face.metrics.eyeOpen = 3;

    const int16_t pupilMotion = static_cast<int16_t>((elapsedMs / 350) % 5) - 2;
    face.metrics.pupilOffsetX += pupilMotion;
    if (face.name == ExpressionName::Happy || face.name == ExpressionName::Excited) {
      face.metrics.mouthHeight += static_cast<int16_t>((elapsedMs / 250) % 2);
    }
  }

  ScreenName screen_ = ScreenName::Face;
  ScreenName gestureScreen_ = ScreenName::Face;
  uint32_t touchStartMs_ = 0;
  uint32_t lastTouchMs_ = 0;
  bool wasTouchDown_ = false;
  bool awaitTouchRelease_ = false;
};

}  // namespace nina
