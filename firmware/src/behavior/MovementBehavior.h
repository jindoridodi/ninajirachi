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
      overlayStartedAtMs_ = cooldownStartedAtMs_ = nowMs;
      overlayActive_ = cooldownActive_ = true;
      jumpPending_ = false;
    }

    if (!tiltInitialized_) { filteredRoll_ = valid ? motion.rollDegrees : 0; tiltInitialized_ = true; }
    const float desiredRoll = valid ? motion.rollDegrees : filteredRoll_;
    const float step = (nowMs - lastTiltMs_) * 0.06f;
    if (desiredRoll > filteredRoll_) filteredRoll_ = fminf(desiredRoll, filteredRoll_ + step);
    else filteredRoll_ = fmaxf(desiredRoll, filteredRoll_ - step);
    lastTiltMs_ = nowMs;
    MovementRenderState result = {withTilt({filteredRoll_, motion.verticalAccelerationG}, underlying), overlayActive_};
    if (result.jumpOverlayActive) result.preset = surprisedOverlay(result.preset, nowMs - overlayStartedAtMs_);
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

  static ExpressionPreset surprisedOverlay(ExpressionPreset preset, uint32_t elapsedMs) {
    constexpr uint32_t colors[] = {0xff0044, 0x0055ff, 0x00aa55, 0x9900cc, 0xdd7700, 0x00aaaa};
    preset.colors.background.rgb = colors[(elapsedMs / 200) % (sizeof(colors) / sizeof(colors[0]))];
    preset.metrics.eyeOpen += 24;
    preset.metrics.mouthHeight += 26;
    return preset;
  }

  MovementTuning tuning_;
  bool jumpPending_ = false;
  uint32_t jumpStartedAtMs_ = 0;
  bool tiltInitialized_ = false;
  float filteredRoll_ = 0;
  uint32_t lastTiltMs_ = 0;
  bool overlayActive_ = false, cooldownActive_ = false;
  uint32_t overlayStartedAtMs_ = 0, cooldownStartedAtMs_ = 0;
};

}  // namespace nina
