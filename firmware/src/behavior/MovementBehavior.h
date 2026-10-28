#pragma once

#include <stdint.h>

#include "../face/ExpressionPreset.h"

namespace nina {

struct MovementSnapshot {
  float rollDegrees;
  float verticalAccelerationG;
};

struct MovementTuning {
  float tiltDegreesForFullPupilShift;
  int16_t maximumTiltPupilShift;
  float jumpAccelerationThresholdG;
  uint32_t jumpOverlayDurationMs;
  uint32_t jumpCooldownMs;
};

inline constexpr MovementTuning kDefaultMovementTuning = {
    20.0f, 4, 1.8f, 350, 800,
};

struct MovementRenderState {
  ExpressionPreset preset;
  bool jumpOverlayActive;
};

class MovementBehavior {
 public:
  explicit MovementBehavior(MovementTuning tuning = kDefaultMovementTuning) : tuning_(tuning) {}

  MovementRenderState update(const MovementSnapshot& motion, const ExpressionPreset& underlying,
                             uint32_t nowMs) {
    const bool jumpSignalActive = motion.verticalAccelerationG >= tuning_.jumpAccelerationThresholdG;
    if (jumpSignalActive && !jumpSignalActive_ && hasReached(nowMs, cooldownEndsAtMs_)) {
      overlayEndsAtMs_ = nowMs + tuning_.jumpOverlayDurationMs;
      cooldownEndsAtMs_ = nowMs + tuning_.jumpCooldownMs;
    }
    jumpSignalActive_ = jumpSignalActive;

    MovementRenderState result = {withTilt(motion, underlying), !hasReached(nowMs, overlayEndsAtMs_)};
    if (result.jumpOverlayActive) result.preset = surprisedOverlay(result.preset);
    return result;
  }

 private:
  static bool hasReached(uint32_t nowMs, uint32_t deadlineMs) {
    return static_cast<int32_t>(nowMs - deadlineMs) >= 0;
  }

  ExpressionPreset withTilt(const MovementSnapshot& motion, const ExpressionPreset& underlying) const {
    ExpressionPreset adjusted = underlying;
    float ratio = motion.rollDegrees / tuning_.tiltDegreesForFullPupilShift;
    if (ratio > 1.0f) ratio = 1.0f;
    if (ratio < -1.0f) ratio = -1.0f;
    adjusted.metrics.pupilOffsetX += static_cast<int16_t>(ratio * tuning_.maximumTiltPupilShift);
    return adjusted;
  }

  static ExpressionPreset surprisedOverlay(ExpressionPreset preset) {
    preset.colors.background = preset.colors.effect;
    preset.metrics.eyeOpen += 24;
    preset.metrics.eyelidHeight = 0;
    preset.metrics.mouthHeight += 26;
    if (preset.metrics.effectRadius < 12) preset.metrics.effectRadius = 12;
    return preset;
  }

  MovementTuning tuning_;
  bool jumpSignalActive_ = false;
  uint32_t overlayEndsAtMs_ = 0;
  uint32_t cooldownEndsAtMs_ = 0;
};

}  // namespace nina
