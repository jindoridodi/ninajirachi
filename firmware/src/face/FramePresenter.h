#pragma once

#include "SettingsBatteryRenderer.h"
#include "../behavior/WearableBehavior.h"
#include <math.h>
#include <string.h>

namespace nina {

class FramePresenter {
 public:
  template <typename Display>
  bool present(Display& display, const WearableOutput& output, bool headingValid,
               const char* calibrationLabel) {
    const bool screenChanged = !valid_ || screen_ != output.screen;
    bool changed = screenChanged;
    if (output.screen == ScreenName::Face) {
      changed = changed || !sameFace(face_, output.face);
      face_ = output.face;
      if (changed) display.drawFace(output.face);
    } else if (output.screen == ScreenName::SettingsBattery) {
      if (screenChanged) settings_.invalidate();
      const auto lines = settingsBatteryLines(output.battery);
      changed = settings_.changed(lines) || changed;
      if (changed) display.drawSettings(lines);
    } else {
      const bool directionValid = isfinite(output.relativeDirection);
      changed = changed || directionValid != directionValid_ ||
          (directionValid && output.relativeDirection != direction_) ||
          (output.targetId != 0) != hasTarget_ || headingValid != headingValid_ ||
          !calibrationLabel_ || strcmp(calibrationLabel_, calibrationLabel) != 0;
      directionValid_ = directionValid;
      direction_ = output.relativeDirection;
      hasTarget_ = output.targetId != 0;
      headingValid_ = headingValid;
      calibrationLabel_ = calibrationLabel;
      if (changed) display.drawDirection(output, headingValid, calibrationLabel);
    }
    screen_ = output.screen;
    valid_ = true;
    if (!changed) return false;
    display.present();
    return true;
  }

 private:
  static bool sameFace(const ExpressionPreset& a, const ExpressionPreset& b) {
    return a.colors.background.rgb == b.colors.background.rgb &&
           a.colors.face.rgb == b.colors.face.rgb && a.colors.eye.rgb == b.colors.eye.rgb &&
           a.colors.mouth.rgb == b.colors.mouth.rgb &&
           a.metrics.eyeWidth == b.metrics.eyeWidth && a.metrics.eyeOpen == b.metrics.eyeOpen &&
           a.metrics.eyeOffsetX == b.metrics.eyeOffsetX && a.metrics.eyeOffsetY == b.metrics.eyeOffsetY &&
           a.metrics.mouthWidth == b.metrics.mouthWidth && a.metrics.mouthHeight == b.metrics.mouthHeight &&
           a.metrics.tongueOut == b.metrics.tongueOut;
  }
  SettingsBatteryRefresh settings_;
  ExpressionPreset face_{};
  ScreenName screen_ = ScreenName::Face;
  const char* calibrationLabel_ = nullptr;
  float direction_ = 0;
  bool valid_ = false, directionValid_ = false, hasTarget_ = false, headingValid_ = false;
};

}
