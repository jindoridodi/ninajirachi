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
  float headingOffsetDegrees = 0;
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
    return maximum_.x > minimum_.x && maximum_.y > minimum_.y && maximum_.z > minimum_.z;
  }

  CompassCalibrationData data() const { return {minimum_, maximum_, headingOffsetDegrees_}; }

  MagneticVector apply(MagneticSample sample) const {
    return {center(sample.x, minimum_.x, maximum_.x), center(sample.y, minimum_.y, maximum_.y),
            center(sample.z, minimum_.z, maximum_.z)};
  }

  float headingDegrees(MagneticSample sample) const {
    const MagneticVector field = apply(sample);
    constexpr float kRadiansToDegrees = 57.2957795131f;
    return normalizeDegrees(std::atan2(field.y, field.x) * kRadiansToDegrees + headingOffsetDegrees_);
  }

 private:
  static int16_t min(int16_t left, int16_t right) { return left < right ? left : right; }
  static int16_t max(int16_t left, int16_t right) { return left > right ? left : right; }

  static float center(int16_t value, int16_t minimum, int16_t maximum) {
    // shortcut: axis-aligned min/max compensation, upgrade to an ellipsoid fit if enclosure tests warp headings.
    return static_cast<float>(value) - (static_cast<float>(minimum) + maximum) / 2.0f;
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
