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

bool batchedDelayedContinuity(bool otherPeerFirst, uint32_t start) {
  nina::WearableBehavior b;
  nina::BatterySnapshot battery;
  const uint8_t p[] = {'N','I','N','A',1,1,0,0,0,0,0,2};
  const uint8_t other[] = {'N','I','N','A',1,1,0,0,0,0,0,3};
  nina::WearableInput in(start, false, nina::ExpressionName::Idle, battery, 1, {p, 12, -40, start});
  b.update(in);
  const nina::DiscoveryObservation batch[] = {
      {otherPeerFirst ? other : p, 12, otherPeerFirst ? -80 : -40, start + (otherPeerFirst ? 3000u : 100u)},
      {p, 12, -40, start + 2900}};
  in.elapsedMs = start + 3100; in.discovery = {};
  const auto result = b.update(in, batch, 2);
  if (!expect(result.targetId == 2 && result.peerCount == (otherPeerFirst ? 2u : 1u),
              "batch retains target and ignores stale packet")) return false;
  in.discovery = {p, 12, -40, start + 5800}; in.elapsedMs = start + 5800; b.update(in);
  in.discovery.receivedMs = start + 8700; in.elapsedMs = start + 8700; b.update(in);
  in.discovery.receivedMs = start + 10000; in.elapsedMs = start + 10000;
  return expect(b.update(in).face.name == nina::ExpressionName::Excited,
                "batch preserves uninterrupted sustained proximity");
}

bool batchDoesNotReviveStalePeers() {
  nina::WearableBehavior b;
  nina::BatterySnapshot battery;
  const uint8_t p[] = {'N','I','N','A',1,1,0,0,0,0,0,2};
  nina::WearableInput in(0, false, nina::ExpressionName::Idle, battery, 1, {p, 12, -40, 0});
  b.update(in);
  const nina::DiscoveryObservation batch[] = {{p, 12, -40, 100}, {p, 12, -40, 200}};
  in.elapsedMs = 3200; in.discovery = {};
  const auto result = b.update(in, batch, 2);
  return expect(result.peerCount == 0 && result.targetId == 0 && result.face.name == nina::ExpressionName::Idle,
                "stale batch cannot revive an expired peer");
}

bool batchKeepsNewestSignal() {
  nina::WearableBehavior b;
  nina::BatterySnapshot battery;
  const uint8_t p[] = {'N','I','N','A',1,1,0,0,0,0,0,2};
  const uint8_t other[] = {'N','I','N','A',1,1,0,0,0,0,0,3};
  nina::WearableInput in(100, false, nina::ExpressionName::Idle, battery, 1, {p, 12, -40, 90});
  b.update(in);
  const nina::DiscoveryObservation batch[] = {{p, 12, -40, 200}, {p, 12, -90, 150},
                                                {other, 12, -50, 200}};
  in.elapsedMs = 200; in.discovery = {};
  return expect(b.update(in, batch, 3).targetId == 2,
                "older packet later in batch cannot replace newer signal");
}

bool batchDoesNotBridgeAGapOutOfOrder() {
  nina::WearableBehavior b;
  nina::BatterySnapshot battery;
  const uint8_t p[] = {'N','I','N','A',1,1,0,0,0,0,0,2};
  nina::WearableInput in(0, false, nina::ExpressionName::Idle, battery, 1, {p, 12, -40, 0});
  b.update(in);
  const nina::DiscoveryObservation batch[] = {{p, 12, -40, 3100}, {p, 12, -40, 2900}};
  in.elapsedMs = 3200; in.discovery = {}; b.update(in, batch, 2);
  in.elapsedMs = 6000; in.discovery = {p, 12, -40, 6000}; b.update(in);
  in.elapsedMs = 8000; in.discovery.receivedMs = 8000; b.update(in);
  in.elapsedMs = 10000; in.discovery.receivedMs = 10000;
  return expect(b.update(in).face.name == nina::ExpressionName::Curious,
                "older packet cannot undo a genuine gap in a batch");
}

bool delayedContinuousProximity(uint32_t start) {
  nina::WearableBehavior b;
  nina::BatterySnapshot battery;
  const uint8_t p[] = {'N','I','N','A',1,1,0,0,0,0,0,2};
  nina::WearableInput in(start, false, nina::ExpressionName::Idle, battery, 1, {p, 12, -40, start});
  b.update(in);
  in.elapsedMs = start + 3100; in.discovery.receivedMs = start + 2900; b.update(in);
  in.elapsedMs = start + 5800; in.discovery.receivedMs = start + 5800; b.update(in);
  in.elapsedMs = start + 8700; in.discovery.receivedMs = start + 8700; b.update(in);
  in.elapsedMs = start + 10000; in.discovery.receivedMs = start + 10000;
  if (!expect(b.update(in).face.name == nina::ExpressionName::Excited,
              "delayed observation within grace preserves sustained proximity")) return false;
  in.elapsedMs = start + 13100; in.discovery.receivedMs = start + 13000;
  return expect(b.update(in).face.name == nina::ExpressionName::Curious,
                "a full grace period between observations resets sustained proximity");
}

bool delayedObservationsExpireFromReceiveTime() {
  nina::WearableBehavior b;
  nina::BatterySnapshot battery;
  const uint8_t p[] = {'N','I','N','A',1,1,0,0,0,0,0,2};
  nina::WearableInput in(3000, false, nina::ExpressionName::Idle, battery, 1, {p, 12, -40, 0});
  if (!expect(b.update(in).peerCount == 0, "stale queued observation is ignored")) return false;
  in.elapsedMs = 4000; in.discovery.receivedMs = 2000;
  if (!expect(b.update(in).peerCount == 1, "recent queued observation is accepted")) return false;
  in.discovery = {}; in.elapsedMs = 5000;
  return expect(b.update(in).peerCount == 0, "peer expires from receive time");
}

bool olderObservationsDoNotReplaceNewerOnes() {
  nina::WearableBehavior b;
  nina::BatterySnapshot battery;
  uint8_t p[] = {'N','I','N','A',1,1,0,0,0,0,0,2};
  nina::WearableInput in(100, false, nina::ExpressionName::Idle, battery, 1, {p, 12, -40, 90});
  b.update(in);
  in.discovery.rssi = -90; in.discovery.receivedMs = 0xfffffff0u;
  b.update(in);
  p[11] = 3; in.discovery.rssi = -50; in.discovery.receivedMs = 100;
  if (!expect(b.update(in).targetId == 2, "older pre-wrap RSSI cannot replace newer RSSI")) return false;
  in.discovery = {}; in.elapsedMs = 3000;
  if (!expect(b.update(in).targetId == 2, "older pre-wrap timestamp cannot shorten peer lifetime")) return false;
  in.elapsedMs = 3090;
  return expect(b.update(in).targetId == 3, "newer post-wrap observation expires at its deadline");
}

bool discoversPeer() {
  nina::WearableBehavior behavior;
  nina::BatterySnapshot battery;
  const uint8_t packet[] = {'N','I','N','A',1,1,0,0,0,0,0,2};
  nina::WearableInput input = {100, false, nina::ExpressionName::Idle, battery};
  input.localId = 1;
  input.discovery = {packet, sizeof(packet), -45, 100};
  const auto found = behavior.update(input);
  input.discovery = {};
  const auto stale = behavior.update({3100, false, nina::ExpressionName::Idle, battery});
  return expect(found.peerCount == 1 && found.targetId == 2 &&
                found.face.name == nina::ExpressionName::Curious, "discovery starts focused interaction") &&
         expect(stale.peerCount == 0 && stale.face.name == nina::ExpressionName::Idle,
                "peer expires after grace period");
}

bool selectsStableTargetsAndIgnoresInvalidPackets() {
  nina::WearableBehavior b;
  nina::BatterySnapshot battery;
  uint8_t p[] = {'N','I','N','A',1,1,0,0,0,0,0,1};
  nina::WearableInput in(0, false, nina::ExpressionName::Idle, battery, 1, {p, 12, -50, 0});
  if (!expect(b.update(in).peerCount == 0, "own identity ignored")) return false;
  p[11] = 2; p[4] = 2;
  if (!expect(b.update(in).peerCount == 0, "unknown protocol ignored")) return false;
  p[4] = 1; in.discovery.size = 11;
  if (!expect(b.update(in).peerCount == 0, "truncated protocol ignored")) return false;
  in.discovery.size = 12;
  if (!expect(b.update(in).targetId == 2, "first peer selected")) return false;
  p[11] = 3; in.elapsedMs = 100; in.discovery.receivedMs = 100; in.discovery.rssi = -47;
  if (!expect(b.update(in).targetId == 2, "small RSSI difference preserves target")) return false;
  in.elapsedMs = 200; in.discovery.receivedMs = 200; in.discovery.rssi = -44;
  if (!expect(b.update(in).targetId == 3, "clearly stronger peer selected")) return false;
  p[11] = 2; in.elapsedMs = 3000; in.discovery.receivedMs = 3000; in.discovery.rssi = -50;
  if (!expect(b.update(in).targetId == 3, "selected peer remains while recent")) return false;
  in.elapsedMs = 3200; in.discovery = {};
  return expect(b.update(in).targetId == 2, "expired target replaced by recent peer");
}

bool sustainedProximityAndCapacity() {
  nina::WearableBehavior b;
  nina::BatterySnapshot battery;
  uint8_t p[] = {'N','I','N','A',1,1,0,0,0,0,0,2};
  nina::WearableInput in(0, false, nina::ExpressionName::Idle, battery, 1, {p, 12, -40, 0});
  b.update(in);
  for (int t = 2000; t <= 10000; t += 2000) {
    in.elapsedMs = t; in.discovery.receivedMs = t;
    const auto output = b.update(in);
    if (t == 8000 && !expect(output.face.name == nina::ExpressionName::Curious,
                             "focused until sustained proximity")) return false;
    if (t == 10000 && !expect(output.face.name == nina::ExpressionName::Excited,
                              "sustained peer becomes excited")) return false;
  }
  for (int id = 3; id <= 6; ++id) { p[11] = id; b.update(in); }
  if (!expect(b.update(in).peerCount == 4, "peer list has bounded capacity")) return false;
  in.discovery = {};
  in.elapsedMs = 13000; in.discovery.receivedMs = 13000;
  return expect(b.update(in).face.name == nina::ExpressionName::Idle, "expiry returns to idle");
}

bool returningPeerStartsFocusedAgain() {
  nina::WearableBehavior b;
  nina::BatterySnapshot battery;
  const uint8_t p[] = {'N','I','N','A',1,1,0,0,0,0,0,2};
  nina::WearableInput in(0, false, nina::ExpressionName::Idle, battery, 1, {p, 12, -40, 0});
  for (int t = 0; t <= 10000; t += 2000) { in.elapsedMs = t; in.discovery.receivedMs = t; b.update(in); }
  in.elapsedMs = 13000; in.discovery.receivedMs = 13000;
  return expect(b.update(in).face.name == nina::ExpressionName::Curious,
                "peer returning after expiry starts focused again");
}

bool discoveryContinuesInSettingsAndAcrossClockWrap() {
  nina::WearableBehavior b;
  nina::BatterySnapshot battery;
  const uint8_t p[] = {'N','I','N','A',1,1,0,0,0,0,0,2};
  nina::WearableInput in(0, true, nina::ExpressionName::Idle, battery, 1, {p, 12, -50, 0});
  b.update(in);
  in.elapsedMs = 5000; in.discovery.receivedMs = 5000;
  const auto settings = b.update(in);
  if (!expect(settings.screen == nina::ScreenName::SettingsBattery && settings.peerCount == 1,
              "discovery remains active on settings screen")) return false;
  nina::WearableBehavior wrapped;
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
  return !(batchDoesNotBridgeAGapOutOfOrder() && batchKeepsNewestSignal() && batchDoesNotReviveStalePeers() && batchedDelayedContinuity(false, 0) && batchedDelayedContinuity(true, 0) && batchedDelayedContinuity(false, 0xfffff800u) && delayedContinuousProximity(0) && delayedContinuousProximity(0xfffff800u) && olderObservationsDoNotReplaceNewerOnes() && delayedObservationsExpireFromReceiveTime() && discoveryContinuesInSettingsAndAcrossClockWrap() && returningPeerStartsFocusedAgain() && sustainedProximityAndCapacity() && selectsStableTargetsAndIgnoresInvalidPackets() && discoversPeer() && animatesIdleMouth() && transitionsFromVisibleBlink() &&
           keepsIdleFaceAlive() && rendersNamedExpressions() &&
           opensAndClosesSettingsWithTheSpecifiedHolds() && returnsToFaceAfterSettingsInactivity() &&
           requiresReleaseAndExactTimeouts());
}
