#pragma once

#include "CompassCalibration.h"

namespace nina {

class GravityEstimator {
 public:
  static constexpr uint32_t kQuietWindowMs = 250;
  static constexpr uint32_t kMaxSampleAgeMs = 250;
  static constexpr float kMaxRotationDps = 10;
  static constexpr float kMaxMagnitudeDeviationG = 0.1f;
  static constexpr float kMaxDirectionDeviation = 0.1f;
  static constexpr float kFilterAlpha = 0.2f;

  MagneticVector update(uint32_t timestamp, MagneticVector accelerationMg,
                        MagneticVector gyroDps, uint32_t nowMs) {
    const bool stale = haveSample_ && uint32_t(nowMs - sampledAtMs_) > kMaxSampleAgeMs;
    if (stale) settling_ = false;
    if (haveSample_ && timestamp == timestamp_) return {0, 0, 0};
    haveSample_ = true;
    timestamp_ = timestamp;
    sampledAtMs_ = nowMs;
    const float magnitude = length(accelerationMg) / 1000;
    const float rotation = length(gyroDps);
    if (!std::isfinite(magnitude) || !std::isfinite(rotation) ||
        std::fabs(magnitude - 1) > kMaxMagnitudeDeviationG || rotation > kMaxRotationDps) {
      settling_ = false;
      return {0, 0, 0};
    }
    const MagneticVector down{accelerationMg.x / (1000 * magnitude),
                              accelerationMg.y / (1000 * magnitude),
                              accelerationMg.z / (1000 * magnitude)};
    if (settling_ && length({down.x - filtered_.x, down.y - filtered_.y,
                            down.z - filtered_.z}) > kMaxDirectionDeviation) {
      settling_ = false;
      return {0, 0, 0};
    }
    if (!settling_) {
      filtered_ = down;
      quietSinceMs_ = nowMs;
      settling_ = true;
    } else {
      filtered_.x += kFilterAlpha * (down.x - filtered_.x);
      filtered_.y += kFilterAlpha * (down.y - filtered_.y);
      filtered_.z += kFilterAlpha * (down.z - filtered_.z);
    }
    // shortcut: quiet-motion gates cannot identify all linear acceleration; tune on the assembled wearable.
    return uint32_t(nowMs - quietSinceMs_) >= kQuietWindowMs ? filtered_ : MagneticVector{0, 0, 0};
  }

 private:
  static float length(MagneticVector v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }
  MagneticVector filtered_{0, 0, 0};
  uint32_t timestamp_ = 0, sampledAtMs_ = 0, quietSinceMs_ = 0;
  bool haveSample_ = false, settling_ = false;
};

}  // namespace nina
