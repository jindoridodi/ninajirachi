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
  const nina::WearableOutput blink = behavior.update({3900, false, nina::ExpressionName::Idle, battery});
  const nina::WearableOutput later = behavior.update({4200, false, nina::ExpressionName::Idle, battery});

  return expect(start.screen == nina::ScreenName::Face, "starts on the face screen") &&
         expect(start.face.name == nina::ExpressionName::Idle, "renders the requested idle preset") &&
         expect(blink.face.metrics.eyeOpen < start.face.metrics.eyeOpen,
                "idle face closes its eyes during a blink") &&
         expect(blink.face.metrics.pupilRadius == 0, "blink hides pupils") &&
         expect(later.face.metrics.eyeOpen == start.face.metrics.eyeOpen,
                "idle face reopens its eyes after a blink") &&
         expect(later.face.metrics.pupilOffsetX != start.face.metrics.pupilOffsetX,
                "idle pupils move over time");
}

bool rendersNamedExpressions() {
  nina::WearableBehavior behavior;
  nina::WearableBehavior happyBehavior, surprisedBehavior, excitedBehavior;
  const nina::BatterySnapshot battery = {3850, false, false, 0, "NINA-1"};

  const nina::WearableOutput curious = behavior.update({1000, false, nina::ExpressionName::Curious, battery});
  const nina::WearableOutput happy = happyBehavior.update({1000, false, nina::ExpressionName::Happy, battery});
  const nina::WearableOutput surprised = surprisedBehavior.update({1000, false, nina::ExpressionName::Surprised, battery});
  const nina::WearableOutput excited = excitedBehavior.update({1000, false, nina::ExpressionName::Excited, battery});

  return expect(curious.face.name == nina::ExpressionName::Curious, "renders curious") &&
         expect(happy.face.name == nina::ExpressionName::Happy, "renders happy") &&
         expect(surprised.face.name == nina::ExpressionName::Surprised, "renders surprised") &&
         expect(excited.face.name == nina::ExpressionName::Excited, "renders excited") &&
         expect(happy.face.metrics.mouthHeight != curious.face.metrics.mouthHeight,
                "expressions have distinct mouths") &&
         expect(excited.face.metrics.effectRadius > 0, "excited has an effect");
}

bool animatesIdleMouth() {
  nina::WearableBehavior behavior;
  nina::BatterySnapshot battery;
  const auto start = behavior.update({0, false, nina::ExpressionName::Idle, battery});
  const auto later = behavior.update({1400, false, nina::ExpressionName::Idle, battery});
  return expect(later.face.metrics.mouthHeight != start.face.metrics.mouthHeight,
                "idle mouth moves gently over time") &&
         expect(later.face.metrics.mouthHeight > 0 &&
                later.face.metrics.mouthHeight <= start.face.metrics.mouthHeight + 2,
                "idle mouth animation stays subtle");
}

bool transitionsFromVisibleBlink() {
  nina::WearableBehavior behavior;
  nina::BatterySnapshot battery;
  const auto blink = behavior.update({3900, false, nina::ExpressionName::Idle, battery});
  const auto change = behavior.update({3900, false, nina::ExpressionName::Surprised, battery});
  if (!expect(blink.face.metrics.eyeOpen == 3 && change.face.metrics.eyeOpen == 3 &&
              change.face.metrics.pupilRadius == blink.face.metrics.pupilRadius &&
              change.face.metrics.pupilOffsetX == blink.face.metrics.pupilOffsetX &&
              change.face.metrics.mouthHeight == blink.face.metrics.mouthHeight,
              "changing expression during blink preserves visible geometry")) return false;
  const auto halfway = behavior.update({4050, false, nina::ExpressionName::Surprised, battery});
  if (!expect(halfway.face.metrics.eyeOpen > 3 && halfway.face.metrics.eyeOpen < 58,
              "blink opens smoothly into surprised eyes")) return false;
  const auto changedAgain = behavior.update({4050, false, nina::ExpressionName::Happy, battery});
  if (!expect(changedAgain.face.metrics.eyeOpen == halfway.face.metrics.eyeOpen,
              "interrupted transition also preserves visible eyes")) return false;
  const auto finished = behavior.update({4350, false, nina::ExpressionName::Happy, battery});
  return expect(finished.face.metrics.eyeOpen == nina::expressionPreset(nina::ExpressionName::Happy).metrics.eyeOpen,
                "interrupted transition reaches its new expression");
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

bool requiresReleaseAndExactTimeouts() {
  nina::WearableBehavior behavior;
  nina::BatterySnapshot battery;
  behavior.update({100, true, nina::ExpressionName::Idle, battery});
  if (!expect(behavior.update({5099, true, nina::ExpressionName::Idle, battery}).screen ==
              nina::ScreenName::Face, "settings requires full five-second hold")) return false;
  behavior.update({5100, true, nina::ExpressionName::Idle, battery});
  if (!expect(behavior.update({15100, true, nina::ExpressionName::Happy, battery}).screen ==
              nina::ScreenName::SettingsBattery, "continuing entry hold cannot exit settings")) return false;
  behavior.update({15200, false, nina::ExpressionName::Happy, battery});
  if (!expect(behavior.update({22199, false, nina::ExpressionName::Happy, battery}).screen ==
              nina::ScreenName::SettingsBattery, "idle timeout starts at touch release")) return false;
  return expect(behavior.update({22200, false, nina::ExpressionName::Happy, battery}).screen ==
                nina::ScreenName::Face, "settings times out at seven seconds");
}

}  // namespace

int main() {
  nina::WearableBehavior transition;
  const nina::BatterySnapshot battery = {3850, false, false, 0, "NINA-1"};
  transition.update({0, false, nina::ExpressionName::Idle, battery});
  const auto before = transition.update({100, false, nina::ExpressionName::Idle, battery});
  const auto changed = transition.update({100, false, nina::ExpressionName::Surprised, battery});
  if (!expect(changed.face.metrics.eyeOpen == before.face.metrics.eyeOpen,
              "expression change starts at the visible face")) return 1;
  const auto halfway = transition.update({250, false, nina::ExpressionName::Surprised, battery});
  if (!expect(halfway.face.metrics.eyeOpen > before.face.metrics.eyeOpen &&
              halfway.face.metrics.eyeOpen < nina::expressionPreset(nina::ExpressionName::Surprised).metrics.eyeOpen,
              "expression passes through intermediate geometry")) return 1;
  const auto finished = transition.update({400, false, nina::ExpressionName::Surprised, battery});
  if (!expect(finished.face.metrics.eyeOpen == 58, "transition reaches requested expression")) return 1;
  return !(animatesIdleMouth() && transitionsFromVisibleBlink() &&
           keepsIdleFaceAlive() && rendersNamedExpressions() &&
           opensAndClosesSettingsWithTheSpecifiedHolds() && returnsToFaceAfterSettingsInactivity() &&
           requiresReleaseAndExactTimeouts());
}
