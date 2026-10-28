#pragma once

#include <cmath>
#include <cstdint>
#include <limits>

namespace nina {

struct MagneticSample {
  int16_t x;
  int16_t y;
  int16_t z;
};

struct MagneticVector {
  float x;
  float y;
  float z;
};

struct CompassCalibrationData {
  MagneticSample minimum;
  MagneticSample maximum;
  float headingOffsetDegrees;
  CompassCalibrationData(MagneticSample low = {}, MagneticSample high = {}, float offset = 0)
      : minimum(low), maximum(high), headingOffsetDegrees(offset) {}
};

class CompassCalibration {
 public:
  CompassCalibration() = default;
  explicit CompassCalibration(CompassCalibrationData data)
      : minimum_(data.minimum), maximum_(data.maximum), headingOffsetDegrees_(data.headingOffsetDegrees) {}

  void observe(MagneticSample sample) {
    minimum_.x = min(minimum_.x, sample.x);
    minimum_.y = min(minimum_.y, sample.y);
    minimum_.z = min(minimum_.z, sample.z);
    maximum_.x = max(maximum_.x, sample.x);
    maximum_.y = max(maximum_.y, sample.y);
    maximum_.z = max(maximum_.z, sample.z);
  }

  bool ready() const {
    return std::isfinite(headingOffsetDegrees_) && std::fabs(headingOffsetDegrees_) <= 360 &&
           maximum_.x > minimum_.x && maximum_.y > minimum_.y && maximum_.z > minimum_.z;
  }

  CompassCalibrationData data() const { return {minimum_, maximum_, headingOffsetDegrees_}; }

  MagneticVector apply(MagneticSample sample) const {
    return {center(sample.x, minimum_.x, maximum_.x), center(sample.y, minimum_.y, maximum_.y),
            center(sample.z, minimum_.z, maximum_.z)};
  }

  float headingDegrees(MagneticSample sample, MagneticVector gravity = {0, 0, 1}) const {
    const float invalid = std::numeric_limits<float>::quiet_NaN();
    if (!ready()) return invalid;
    const float magnitude = std::sqrt(gravity.x * gravity.x + gravity.y * gravity.y + gravity.z * gravity.z);
    // Gravity is in g, aligned to the magnetometer; acceleration during jumps is unreliable.
    if (!std::isfinite(magnitude) || magnitude < 0.8f || magnitude > 1.2f) return invalid;
    gravity = {gravity.x / magnitude, gravity.y / magnitude, gravity.z / magnitude};
    const float forwardLength = std::sqrt(gravity.y * gravity.y + gravity.z * gravity.z);
    if (forwardLength < 0.1f) return invalid;
    const MagneticVector forward{forwardLength, -gravity.x * gravity.y / forwardLength,
                                 -gravity.x * gravity.z / forwardLength};
    const MagneticVector right{0, gravity.z / forwardLength, -gravity.y / forwardLength};
    const MagneticVector field = apply(sample);
    const float x = field.x * forward.x + field.y * forward.y + field.z * forward.z;
    const float y = field.y * right.y + field.z * right.z;
    if (!std::isfinite(x) || !std::isfinite(y) || x * x + y * y < 1e-8f) return invalid;
    constexpr float kRadiansToDegrees = 57.2957795131f;
    return normalizeDegrees(std::atan2(y, x) * kRadiansToDegrees + headingOffsetDegrees_);
  }

 private:
  static int16_t min(int16_t left, int16_t right) { return left < right ? left : right; }
  static int16_t max(int16_t left, int16_t right) { return left > right ? left : right; }

  static float center(int16_t value, int16_t minimum, int16_t maximum) {
    // shortcut: diagonal soft-iron correction; use an ellipsoid fit if enclosure tests show cross-axis distortion.
    return (static_cast<float>(value) - (static_cast<float>(minimum) + maximum) / 2.0f) /
           ((static_cast<float>(maximum) - minimum) / 2.0f);
  }

  static float normalizeDegrees(float heading) {
    while (heading < 0) heading += 360.0f;
    while (heading >= 360.0f) heading -= 360.0f;
    return heading;
  }

  MagneticSample minimum_{std::numeric_limits<int16_t>::max(),
                          std::numeric_limits<int16_t>::max(),
                          std::numeric_limits<int16_t>::max()};
  MagneticSample maximum_{std::numeric_limits<int16_t>::min(),
                          std::numeric_limits<int16_t>::min(),
                          std::numeric_limits<int16_t>::min()};
  float headingOffsetDegrees_ = 0;
};

}  // namespace nina
