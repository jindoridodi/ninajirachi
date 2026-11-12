#include "../firmware/src/behavior/NearbyInteraction.h"
#include <cstdio>

namespace {

struct TestInput {
  TestInput(uint32_t time, nina::DiscoveryObservation observation = {})
      : elapsedMs(time), discovery(observation) {}
  uint32_t elapsedMs;
  uint64_t localId = 1;
  nina::DiscoveryObservation discovery;
  uint64_t radioPeerId = 0;
  int radioRssi = 0;
  float headingDegrees = 0;
  bool headingValid = false;
};

class TestInteraction : public nina::NearbyInteraction {
 public:
  nina::NearbyInteractionState update(const TestInput& input) {
    const nina::DirectionObservation direction = {input.radioPeerId, input.radioRssi,
                                                   input.headingDegrees, input.elapsedMs};
    return nina::NearbyInteraction::update(input.elapsedMs, input.localId, &input.discovery, 1,
        input.headingValid, input.headingDegrees, &direction,
        input.headingValid && input.radioPeerId ? 1 : 0);
  }

  nina::NearbyInteractionState update(const TestInput& input,
                                     const nina::DiscoveryObservation* observations, size_t count) {
    return nina::NearbyInteraction::update(input.elapsedMs, input.localId, observations, count,
                                          input.headingValid, input.headingDegrees);
  }
};

bool expect(bool condition, const char* message) {
  if (condition) return true;
  std::fprintf(stderr, "failed: %s\n", message);
  return false;
}

bool multiHopQueuedContinuity(uint32_t start) {
  TestInteraction b;
  uint64_t p = 2;
  TestInput in(start, {p, -40, start});
  b.update(in);
  const nina::DiscoveryObservation batch[] = {{p, -40, start + 2900},
                                                {p, -40, start + 5800}};
  in.elapsedMs = start + 6000; in.discovery = {}; b.update(in, batch, 2);
  in.elapsedMs = start + 8700; in.discovery = {p, -40, start + 8700}; b.update(in);
  in.elapsedMs = start + 10000; in.discovery.receivedMs = start + 10000;
  return expect(b.update(in).sustainedProximity,
                "stale intermediate observation bridges a continuous queued chain");
}

bool multiHopGapResetsContinuity() {
  TestInteraction b;
  uint64_t p = 2;
  TestInput in(0, {p, -40, 0});
  b.update(in);
  const nina::DiscoveryObservation batch[] = {{p, -40, 2900}, {p, -40, 5900}};
  in.elapsedMs = 6000; in.discovery = {}; b.update(in, batch, 2);
  in.elapsedMs = 8700; in.discovery = {p, -40, 8700}; b.update(in);
  in.elapsedMs = 10000; in.discovery.receivedMs = 10000;
  return expect(!b.update(in).sustainedProximity,
                "a grace-period gap within a queued chain resets continuity");
}

bool batchedDelayedContinuity(bool otherPeerFirst, uint32_t start) {
  TestInteraction b;
  uint64_t p = 2;
  uint64_t other = 3;
  TestInput in(start, {p, -40, start});
  b.update(in);
  const nina::DiscoveryObservation batch[] = {
      {otherPeerFirst ? other : p, otherPeerFirst ? -80 : -40, start + (otherPeerFirst ? 3000u : 100u)},
      {p, -40, start + 2900}};
  in.elapsedMs = start + 3100; in.discovery = {};
  const auto result = b.update(in, batch, 2);
  if (!expect(result.targetId == 2 && result.peerCount == (otherPeerFirst ? 2u : 1u),
              "batch retains target and ignores stale packet")) return false;
  in.discovery = {p, -40, start + 5800}; in.elapsedMs = start + 5800; b.update(in);
  in.discovery.receivedMs = start + 8700; in.elapsedMs = start + 8700; b.update(in);
  in.discovery.receivedMs = start + 10000; in.elapsedMs = start + 10000;
  return expect(b.update(in).sustainedProximity,
                "batch preserves uninterrupted sustained proximity");
}

bool batchDoesNotReviveStalePeers() {
  TestInteraction b;
  uint64_t p = 2;
  TestInput in(0, {p, -40, 0});
  b.update(in);
  const nina::DiscoveryObservation batch[] = {{p, -40, 100}, {p, -40, 200}};
  in.elapsedMs = 3200; in.discovery = {};
  const auto result = b.update(in, batch, 2);
  return expect(result.peerCount == 0 && result.targetId == 0,
                "stale batch cannot revive an expired peer");
}

bool batchKeepsNewestSignal() {
  TestInteraction b;
  uint64_t p = 2;
  uint64_t other = 3;
  TestInput in(100, {p, -40, 90});
  b.update(in);
  const nina::DiscoveryObservation batch[] = {{p, -40, 200}, {p, -90, 150},
                                                {other, -50, 200}};
  in.elapsedMs = 200; in.discovery = {};
  return expect(b.update(in, batch, 3).targetId == 2,
                "older packet later in batch cannot replace newer signal");
}

bool batchDoesNotBridgeAGapOutOfOrder() {
  TestInteraction b;
  uint64_t p = 2;
  TestInput in(0, {p, -40, 0});
  b.update(in);
  const nina::DiscoveryObservation batch[] = {{p, -40, 3100}, {p, -40, 2900}};
  in.elapsedMs = 3200; in.discovery = {}; b.update(in, batch, 2);
  in.elapsedMs = 6000; in.discovery = {p, -40, 6000}; b.update(in);
  in.elapsedMs = 8000; in.discovery.receivedMs = 8000; b.update(in);
  in.elapsedMs = 10000; in.discovery.receivedMs = 10000;
  return expect(!b.update(in).sustainedProximity,
                "older packet cannot undo a genuine gap in a batch");
}

bool delayedContinuousProximity(uint32_t start) {
  TestInteraction b;
  uint64_t p = 2;
  TestInput in(start, {p, -40, start});
  b.update(in);
  in.elapsedMs = start + 3100; in.discovery.receivedMs = start + 2900; b.update(in);
  in.elapsedMs = start + 5800; in.discovery.receivedMs = start + 5800; b.update(in);
  in.elapsedMs = start + 8700; in.discovery.receivedMs = start + 8700; b.update(in);
  in.elapsedMs = start + 10000; in.discovery.receivedMs = start + 10000;
  if (!expect(b.update(in).sustainedProximity,
              "delayed observation within grace preserves sustained proximity")) return false;
  in.elapsedMs = start + 13100; in.discovery.receivedMs = start + 13000;
  return expect(!b.update(in).sustainedProximity,
                "a full grace period between observations resets sustained proximity");
}

bool delayedObservationsExpireFromReceiveTime() {
  TestInteraction b;
  uint64_t p = 2;
  TestInput in(3000, {p, -40, 0});
  if (!expect(b.update(in).peerCount == 0, "stale queued observation is ignored")) return false;
  in.elapsedMs = 4000; in.discovery.receivedMs = 2000;
  if (!expect(b.update(in).peerCount == 1, "recent queued observation is accepted")) return false;
  in.discovery = {}; in.elapsedMs = 5000;
  return expect(b.update(in).peerCount == 0, "peer expires from receive time");
}

bool olderObservationsDoNotReplaceNewerOnes() {
  TestInteraction b;
  uint64_t p = 2;
  TestInput in(100, {p, -40, 90});
  b.update(in);
  in.discovery.rssi = -90; in.discovery.receivedMs = 0xfffffff0u;
  b.update(in);
  in.discovery.peerId = 3; in.discovery.rssi = -50; in.discovery.receivedMs = 100;
  if (!expect(b.update(in).targetId == 2, "older pre-wrap RSSI cannot replace newer RSSI")) return false;
  in.discovery = {}; in.elapsedMs = 3000;
  if (!expect(b.update(in).targetId == 2, "older pre-wrap timestamp cannot shorten peer lifetime")) return false;
  in.elapsedMs = 3090;
  return expect(b.update(in).targetId == 3, "newer post-wrap observation expires at its deadline");
}

bool gazeRejectsWeakAndUnstableEvidence() {
  TestInteraction b;
  uint64_t peer = 2;
  TestInput in(100, {peer,-40,100});
  in.radioPeerId = 2; in.headingValid = true;
  for (int i = 0; i < 6; ++i) {
    in.elapsedMs = 100 + i * 100; in.headingDegrees = (i / 2) * 90;
    in.radioRssi = -90 - (i / 2) * 3; b.update(in);
  }
  in.radioPeerId = 0; in.headingDegrees = 90;
  if (!expect(b.update(in).gaze == nina::Gaze::Center, "weak evidence rejected")) return false;
  TestInteraction unstable;
  for (int i = 0; i < 6; ++i) {
    in.elapsedMs = 100 + i * 100; in.headingDegrees = (i / 2) * 90;
    in.radioPeerId = 2; in.radioRssi = i % 2 ? -80 : -40; unstable.update(in);
  }
  in.radioPeerId = 0; in.headingDegrees = 90;
  return expect(unstable.update(in).gaze == nina::Gaze::Center, "unstable evidence rejected");
}

bool gazeUsesAveragesAndStopsWhenStill() {
  TestInteraction b;
  uint64_t peer = 2;
  TestInput in(100, {peer,-40,100});
  const float headings[] = {0,0,90,90,180,180};
  const int signals[] = {-42,-56,-46,-46,-70,-70};
  in.radioPeerId = 2; in.headingValid = true;
  for (int i = 0; i < 6; ++i) {
    in.elapsedMs = 100 + i * 100; in.headingDegrees = headings[i]; in.radioRssi = signals[i];
    b.update(in);
  }
  in.radioPeerId = 0; in.headingDegrees = 0;
  if (!expect(b.update(in).gaze == nina::Gaze::Right, "bucket averages select heading 90")) return false;
  in.radioPeerId = 2; in.headingDegrees = 180; in.radioRssi = -70;
  in.elapsedMs = 2600; in.discovery.receivedMs = 2600;
  return expect(b.update(in).gaze == nina::Gaze::Center, "still samples do not renew gaze");
}

bool samePeerReturnClearsGaze() {
  TestInteraction b;
  uint64_t peer = 2;
  TestInput in(100, {peer,-40,100});
  b.update(in);
  in.radioPeerId = 2; in.headingValid = true;
  for (int i = 0; i < 6; ++i) {
    in.elapsedMs = 2500 + 100 * i; in.headingDegrees = (i / 2) * 90;
    in.radioRssi = -40 - (i / 2) * 15; b.update(in);
  }
  in.radioPeerId = 0; in.headingDegrees = 90;
  if (!expect(b.update(in).gaze == nina::Gaze::Left, "gaze confident before peer expiry")) return false;
  in.elapsedMs = 3100; in.discovery.receivedMs = 3100;
  const auto result = b.update(in);
  return expect(result.targetId == 2 && result.gaze == nina::Gaze::Center,
                "same peer returning after expiry clears gaze");
}

bool gazeAgesIndividualSamples() {
  TestInteraction b;
  uint64_t peer = 2;
  TestInput in(100, {peer,-40,100});
  in.radioPeerId = 2; in.headingValid = true; in.headingDegrees = 0; in.radioRssi = -80;
  b.update(in);
  for (int i = 0; i < 18; ++i) {
    in.elapsedMs = 200 + i * 400; in.discovery.receivedMs = in.elapsedMs;
    in.headingDegrees = (i % 3) * 90; in.radioRssi = -40 - (i % 3) * 15;
    b.update(in);
  }
  in.radioPeerId = 0; in.headingDegrees = 90;
  return expect(b.update(in).gaze == nina::Gaze::Left,
                "expired outlier stops poisoning heading bucket");
}

bool gazeRejectsOtherPeerAndInsufficientCoverage() {
  TestInteraction b;
  uint64_t peer = 2;
  TestInput in(100, {peer,-40,100});
  in.headingValid = true; in.radioPeerId = 3;
  for (int i = 0; i < 6; ++i) {
    in.elapsedMs = 100 + i * 100; in.headingDegrees = (i / 2) * 90;
    in.radioRssi = -40 - (i / 2) * 15; b.update(in);
  }
  in.radioPeerId = 0; in.headingDegrees = 90;
  if (!expect(b.update(in).gaze == nina::Gaze::Center, "other peer cannot provide evidence")) return false;
  in.radioPeerId = 2;
  for (int i = 0; i < 4; ++i) {
    in.elapsedMs = 700 + i * 100; in.headingDegrees = (i / 2) * 90;
    in.radioRssi = -40 - (i / 2) * 20; b.update(in);
  }
  in.radioPeerId = 0; in.headingDegrees = 90;
  return expect(b.update(in).gaze == nina::Gaze::Center, "two headings are insufficient coverage");
}

bool selectsStableTargetsAndIgnoresOwnIdentity() {
  TestInteraction b;
  uint64_t p = 1;
  TestInput in(0, {p, -50, 0});
  if (!expect(b.update(in).peerCount == 0, "own identity ignored")) return false;
  in.discovery.peerId = 2;
  if (!expect(b.update(in).targetId == 2, "first peer selected")) return false;
  in.discovery.peerId = 3; in.elapsedMs = 100; in.discovery.receivedMs = 100; in.discovery.rssi = -47;
  if (!expect(b.update(in).targetId == 2, "small RSSI difference preserves target")) return false;
  in.elapsedMs = 200; in.discovery.receivedMs = 200; in.discovery.rssi = -44;
  if (!expect(b.update(in).targetId == 3, "clearly stronger peer selected")) return false;
  in.discovery.peerId = 2; in.elapsedMs = 3000; in.discovery.receivedMs = 3000; in.discovery.rssi = -50;
  if (!expect(b.update(in).targetId == 3, "selected peer remains while recent")) return false;
  in.elapsedMs = 3200; in.discovery = {};
  return expect(b.update(in).targetId == 2, "expired target replaced by recent peer");
}

bool sustainedProximityAndCapacity() {
  TestInteraction b;
  uint64_t p = 2;
  TestInput in(0, {p, -40, 0});
  b.update(in);
  for (int t = 2000; t <= 10000; t += 2000) {
    in.elapsedMs = t; in.discovery.receivedMs = t;
    const auto output = b.update(in);
    if (t == 8000 && !expect(!output.sustainedProximity,
                             "focused until sustained proximity")) return false;
    if (t == 10000 && !expect(output.sustainedProximity,
                              "selected peer reaches sustained proximity")) return false;
  }
  for (int id = 3; id <= 6; ++id) { in.discovery.peerId = id; b.update(in); }
  if (!expect(b.update(in).peerCount == 4, "peer list has bounded capacity")) return false;
  in.discovery = {};
  in.elapsedMs = 13000; in.discovery.receivedMs = 13000;
  return expect(b.update(in).targetId == 0, "expiry clears the selected peer");
}

bool returningPeerStartsFocusedAgain() {
  TestInteraction b;
  uint64_t p = 2;
  TestInput in(0, {p, -40, 0});
  for (int t = 0; t <= 10000; t += 2000) { in.elapsedMs = t; in.discovery.receivedMs = t; b.update(in); }
  in.elapsedMs = 13000; in.discovery.receivedMs = 13000;
  return expect(!b.update(in).sustainedProximity,
                "peer returning after expiry starts focused again");
}

}  // namespace

int main() {
  return !(gazeRejectsOtherPeerAndInsufficientCoverage() &&
           samePeerReturnClearsGaze() &&
           gazeAgesIndividualSamples() &&
           gazeUsesAveragesAndStopsWhenStill() &&
           gazeRejectsWeakAndUnstableEvidence() &&
           multiHopGapResetsContinuity() &&
           multiHopQueuedContinuity(0) &&
           multiHopQueuedContinuity(0xfffff800u) &&
           batchDoesNotBridgeAGapOutOfOrder() &&
           batchKeepsNewestSignal() &&
           batchDoesNotReviveStalePeers() &&
           batchedDelayedContinuity(false, 0) &&
           batchedDelayedContinuity(true, 0) &&
           batchedDelayedContinuity(false, 0xfffff800u) &&
           delayedContinuousProximity(0) &&
           delayedContinuousProximity(0xfffff800u) &&
           olderObservationsDoNotReplaceNewerOnes() &&
           delayedObservationsExpireFromReceiveTime() &&
           returningPeerStartsFocusedAgain() &&
           sustainedProximityAndCapacity() &&
           selectsStableTargetsAndIgnoresOwnIdentity());
}
