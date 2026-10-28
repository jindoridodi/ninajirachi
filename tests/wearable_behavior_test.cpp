#include "../firmware/src/behavior/WearableBehavior.h"

#include <cstdio>

namespace {

bool expect(bool condition, const char* message) {
  if (condition) return true;
  std::fprintf(stderr, "failed: %s\n", message);
  return false;
}

bool keepsIdleFaceAlive() {
  nina::WearableBehavior behavior;
  const nina::BatterySnapshot battery = {3850, false, false, 0, "NINA-1"};

  const nina::WearableOutput start = behavior.update({0, false, nina::ExpressionName::Idle, battery});
  const nina::WearableOutput blink = behavior.update({4000, false, nina::ExpressionName::Idle, battery});
  const nina::WearableOutput later = behavior.update({4200, false, nina::ExpressionName::Idle, battery});

  return expect(start.screen == nina::ScreenName::Face, "starts on the face screen") &&
         expect(start.face.name == nina::ExpressionName::Idle, "renders the requested idle preset") &&
         expect(blink.face.metrics.eyeOpen < start.face.metrics.eyeOpen,
                "idle face closes its eyes during a blink") &&
         expect(later.face.metrics.eyeOpen == start.face.metrics.eyeOpen,
                "idle face reopens its eyes after a blink") &&
         expect(later.face.metrics.pupilOffsetX != start.face.metrics.pupilOffsetX,
                "idle pupils move over time");
}

bool rendersNamedExpressions() {
  nina::WearableBehavior behavior;
  const nina::BatterySnapshot battery = {3850, false, false, 0, "NINA-1"};

  const nina::WearableOutput curious = behavior.update({1000, false, nina::ExpressionName::Curious, battery});
  const nina::WearableOutput happy = behavior.update({1000, false, nina::ExpressionName::Happy, battery});
  const nina::WearableOutput surprised = behavior.update({1000, false, nina::ExpressionName::Surprised, battery});
  const nina::WearableOutput excited = behavior.update({1000, false, nina::ExpressionName::Excited, battery});

  return expect(curious.face.name == nina::ExpressionName::Curious, "renders curious") &&
         expect(happy.face.name == nina::ExpressionName::Happy, "renders happy") &&
         expect(surprised.face.name == nina::ExpressionName::Surprised, "renders surprised") &&
         expect(excited.face.name == nina::ExpressionName::Excited, "renders excited") &&
         expect(happy.face.metrics.mouthHeight != curious.face.metrics.mouthHeight,
                "expressions have distinct mouths") &&
         expect(excited.face.metrics.effectRadius > 0, "excited has an effect");
}

bool opensAndClosesSettingsWithTheSpecifiedHolds() {
  nina::WearableBehavior behavior;
  const nina::BatterySnapshot battery = {3890, true, true, 72, "NINA-42"};

  behavior.update({0, true, nina::ExpressionName::Idle, battery});
  const nina::WearableOutput settings = behavior.update({5000, true, nina::ExpressionName::Idle, battery});
  behavior.update({5100, false, nina::ExpressionName::Idle, battery});
  behavior.update({6000, true, nina::ExpressionName::Idle, battery});
  const nina::WearableOutput face = behavior.update({9000, true, nina::ExpressionName::Idle, battery});

  return expect(settings.screen == nina::ScreenName::SettingsBattery,
                "five-second face hold opens settings") &&
         expect(settings.settings.battery.millivolts == 3890, "settings exposes battery voltage") &&
         expect(settings.settings.battery.charging, "settings exposes charging state") &&
         expect(settings.settings.battery.hasEstimatedPercent &&
                    settings.settings.battery.estimatedPercent == 72,
                "settings exposes only calibrated percentage") &&
         expect(settings.settings.battery.deviceId == battery.deviceId,
                "settings exposes the device ID") &&
         expect(face.screen == nina::ScreenName::Face, "three-second settings hold returns to face");
}

bool returnsToFaceAfterSettingsInactivity() {
  nina::WearableBehavior behavior;
  const nina::BatterySnapshot battery = {3700, false, false, 0, "NINA-2"};

  behavior.update({0, true, nina::ExpressionName::Idle, battery});
  behavior.update({5000, true, nina::ExpressionName::Idle, battery});
  behavior.update({5100, false, nina::ExpressionName::Idle, battery});
  const nina::WearableOutput face = behavior.update({12100, false, nina::ExpressionName::Idle, battery});

  return expect(face.screen == nina::ScreenName::Face,
                "seven seconds without touch returns settings to face");
}

}  // namespace

int main() {
  return !(keepsIdleFaceAlive() && rendersNamedExpressions() &&
           opensAndClosesSettingsWithTheSpecifiedHolds() && returnsToFaceAfterSettingsInactivity());
}
