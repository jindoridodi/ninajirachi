#pragma once

#include <stdint.h>
#include <stddef.h>
#include <math.h>

#include "../face/ExpressionPreset.h"

namespace nina {

constexpr uint32_t kFaceSettingsHoldMs = 5000;
constexpr uint32_t kSettingsFaceHoldMs = 3000;
constexpr uint32_t kSettingsInactivityMs = 7000;
constexpr uint32_t kExpressionTransitionMs = 300;
constexpr uint32_t kBlinkPeriodMs = 4000;
constexpr uint32_t kBlinkDurationMs = 200;

enum class ScreenName { Face, SettingsBattery };

struct BatterySnapshot {
  BatterySnapshot(uint16_t voltage = 0, bool isCharging = false, bool calibrated = false,
                  uint8_t percent = 0, const char* id = "unknown", bool voltageKnown = true,
                  bool knownCharging = false, const char* firmware = "prototype")
      : millivolts(voltage), charging(isCharging), hasEstimatedPercent(calibrated),
        estimatedPercent(percent), deviceId(id), hasVoltage(voltageKnown),
        chargingKnown(knownCharging), firmwareId(firmware) {}
  uint16_t millivolts;
  bool charging;
  bool hasEstimatedPercent;
  uint8_t estimatedPercent;
  const char* deviceId;
  bool hasVoltage;
  bool chargingKnown;
  const char* firmwareId;
};

struct SettingsBatteryDisplay {
  BatterySnapshot battery;
};

constexpr uint32_t kPeerGraceMs = 3000;
constexpr uint32_t kSustainedProximityMs = 10000;
constexpr int kTargetSwitchMarginDb = 6;
constexpr size_t kMaxPeers = 4;

struct DiscoveryObservation {
  const uint8_t* data;
  size_t size;
  int rssi;
  uint32_t receivedMs;
};

struct WearableInput {
  WearableInput(uint32_t time, bool touch, ExpressionName requested, BatterySnapshot power,
                uint64_t identity = 0, DiscoveryObservation observation = {})
      : elapsedMs(time), touchDown(touch), expression(requested), battery(power),
        localId(identity), discovery(observation) {}
  uint32_t elapsedMs;
  bool touchDown;
  ExpressionName expression;
  BatterySnapshot battery;
  uint64_t localId;
  DiscoveryObservation discovery;
};

struct WearableOutput {
  ScreenName screen;
  ExpressionPreset face;
  SettingsBatteryDisplay settings;
  size_t peerCount;
  uint64_t targetId;
};

class WearableBehavior {
 public:
  WearableOutput update(const WearableInput& input) {
    return update(input, &input.discovery, 1);
  }

  WearableOutput update(const WearableInput& input, const DiscoveryObservation* observations, size_t count) {
    updateScreen(input.elapsedMs, input.touchDown);
    updatePeers(input.elapsedMs, input.localId, observations, count);
    ExpressionName requested = input.expression;
    if (targetId_) requested = input.elapsedMs - targetSinceMs_ >= kSustainedProximityMs
        ? ExpressionName::Excited : ExpressionName::Curious;
    if (!initialized_) {
      face_ = from_ = expressionPreset(requested);
      transitionStartMs_ = input.elapsedMs - kExpressionTransitionMs;
      initialized_ = true;
    }
    if (requested != face_.name) {
      from_ = face_;
      transitionStartMs_ = input.elapsedMs;
    }
    const uint32_t age = input.elapsedMs - transitionStartMs_;
    float t = age >= kExpressionTransitionMs ? 1.0f : float(age) / kExpressionTransitionMs;
    t = t * t * (3 - 2 * t);
    ExpressionPreset face = expressionPreset(requested);
    animate(face, input.elapsedMs);
    face_ = blend(from_, face, t);
    return {screen_, face_, {input.battery}, peerCount_, targetId_};
  }

 private:
  struct Peer { uint64_t id; uint32_t seen; int rssi; };

  static uint64_t observationId(const DiscoveryObservation& observation, uint32_t now, uint64_t localId) {
    uint64_t id = 0;
    if (observation.data && now - observation.receivedMs < 0x80000000u &&
        observation.size == 12 && observation.rssi < 0 &&
        observation.rssi >= -127 && observation.data[0] == 'N' && observation.data[1] == 'I' &&
        observation.data[2] == 'N' && observation.data[3] == 'A' &&
        observation.data[4] == 1 && observation.data[5] == 1) {
      for (size_t i = 6; i < 12; ++i) id = (id << 8) | observation.data[i];
      if (id == localId) id = 0;
    }
    return id;
  }

  void updatePeers(uint32_t now, uint64_t localId, const DiscoveryObservation* observations, size_t count) {
    for (size_t i = 0; i < kMaxPeers; ++i) {
      Peer& peer = peers_[i];
      bool hasRecentObservation = false;
      for (size_t j = 0; peer.id && j < count; ++j) {
        const auto& observation = observations[j];
        if (now - observation.receivedMs < kPeerGraceMs &&
            observationId(observation, now, localId) == peer.id) {
          hasRecentObservation = true;
          break;
        }
      }
      if (peer.id && now - peer.seen >= kPeerGraceMs && !hasRecentObservation) {
        if (peer.id == targetId_) targetId_ = 0;
        peer.id = 0;
      }
    }
    for (size_t j = 0; j < count; ++j) {
      const auto& observation = observations[j];
      const uint64_t id = observationId(observation, now, localId);
      if (!id) continue;
      Peer* slot = nullptr;
      for (size_t i = 0; i < kMaxPeers; ++i) {
        if (peers_[i].id == id) { slot = &peers_[i]; break; }
        if (!peers_[i].id && !slot) slot = &peers_[i];
      }
      if (slot && !slot->id && now - observation.receivedMs >= kPeerGraceMs) continue;
      if (slot && (!slot->id || now - observation.receivedMs <= now - slot->seen)) {
        if (slot->id && observation.receivedMs - slot->seen >= kPeerGraceMs && id == targetId_)
          targetId_ = 0;
        *slot = {id, observation.receivedMs, observation.rssi};
      }
    }
    for (size_t i = 0; i < kMaxPeers; ++i) {
      Peer& peer = peers_[i];
      if (peer.id && now - peer.seen >= kPeerGraceMs) {
        if (peer.id == targetId_) targetId_ = 0;
        peer.id = 0;
      }
    }
    Peer* current = nullptr;
    Peer* strongest = nullptr;
    peerCount_ = 0;
    for (size_t i = 0; i < kMaxPeers; ++i) {
      Peer& peer = peers_[i];
      if (!peer.id) continue;
      ++peerCount_;
      if (peer.id == targetId_) current = &peer;
      if (!strongest || peer.rssi > strongest->rssi) strongest = &peer;
    }
    if (!current || (strongest && strongest->rssi >= current->rssi + kTargetSwitchMarginDb)) {
      targetId_ = strongest ? strongest->id : 0;
      targetSinceMs_ = now;
    }
  }

  static ExpressionPreset blend(const ExpressionPreset& a, const ExpressionPreset& b, float t) {
    auto value = [t](int x, int y) { return static_cast<int16_t>(lroundf(x + (y - x) * t)); };
    auto color = [value](Color x, Color y) {
      uint32_t result = 0;
      for (int shift = 16; shift >= 0; shift -= 8)
        result |= uint32_t(value((x.rgb >> shift) & 255, (y.rgb >> shift) & 255)) << shift;
      return Color{result};
    };
    const auto& x = a.metrics;
    const auto& y = b.metrics;
    return {b.name,
      {color(a.colors.background, b.colors.background), color(a.colors.face, b.colors.face),
       color(a.colors.eye, b.colors.eye), color(a.colors.eyelid, b.colors.eyelid),
       color(a.colors.pupil, b.colors.pupil), color(a.colors.mouth, b.colors.mouth),
       color(a.colors.effect, b.colors.effect)},
      {value(x.faceRadius, y.faceRadius), value(x.eyeWidth, y.eyeWidth),
       value(x.eyeOpen, y.eyeOpen), value(x.eyelidHeight, y.eyelidHeight),
       value(x.pupilRadius, y.pupilRadius), value(x.pupilOffsetX, y.pupilOffsetX),
       value(x.pupilOffsetY, y.pupilOffsetY), value(x.mouthWidth, y.mouthWidth),
       value(x.mouthHeight, y.mouthHeight), value(x.effectRadius, y.effectRadius)}};
  }

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
      if (wasTouchDown_) lastTouchMs_ = elapsedMs;
      awaitTouchRelease_ = false;
      if (screen_ == ScreenName::SettingsBattery &&
          elapsedMs - lastTouchMs_ >= kSettingsInactivityMs) {
        screen_ = ScreenName::Face;
      }
    }
    wasTouchDown_ = touchDown;
  }

  static void animate(ExpressionPreset& face, uint32_t elapsedMs) {
    const uint32_t phaseMs = elapsedMs % kBlinkPeriodMs;
    const bool blinking = face.name != ExpressionName::Surprised &&
                          face.name != ExpressionName::Excited &&
                          phaseMs >= kBlinkPeriodMs - kBlinkDurationMs;
    if (blinking) {
      const float openness = fabsf(float(phaseMs) - (kBlinkPeriodMs - kBlinkDurationMs / 2)) /
                             (kBlinkDurationMs / 2);
      face.metrics.eyeOpen = 3 + lroundf((face.metrics.eyeOpen - 3) * openness);
      face.metrics.pupilRadius = lroundf(face.metrics.pupilRadius * openness);
    }

    const int16_t pupilMotion = lroundf(2 * sinf(float(elapsedMs % 5600) * 6.2831853f / 5600));
    face.metrics.pupilOffsetX += pupilMotion;
    if (face.name == ExpressionName::Idle) {
      face.metrics.mouthHeight += lroundf(sinf(float(elapsedMs % 5600) * 6.2831853f / 5600));
    }
    if (face.name == ExpressionName::Happy || face.name == ExpressionName::Excited) {
      face.metrics.mouthHeight += lroundf(2 * sinf(float(elapsedMs % 1000) * 6.2831853f / 1000));
    }
  }

  Peer peers_[kMaxPeers] = {};
  size_t peerCount_ = 0;
  uint64_t targetId_ = 0;
  uint32_t targetSinceMs_ = 0;
  ScreenName screen_ = ScreenName::Face;
  ScreenName gestureScreen_ = ScreenName::Face;
  uint32_t touchStartMs_ = 0;
  uint32_t lastTouchMs_ = 0;
  bool wasTouchDown_ = false;
  bool awaitTouchRelease_ = false;
  bool initialized_ = false;
  uint32_t transitionStartMs_ = 0;
  ExpressionPreset face_ = idlePreset();
  ExpressionPreset from_ = idlePreset();
};

}  // namespace nina
