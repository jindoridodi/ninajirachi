#pragma once

#include <stdint.h>
#include <math.h>

#include "../face/ExpressionPreset.h"

namespace nina {

struct MovementSnapshot {
  float rollDegrees;
  float verticalAccelerationG;
};

struct MovementTuning {
  float tiltDegreesForFullEyeShift;
  int16_t maximumTiltEyeShift;
  float jumpAccelerationThresholdG;
  uint32_t jumpOverlayDurationMs;
  uint32_t jumpCooldownMs;
};

constexpr MovementTuning kDefaultMovementTuning = {
    20.0f, 4, 1.8f, 2000, 800,
};
constexpr float kJumpLowAccelerationG = 0.6f;
constexpr float kJumpLowMagnitudeG = 0.4f;
constexpr uint32_t kJumpPatternWindowMs = 800;

struct MovementRenderState {
  ExpressionPreset preset;
  bool jumpOverlayActive;
};

class MovementBehavior {
 public:
  explicit MovementBehavior(MovementTuning tuning = kDefaultMovementTuning) : tuning_(tuning) {}

  void triggerJump(uint32_t nowMs) {
    ++jumpSequence_;
    overlayStartedAtMs_ = cooldownStartedAtMs_ = nowMs;
    overlayActive_ = cooldownActive_ = true;
    jumpPending_ = false;
  }

  MovementRenderState update(const MovementSnapshot& motion, const ExpressionPreset& underlying,
                             uint32_t nowMs, bool valid = true, bool accelerationValid = false,
                             float accelerationMagnitudeG = 0) {
    const bool jumpValid = valid || (accelerationValid && isfinite(accelerationMagnitudeG));
    const float jumpAcceleration = valid ? motion.verticalAccelerationG : accelerationMagnitudeG;
    if (cooldownActive_ && nowMs - cooldownStartedAtMs_ >= tuning_.jumpCooldownMs) cooldownActive_ = false;
    if (overlayActive_ && nowMs - overlayStartedAtMs_ >= tuning_.jumpOverlayDurationMs) overlayActive_ = false;
    if (jumpValid && jumpAcceleration < (valid ? kJumpLowAccelerationG : kJumpLowMagnitudeG) &&
        !cooldownActive_ && !jumpPending_) {
      jumpPending_ = true;
      jumpStartedAtMs_ = nowMs;
    }
    if (jumpPending_ && nowMs - jumpStartedAtMs_ > kJumpPatternWindowMs) jumpPending_ = false;
    if (jumpValid && jumpPending_ && jumpAcceleration >= tuning_.jumpAccelerationThresholdG &&
        !cooldownActive_) {
      triggerJump(nowMs);
    }

    if (!tiltInitialized_) { filteredRoll_ = valid ? motion.rollDegrees : 0; tiltInitialized_ = true; }
    const float desiredRoll = valid ? motion.rollDegrees : filteredRoll_;
    const float step = (nowMs - lastTiltMs_) * 0.06f;
    if (desiredRoll > filteredRoll_) filteredRoll_ = fminf(desiredRoll, filteredRoll_ + step);
    else filteredRoll_ = fmaxf(desiredRoll, filteredRoll_ - step);
    lastTiltMs_ = nowMs;
    MovementRenderState result = {withTilt({filteredRoll_, motion.verticalAccelerationG}, underlying), overlayActive_};
    if (result.jumpOverlayActive) result.preset = jumpOverlay(result.preset, nowMs - overlayStartedAtMs_, tuning_.jumpOverlayDurationMs, jumpSequence_);
    return result;
  }

 private:
  ExpressionPreset withTilt(const MovementSnapshot& motion, const ExpressionPreset& underlying) const {
    ExpressionPreset adjusted = underlying;
    float ratio = motion.rollDegrees / tuning_.tiltDegreesForFullEyeShift;
    if (ratio > 1.0f) ratio = 1.0f;
    if (ratio < -1.0f) ratio = -1.0f;
    adjusted.metrics.eyeOffsetX += static_cast<int16_t>(ratio * tuning_.maximumTiltEyeShift);
    return adjusted;
  }

  static ExpressionPreset jumpOverlay(ExpressionPreset preset, uint32_t elapsedMs, uint32_t durationMs, uint8_t sequence) {
    constexpr uint32_t colors[] = {0xff315f, 0x00e5ff, 0xffdf00, 0xe050ff, 0x32ff65, 0xff9a10};
    const float phase = durationMs ? float(elapsedMs) / durationMs : 1.0f;
    const float release = fmaxf(0, (phase - 0.75f) / 0.25f);
    const float strength = 1 - release * release * (3 - 2 * release);
    const float attack = fminf(1, elapsedMs / 150.0f);
    const float eyeStrength = strength * attack * attack * (3 - 2 * attack);
    auto toward = [strength](int value, int target) {
      return static_cast<int16_t>(lroundf(value + (target - value) * strength));
    };
    const uint32_t step = elapsedMs / 100;
    const uint32_t flash = colors[(sequence + step * (sequence % 2 ? 1 : 5)) % 6];
    uint32_t background = 0;
    for (int shift = 16; shift >= 0; shift -= 8)
      background |= uint32_t(toward((preset.colors.background.rgb >> shift) & 255,
                                    (flash >> shift) & 255)) << shift;
    preset.colors.background.rgb = background;
    auto eyeToward = [eyeStrength](int value, int target) {
      return static_cast<int16_t>(lroundf(value + (target - value) * eyeStrength));
    };
    preset.metrics.eyeWidth = eyeToward(preset.metrics.eyeWidth, 38);
    preset.metrics.eyeOpen = eyeToward(preset.metrics.eyeOpen, 48);
    preset.metrics.eyeOffsetY = eyeToward(preset.metrics.eyeOffsetY, 2);
    const float tongueAttack = fminf(1, elapsedMs / 400.0f);
    const float tongueStrength = strength * tongueAttack * tongueAttack * (3 - 2 * tongueAttack);
    preset.metrics.tongueOut = static_cast<int16_t>(lroundf(preset.metrics.tongueOut +
        (12 - preset.metrics.tongueOut) * tongueStrength));
    return preset;
  }

  MovementTuning tuning_;
  uint8_t jumpSequence_ = 0;
  bool jumpPending_ = false;
  uint32_t jumpStartedAtMs_ = 0;
  bool tiltInitialized_ = false;
  float filteredRoll_ = 0;
  uint32_t lastTiltMs_ = 0;
  bool overlayActive_ = false, cooldownActive_ = false;
  uint32_t overlayStartedAtMs_ = 0, cooldownStartedAtMs_ = 0;
};

}  // namespace nina
