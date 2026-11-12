#pragma once

#include <stdint.h>
#include <stddef.h>
#include <math.h>

#include "../face/ExpressionPreset.h"
#include "NearbyInteraction.h"
#include "MovementBehavior.h"

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

struct WearableInput {
  WearableInput(uint32_t time, bool touch, ExpressionName requested, BatterySnapshot power,
                uint64_t identity = 0)
      : elapsedMs(time), touchDown(touch), expression(requested), battery(power),
        localId(identity) {}
  uint32_t elapsedMs;
  bool touchDown;
  ExpressionName expression;
  BatterySnapshot battery;
  uint64_t localId;
  float headingDegrees = 0;
  bool headingValid = false;
  bool motionValid = false;
  bool accelerationValid = false;
  float accelerationMagnitudeG = 0;
  float rollDegrees = 0;
  float verticalAccelerationG = 0;
};

struct WearableOutput {
  ScreenName screen;
  ExpressionPreset face;
  SettingsBatteryDisplay settings;
  size_t peerCount;
  uint64_t targetId;
  Gaze gaze;
  bool jumpOverlayActive;
};

class WearableBehavior {
 public:
  void triggerJump(uint32_t nowMs) { movement_.triggerJump(nowMs); }

  WearableOutput update(const WearableInput& input, const DiscoveryObservation* observations, size_t count,
                        const DirectionObservation* directions = nullptr, size_t directionCount = 0) {
    updateScreen(input.elapsedMs, input.touchDown);
    const auto nearby = nearby_.update(input.elapsedMs, input.localId, observations, count,
                                       input.headingValid, input.headingDegrees, directions, directionCount);
    const Gaze gaze = nearby.gaze;
    ExpressionName requested = input.expression;
    if (nearby.targetId) requested = nearby.sustainedProximity
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
    const float desiredGaze = nearby.targetId && gaze == Gaze::Left ? -8 :
                              nearby.targetId && gaze == Gaze::Right ? 8 : 0;
    const float step = float(input.elapsedMs - gazeUpdatedMs_) * (16.0f / kExpressionTransitionMs);
    if (desiredGaze > gazeOffset_) gazeOffset_ = fminf(desiredGaze, gazeOffset_ + step);
    else gazeOffset_ = fmaxf(desiredGaze, gazeOffset_ - step);
    gazeUpdatedMs_ = input.elapsedMs;
    ExpressionPreset visible = face_;
    if (nearby.targetId) {
      visible.metrics.eyeOffsetX = lroundf(gazeOffset_) + idleEyeMotion(input.elapsedMs);
    } else visible.metrics.eyeOffsetX += lroundf(gazeOffset_);
    const auto motion = movement_.update({input.motionValid ? input.rollDegrees : 0,
        input.motionValid ? input.verticalAccelerationG : 0}, visible, input.elapsedMs, input.motionValid,
        input.accelerationValid, input.accelerationMagnitudeG);
    return {screen_, motion.preset, {input.battery}, nearby.peerCount, nearby.targetId, gaze, motion.jumpOverlayActive};
  }

 private:
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
       color(a.colors.eye, b.colors.eye), color(a.colors.mouth, b.colors.mouth)},
      {value(x.eyeWidth, y.eyeWidth), value(x.eyeOpen, y.eyeOpen),
       value(x.eyeOffsetX, y.eyeOffsetX),
       value(x.eyeOffsetY, y.eyeOffsetY), value(x.mouthWidth, y.mouthWidth),
       value(x.mouthHeight, y.mouthHeight), value(x.tongueOut, y.tongueOut)}};
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

  static int16_t idleEyeMotion(uint32_t elapsedMs) {
    return lroundf(2 * sinf(float(elapsedMs % 5600) * 6.2831853f / 5600));
  }

  static void animate(ExpressionPreset& face, uint32_t elapsedMs) {
    const uint32_t phaseMs = elapsedMs % kBlinkPeriodMs;
    const bool blinking = face.name != ExpressionName::Excited &&
                          phaseMs >= kBlinkPeriodMs - kBlinkDurationMs;
    if (blinking) {
      const float openness = fabsf(float(phaseMs) - (kBlinkPeriodMs - kBlinkDurationMs / 2)) /
                             (kBlinkDurationMs / 2);
      face.metrics.eyeOpen = 3 + lroundf((face.metrics.eyeOpen - 3) * openness);
    }

    face.metrics.eyeOffsetX += idleEyeMotion(elapsedMs);
    if (face.name == ExpressionName::Idle || face.name == ExpressionName::Curious) {
      face.metrics.mouthHeight += lroundf(sinf(float(elapsedMs % 5600) * 6.2831853f / 5600));
    }
    if (face.name == ExpressionName::Excited) {
      face.metrics.mouthHeight += lroundf(2 * sinf(float(elapsedMs % 1000) * 6.2831853f / 1000));
    }
  }

  NearbyInteraction nearby_;
  MovementBehavior movement_;
  float gazeOffset_ = 0;
  uint32_t gazeUpdatedMs_ = 0;
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
