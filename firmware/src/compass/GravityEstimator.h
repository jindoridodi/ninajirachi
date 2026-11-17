#pragma once

#include "CompassCalibration.h"

namespace nina {

class GravityEstimator {
 public:
  static constexpr uint32_t kQuietWindowMs = 250;
  static constexpr uint32_t kMaxSampleAgeMs = 250;
  static constexpr float kMaxTiltRotationDps = 10;
  static constexpr float kMaxMagnitudeDeviationG = 0.1f;
  static constexpr float kMaxDirectionDeviation = 0.1f;
  static constexpr float kMaxPredictedTiltRadians = 0.35f;
  static constexpr float kFilterAlpha = 0.2f;

  MagneticVector update(uint32_t timestamp, MagneticVector accelerationMg,
                        MagneticVector gyroDps, uint32_t nowMs) {
    const uint32_t elapsed = uint32_t(nowMs - sampledAtMs_);
    const bool stale = haveSample_ && elapsed > kMaxSampleAgeMs;
    if (stale) settling_ = ready_ = false;
    if (haveSample_ && timestamp == timestamp_) return {0, 0, 0};
    haveSample_ = true;
    timestamp_ = timestamp;
    sampledAtMs_ = nowMs;
    const float magnitude = length(accelerationMg) / 1000;
    const float rotation = length(gyroDps);
    if (!std::isfinite(magnitude) || !std::isfinite(rotation) ||
        std::fabs(magnitude - 1) > kMaxMagnitudeDeviationG) {
      settling_ = ready_ = false;
      return {0, 0, 0};
    }
    const MagneticVector down{accelerationMg.x / (1000 * magnitude),
                              accelerationMg.y / (1000 * magnitude),
                              accelerationMg.z / (1000 * magnitude)};
    // Rotation around gravity changes heading without changing the gravity direction.
    const float tiltRotation = length({gyroDps.y * down.z - gyroDps.z * down.y,
                                       gyroDps.z * down.x - gyroDps.x * down.z,
                                       gyroDps.x * down.y - gyroDps.y * down.x});
    if (ready_) {
      const float angle = rotation * elapsed * (0.01745329252f / 1000.0f);
      const float predictedTilt = length({gyroDps.y * filtered_.z - gyroDps.z * filtered_.y,
                                          gyroDps.z * filtered_.x - gyroDps.x * filtered_.z,
                                          gyroDps.x * filtered_.y - gyroDps.y * filtered_.x}) *
                                  elapsed * (0.01745329252f / 1000.0f);
      if (predictedTilt > kMaxPredictedTiltRadians) {
        settling_ = ready_ = false;
        return {0, 0, 0};
      }
      if (rotation > 0) {
        // Rotate the previous gravity vector into the current sensor frame.
        const MagneticVector axis{gyroDps.x / rotation, gyroDps.y / rotation, gyroDps.z / rotation};
        const float cosine = std::cos(angle), sine = std::sin(angle);
        const float dot = axis.x * filtered_.x + axis.y * filtered_.y + axis.z * filtered_.z;
        const MagneticVector cross{axis.y * filtered_.z - axis.z * filtered_.y,
                                   axis.z * filtered_.x - axis.x * filtered_.z,
                                   axis.x * filtered_.y - axis.y * filtered_.x};
        filtered_ = {filtered_.x * cosine - cross.x * sine + axis.x * dot * (1 - cosine),
                     filtered_.y * cosine - cross.y * sine + axis.y * dot * (1 - cosine),
                     filtered_.z * cosine - cross.z * sine + axis.z * dot * (1 - cosine)};
      }
      if (length({down.x - filtered_.x, down.y - filtered_.y, down.z - filtered_.z}) <= kMaxDirectionDeviation) {
        filtered_.x += kFilterAlpha * (down.x - filtered_.x);
        filtered_.y += kFilterAlpha * (down.y - filtered_.y);
        filtered_.z += kFilterAlpha * (down.z - filtered_.z);
        correctedAtMs_ = nowMs;
      } else if (uint32_t(nowMs - correctedAtMs_) > kMaxSampleAgeMs) {
        settling_ = ready_ = false;
        return {0, 0, 0};
      }
      return filtered_;
    }
    if (tiltRotation > kMaxTiltRotationDps) {
      settling_ = false;
      return {0, 0, 0};
    }
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
    if (uint32_t(nowMs - quietSinceMs_) < kQuietWindowMs) return {0, 0, 0};
    ready_ = true;
    correctedAtMs_ = nowMs;
    return filtered_;
  }

 private:
  static float length(MagneticVector v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }
  MagneticVector filtered_{0, 0, 0};
  uint32_t timestamp_ = 0, sampledAtMs_ = 0, quietSinceMs_ = 0, correctedAtMs_ = 0;
  bool haveSample_ = false, settling_ = false, ready_ = false;
};

}  // namespace nina
