#include "../firmware/src/behavior/WearableBehavior.h"

#include <cstdio>

namespace {

struct TestInput : nina::WearableInput {
  TestInput(uint32_t time, bool touch, nina::ExpressionName requested, nina::BatterySnapshot power,
            uint64_t identity = 0, nina::DiscoveryObservation observation = {})
      : nina::WearableInput(time, touch, requested, power, identity), discovery(observation) {}
  nina::DiscoveryObservation discovery;
  uint64_t radioPeerId = 0;
  int radioRssi = 0;
};

class TestBehavior : public nina::WearableBehavior {
 public:
  using nina::WearableBehavior::update;
  nina::WearableOutput update(const TestInput& input) {
    const nina::DirectionObservation direction = {input.radioPeerId, input.radioRssi,
                                                   input.headingDegrees, input.elapsedMs};
    return nina::WearableBehavior::update(input, &input.discovery, 1, &direction,
                                          input.headingValid && input.radioPeerId ? 1 : 0);
  }
};

bool expect(bool condition, const char* message) {
  if (condition) return true;
  std::fprintf(stderr, "failed: %s\n", message);
  return false;
}

bool keepsIdleFaceAlive() {
  TestBehavior behavior;
  const nina::BatterySnapshot battery = {3850, false, false, 0, "NINA-1"};

  const nina::WearableOutput start = behavior.update({0, false, nina::ExpressionName::Idle, battery});
  const nina::WearableOutput blink = behavior.update({3900, false, nina::ExpressionName::Idle, battery});
  const nina::WearableOutput later = behavior.update({4200, false, nina::ExpressionName::Idle, battery});

  return expect(start.screen == nina::ScreenName::Face, "starts on the face screen") &&
         expect(start.face.name == nina::ExpressionName::Idle, "renders the requested idle preset") &&
         expect(blink.face.metrics.eyeOpen < start.face.metrics.eyeOpen,
                "idle face closes its eyes during a blink") &&
         expect(later.face.metrics.eyeOpen == start.face.metrics.eyeOpen,
                "idle face reopens its eyes after a blink") &&
         expect(later.face.metrics.eyeOffsetX != start.face.metrics.eyeOffsetX,
                "idle eyes move over time");
}

bool rendersNamedExpressions() {
  TestBehavior behavior;
  TestBehavior excitedBehavior;
  const nina::BatterySnapshot battery = {3850, false, false, 0, "NINA-1"};
  const auto curious = behavior.update({1000, false, nina::ExpressionName::Curious, battery});
  const auto excited = excitedBehavior.update({1000, false, nina::ExpressionName::Excited, battery});
  return expect(curious.face.name == nina::ExpressionName::Curious, "renders curious") &&
         expect(excited.face.name == nina::ExpressionName::Excited, "renders excited") &&
         expect(excited.face.metrics.mouthHeight != curious.face.metrics.mouthHeight,
                "expressions have distinct mouths");
}

bool animatesIdleMouth() {
  TestBehavior behavior;
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
  TestBehavior behavior;
  nina::BatterySnapshot battery;
  const auto blink = behavior.update({3900, false, nina::ExpressionName::Idle, battery});
  const auto change = behavior.update({3900, false, nina::ExpressionName::Excited, battery});
  if (!expect(blink.face.metrics.eyeOpen == 3 && change.face.metrics.eyeOpen == 3 &&
              change.face.metrics.eyeOffsetX == blink.face.metrics.eyeOffsetX &&
              change.face.metrics.mouthHeight == blink.face.metrics.mouthHeight,
              "changing expression during blink preserves visible geometry")) return false;
  const auto halfway = behavior.update({4050, false, nina::ExpressionName::Excited, battery});
  if (!expect(halfway.face.metrics.eyeOpen > 3 && halfway.face.metrics.eyeOpen < 48,
              "blink opens smoothly into excited eyes")) return false;
  const auto changedAgain = behavior.update({4050, false, nina::ExpressionName::Curious, battery});
  if (!expect(changedAgain.face.metrics.eyeOpen == halfway.face.metrics.eyeOpen,
              "interrupted transition also preserves visible eyes")) return false;
  const auto finished = behavior.update({4350, false, nina::ExpressionName::Curious, battery});
  return expect(finished.face.metrics.eyeOpen == nina::expressionPreset(nina::ExpressionName::Curious).metrics.eyeOpen,
                "interrupted transition reaches its new expression");
}

nina::WearableOutput tap(TestBehavior& behavior, uint32_t time, nina::BatterySnapshot battery) {
  behavior.update({time, true, nina::ExpressionName::Idle, battery});
  return behavior.update({time + 50, false, nina::ExpressionName::Idle, battery});
}

bool fiveTapsOpenSettings() {
  TestBehavior behavior;
  nina::BatterySnapshot battery;
  for (int i = 0; i < 4; ++i) tap(behavior, i * 200, battery);
  if (!expect(behavior.update({750, false, nina::ExpressionName::Idle, battery}).screen ==
              nina::ScreenName::Face, "four taps keep the face")) return false;
  return expect(tap(behavior, 800, battery).screen == nina::ScreenName::SettingsBattery,
                "five taps open settings");
}

bool cyclesDebugScreensAndRequestsSleep() {
  TestBehavior behavior;
  nina::BatterySnapshot battery;
  for (int i = 0; i < 5; ++i) tap(behavior, i * 200, battery);
  if (!expect(tap(behavior, 1200, battery).screen == nina::ScreenName::PeerDirection,
              "one tap opens peer direction")) return false;
  if (!expect(tap(behavior, 1400, battery).screen == nina::ScreenName::SettingsBattery,
              "another tap returns to battery")) return false;
  behavior.update({2000, true, nina::ExpressionName::Idle, battery});
  if (!expect(!behavior.update({8999, true, nina::ExpressionName::Idle, battery}).sleepRequested,
              "short hold does not sleep")) return false;
  if (!expect(behavior.update({9000, true, nina::ExpressionName::Idle, battery}).sleepRequested,
              "seven second hold requests sleep")) return false;
  behavior.update({9001, false, nina::ExpressionName::Idle, battery});
  behavior.update({10000, true, nina::ExpressionName::Idle, battery});
  return expect(!behavior.update({16999, true, nina::ExpressionName::Idle, battery}).sleepRequested,
                "a new hold starts its own timer");
}

bool fiveTapsCloseEitherDebugScreen() {
  nina::BatterySnapshot battery;
  for (int start = 0; start < 2; ++start) {
    TestBehavior behavior;
    behavior.showSettings(0);
    if (start == 1) tap(behavior, 100, battery);
    uint32_t base = start ? 1000 : 100;
    for (int i = 0; i < 4; ++i) {
      if (!expect(tap(behavior, base + i * 200, battery).screen != nina::ScreenName::Face,
                  "fewer than five taps cycle debug screens")) return false;
    }
    if (!expect(tap(behavior, base + 800, battery).screen == nina::ScreenName::Face,
                "five taps close either debug screen")) return false;
  }
  return true;
}

bool wakeHoldIsIgnoredUntilReleased() {
  TestBehavior behavior;
  nina::BatterySnapshot battery;
  behavior.ignoreTouchUntilRelease();
  behavior.update({0, true, nina::ExpressionName::Idle, battery});
  if (!expect(!behavior.update({8000, true, nina::ExpressionName::Idle, battery}).sleepRequested,
              "the wake hold cannot immediately sleep again")) return false;
  behavior.update({8100, false, nina::ExpressionName::Idle, battery});
  behavior.update({8200, true, nina::ExpressionName::Idle, battery});
  return expect(behavior.update({15200, true, nina::ExpressionName::Idle, battery}).sleepRequested,
                "a new hold after release can request sleep");
}

bool serialDirectionTimesOut() {
  TestBehavior behavior;
  nina::BatterySnapshot battery;
  behavior.toggleDirection(100);
  if (!expect(behavior.update({7099, false, nina::ExpressionName::Idle, battery}).screen ==
              nina::ScreenName::PeerDirection, "serial peer display stays open before timeout")) return false;
  return expect(behavior.update({7100, false, nina::ExpressionName::Idle, battery}).screen ==
                nina::ScreenName::Face, "serial peer display returns to face after seven seconds");
}

bool opensSettingsFromSerialCommand() {
  TestBehavior behavior;
  nina::BatterySnapshot battery;
  behavior.update({0, false, nina::ExpressionName::Idle, battery});
  behavior.showSettings(1000);
  if (!expect(behavior.update({1000, false, nina::ExpressionName::Idle, battery}).screen ==
              nina::ScreenName::SettingsBattery, "serial command opens settings")) return false;
  return expect(behavior.update({7999, false, nina::ExpressionName::Idle, battery}).screen ==
                    nina::ScreenName::SettingsBattery,
                "serial command starts the settings timeout") &&
         expect(behavior.update({8000, false, nina::ExpressionName::Idle, battery}).screen ==
                    nina::ScreenName::Face,
                "serial-opened settings times out");
}

bool requiresQuickTapsAndExactTimeout() {
  TestBehavior behavior;
  nina::BatterySnapshot battery;
  for (int i = 0; i < 4; ++i) tap(behavior, i * 200, battery);
  if (!expect(tap(behavior, 1400, battery).screen == nina::ScreenName::Face,
              "a long gap resets the tap sequence")) return false;
  for (int i = 0; i < 4; ++i) tap(behavior, 1600 + i * 200, battery);
  if (!expect(behavior.update({9249, false, nina::ExpressionName::Idle, battery}).screen ==
              nina::ScreenName::SettingsBattery, "settings remains open before timeout")) return false;
  return expect(behavior.update({9250, false, nina::ExpressionName::Idle, battery}).screen ==
                nina::ScreenName::Face, "settings times out after seven seconds");
}

bool discoversPeer() {
  TestBehavior behavior;
  nina::BatterySnapshot battery;
  uint64_t packet = 2;
  TestInput input = {100, false, nina::ExpressionName::Idle, battery};
  input.localId = 1;
  input.discovery = {packet, -45, 100};
  const auto found = behavior.update(input);
  input.discovery = {};
  const auto stale = behavior.update({3100, false, nina::ExpressionName::Idle, battery});
  return expect(found.peerCount == 1 && found.targetId == 2 &&
                found.face.name == nina::ExpressionName::Curious, "discovery starts focused interaction") &&
         expect(stale.peerCount == 0 && stale.face.name == nina::ExpressionName::Idle,
                "peer expires after grace period");
}

bool movementOverlaysFocusedInteraction() {
  TestBehavior b;
  nina::BatterySnapshot battery;
  uint64_t peer = 2;
  TestInput in(100, false, nina::ExpressionName::Idle, battery, 1, {peer,-40,100});
  in.motionValid = true; in.verticalAccelerationG = 1.0f;
  b.update(in);
  in.elapsedMs = 500; in.discovery.receivedMs = 500;
  const auto normal = b.update(in);
  in.elapsedMs = 550; in.verticalAccelerationG = 0; b.update(in);
  in.elapsedMs = 600; in.verticalAccelerationG = 2.1f;
  b.update(in);
  in.elapsedMs = 750; in.verticalAccelerationG = 1.0f;
  const auto jump = b.update(in);
  in.elapsedMs = 600 + nina::kDefaultMovementTuning.jumpOverlayDurationMs;
  in.discovery.receivedMs = in.elapsedMs;
  in.verticalAccelerationG = 1.0f;
  const auto resumed = b.update(in);
  return expect(jump.jumpOverlayActive && jump.face.metrics.eyeOpen > normal.face.metrics.eyeOpen,
                "jump overlays focused expression") &&
         expect(!resumed.jumpOverlayActive && resumed.targetId == 2 && resumed.face.name == nina::ExpressionName::Curious,
                "focused interaction resumes after jump");
}

bool tiltAndFocusedAnimation() {
  TestBehavior b;
  nina::BatterySnapshot battery;
  uint64_t peer = 2;
  TestInput in(0, false, nina::ExpressionName::Idle, battery, 1, {peer,-40,0});
  in.motionValid = true; in.verticalAccelerationG = 1;
  const auto start = b.update(in);
  in.elapsedMs = 1400; in.discovery.receivedMs = 1400;
  const auto animated = b.update(in);
  in.rollDegrees = 20; in.elapsedMs = 1800; in.discovery.receivedMs = 1800;
  const auto tilted = b.update(in);
  return expect(animated.face.metrics.mouthHeight != start.face.metrics.mouthHeight,
                "focused mouth animates") &&
         expect(tilted.face.metrics.eyeOffsetX > animated.face.metrics.eyeOffsetX,
                "tilt offsets the focused eyes");
}

bool jumpOverridesBlink() {
  TestBehavior b;
  nina::BatterySnapshot battery;
  TestInput in(3800, false, nina::ExpressionName::Idle, battery);
  in.motionValid = true; in.verticalAccelerationG = 0.4f; b.update(in);
  in.elapsedMs = 3900; in.verticalAccelerationG = 2.1f;
  b.update(in);
  in.elapsedMs = 3950;
  const auto jump = b.update(in);
  return expect(jump.jumpOverlayActive && jump.face.metrics.eyeOpen > 3,
                "jump overrides a closed blink");
}

bool gazeUsesTargetEvidence() {
  TestBehavior behavior;
  nina::BatterySnapshot battery;
  uint64_t peer = 2;
  TestInput in(100, false, nina::ExpressionName::Idle, battery, 1, {peer,-40,100});
  in.radioPeerId = 2; in.radioRssi = -45; in.headingValid = true; in.headingDegrees = 0;
  behavior.update(in);
  in.elapsedMs = 110; behavior.update(in);
  in.elapsedMs = 200; in.radioRssi = -70; in.headingDegrees = 90; behavior.update(in);
  in.elapsedMs = 210; behavior.update(in);
  in.elapsedMs = 300; in.radioRssi = -80; in.headingDegrees = 180; behavior.update(in);
  in.elapsedMs = 310; behavior.update(in);
  in.elapsedMs = 400; in.radioPeerId = 0; in.headingDegrees = 90;
  const auto firstGaze = behavior.update(in);
  if (!expect(firstGaze.gaze == nina::Gaze::Left && firstGaze.relativeDirection == -90,
              "visual direction matches strongest heading")) return false;
  in.elapsedMs = 700;
  const auto settledGaze = behavior.update(in);
  if (!expect(settledGaze.face.metrics.eyeOffsetX < firstGaze.face.metrics.eyeOffsetX,
              "eyes move smoothly toward gaze")) return false;
  in.headingDegrees = 270;
  if (!expect(behavior.update(in).gaze == nina::Gaze::Right, "strongest heading is right")) return false;
  in.headingDegrees = 350;
  if (!expect(behavior.update(in).gaze == nina::Gaze::Center, "angle wraps through north")) return false;
  in.headingDegrees = 90;
  in.headingValid = false; in.elapsedMs = 750;
  const auto unavailable = behavior.update(in);
  if (!expect(unavailable.gaze == nina::Gaze::Center && isnan(unavailable.relativeDirection),
              "unavailable heading hides visual direction")) return false;
  uint64_t other = 3;
  in.elapsedMs = 800; in.headingValid = true; in.discovery = {other,-20,800};
  const auto switched = behavior.update(in);
  return expect(switched.targetId == 3 && switched.gaze == nina::Gaze::Center &&
                isnan(switched.relativeDirection), "target change clears evidence");
}

bool sustainedProximityChoosesExpression() {
  TestBehavior behavior;
  TestInput input(0, false, nina::ExpressionName::Idle, {}, 1, {2, -40, 0});
  for (uint32_t time = 0; time <= 10000; time += 2000) {
    input.elapsedMs = input.discovery.receivedMs = time;
    behavior.update(input);
  }
  if (!expect(behavior.update(input).face.name == nina::ExpressionName::Excited,
              "sustained proximity chooses the excited expression")) return false;
  input.elapsedMs = 13000;
  input.discovery = {};
  return expect(behavior.update(input).face.name == nina::ExpressionName::Idle,
                "peer expiry restores the manual expression");
}

bool discoveryContinuesInSettingsAndAcrossClockWrap() {
  TestBehavior b;
  nina::BatterySnapshot battery;
  uint64_t p = 2;
  for (int i = 0; i < 5; ++i) tap(b, i * 200, battery);
  TestInput in(1900, false, nina::ExpressionName::Idle, battery, 1, {p, -50, 1900});
  const auto settings = b.update(in);
  if (!expect(settings.screen == nina::ScreenName::SettingsBattery && settings.peerCount == 1,
              "discovery remains active on settings screen")) return false;
  TestBehavior wrapped;
  in.touchDown = false;
  in.elapsedMs = 0xfffffff0u; in.discovery.receivedMs = in.elapsedMs;
  wrapped.update(in);
  in.discovery = {};
  in.elapsedMs = 100;
  if (!expect(wrapped.update(in).peerCount == 1, "peer remains recent across clock wrap")) return false;
  in.elapsedMs = 3000;
  return expect(wrapped.update(in).peerCount == 0, "peer expires across clock wrap");
}

}  // namespace

int main() {
  {
    TestBehavior manual;
    TestInput input(100, false, nina::ExpressionName::Idle, {});
    manual.triggerJump(100);
    manual.update(input);
    input.elapsedMs = 500;
    const auto jump = manual.update(input);
    if (!expect(jump.jumpOverlayActive && jump.face.metrics.tongueOut == 12,
                "manual jump shows the same transient reaction")) return 1;
    input.elapsedMs = 2100;
    if (!expect(!manual.update(input).jumpOverlayActive,
                "manual jump expires normally")) return 1;
  }

  {
    TestBehavior batch;
    uint64_t peer = 2;
    nina::DiscoveryObservation discovery = {peer, -40, 100};
    TestInput in(100, false, nina::ExpressionName::Idle, {}, 1);
    in.headingValid = true;
    in.headingDegrees = 90;
    const nina::DirectionObservation directions[] = {
      {2, -40, 0, 100}, {2, -40, 0, 100}, {2, -65, 90, 100},
      {2, -65, 90, 100}, {2, -80, 180, 100}, {2, -80, 180, 100}};
    const auto output = batch.update(in, &discovery, 1, directions, 6);
    if (!expect(output.targetId == 2 && output.gaze == nina::Gaze::Left,
                "one frame ingests every direction observation")) return 1;
    uint64_t other = 3;
    nina::DiscoveryObservation replacement = {other, -20, 101};
    in.elapsedMs = 101;
    const auto switched = batch.update(in, &replacement, 1, directions, 6);
    if (!expect(switched.targetId == 3 && switched.gaze == nina::Gaze::Center,
                "target switch discards old batched direction evidence")) return 1;
    in.headingValid = false;
    if (!expect(batch.update(in, nullptr, 0, directions, 0).gaze == nina::Gaze::Center,
                "missing heading centers gaze after a batched frame")) return 1;
  }

  TestBehavior staleAcceleration;
  TestInput stale(100, false, nina::ExpressionName::Idle, {});
  stale.accelerationMagnitudeG = 0.4f;
  staleAcceleration.update(stale);
  stale.elapsedMs = 150;
  stale.accelerationValid = true;
  stale.accelerationMagnitudeG = 3.0f;
  if (!expect(!staleAcceleration.update(stale).jumpOverlayActive,
              "stale acceleration and an isolated impact do not trigger a jump")) return 1;
  TestBehavior walking;
  TestInput step(50114, false, nina::ExpressionName::Idle, {});
  step.accelerationValid = true;
  step.accelerationMagnitudeG = 0.449f;
  walking.update(step);
  step.elapsedMs = 50301;
  step.accelerationMagnitudeG = 3.699f;
  if (!expect(!walking.update(step).jumpOverlayActive,
              "recorded walking does not trigger a jump without gravity")) return 1;
  TestBehavior missingGravity;
  TestInput sample(17640, false, nina::ExpressionName::Idle, {});
  sample.accelerationValid = true;
  sample.accelerationMagnitudeG = 0.198f;
  missingGravity.update(sample);
  sample.elapsedMs = 17760;
  sample.accelerationMagnitudeG = 4.067f;
  if (!expect(missingGravity.update(sample).jumpOverlayActive,
              "fresh low acceleration followed by impact reacts without a gravity estimate")) return 1;
  if (!wakeHoldIsIgnoredUntilReleased()) return 1;
  if (!fiveTapsCloseEitherDebugScreen()) return 1;
  if (!serialDirectionTimesOut()) return 1;
  if (!fiveTapsOpenSettings()) return 1;
  if (!jumpOverridesBlink()) return 1;
  if (!tiltAndFocusedAnimation()) return 1;
  if (!movementOverlaysFocusedInteraction()) return 1;
  TestBehavior transition;
  const nina::BatterySnapshot battery = {3850, false, false, 0, "NINA-1"};
  transition.update({0, false, nina::ExpressionName::Idle, battery});
  const auto before = transition.update({100, false, nina::ExpressionName::Idle, battery});
  const auto changed = transition.update({100, false, nina::ExpressionName::Excited, battery});
  if (!expect(changed.face.metrics.eyeOpen == before.face.metrics.eyeOpen,
              "expression change starts at the visible face")) return 1;
  const auto halfway = transition.update({250, false, nina::ExpressionName::Excited, battery});
  if (!expect(halfway.face.metrics.eyeOpen > before.face.metrics.eyeOpen &&
              halfway.face.metrics.eyeOpen < nina::expressionPreset(nina::ExpressionName::Excited).metrics.eyeOpen,
              "expression passes through intermediate geometry")) return 1;
  const auto finished = transition.update({400, false, nina::ExpressionName::Excited, battery});
  if (!expect(finished.face.metrics.eyeOpen == 48, "transition reaches requested expression")) return 1;
  return !(sustainedProximityChoosesExpression() && gazeUsesTargetEvidence() && discoveryContinuesInSettingsAndAcrossClockWrap() && discoversPeer() && animatesIdleMouth() && transitionsFromVisibleBlink() &&
           keepsIdleFaceAlive() && rendersNamedExpressions() &&
           cyclesDebugScreensAndRequestsSleep() && opensSettingsFromSerialCommand() && requiresQuickTapsAndExactTimeout());
}
