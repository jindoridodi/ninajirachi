#pragma once

#include <Wire.h>
#include "../waveshare/QMI8658.h"
#include "../compass/GravityEstimator.h"

namespace nina {

struct MotionSample {
  MagneticVector gravity{0, 0, 0};
  float acceleration[3] = {}, gyro[3] = {};
  uint32_t timestamp = 0;
  bool fresh = false, motionValid = false;
  float rollDegrees = 0, verticalAccelerationG = 0;
};

class MotionSampler {
 public:
  explicit MotionSampler(TwoWire& bus) : bus_(bus) {}

  bool begin() {
    if (!QMI8658_init()) return false;
    for (uint8_t address = 0x6a; address <= 0x6b; ++address) {
      uint8_t id = 0;
      if (read(address, QMI8658Register_WhoAmI, &id, 1) && id == 0x05) {
        address_ = address;
        return true;
      }
    }
    return false;
  }

  MotionSample update(uint32_t now) {
    MotionSample result;
    if (!address_) return result;
    uint8_t stamp[3] = {}, raw[12] = {};
    const bool readOk = read(address_, QMI8658Register_Timestamp_L, stamp, sizeof(stamp)) &&
                        read(address_, QMI8658Register_Ax_L, raw, sizeof(raw));
    result.timestamp = uint32_t(stamp[0]) | (uint32_t(stamp[1]) << 8) | (uint32_t(stamp[2]) << 16);
    result.fresh = readOk && (!haveTimestamp_ || result.timestamp != lastTimestamp_);
    if (haveTimestamp_ && now - lastSuccessMs_ > GravityEstimator::kMaxSampleAgeMs) haveGravity_ = false;
    if (!result.fresh && now - lastSuccessMs_ > 100) haveGravity_ = false;
    if (result.fresh) {
      haveTimestamp_ = true;
      lastTimestamp_ = result.timestamp;
      lastSuccessMs_ = now;
      for (int i = 0; i < 3; ++i) {
        const int16_t acc = int16_t(uint16_t(raw[2*i]) | (uint16_t(raw[2*i+1]) << 8));
        const int16_t gyr = int16_t(uint16_t(raw[6+2*i]) | (uint16_t(raw[7+2*i]) << 8));
        result.acceleration[i] = acc * (1000.0f / 4096.0f);
        result.gyro[i] = gyr / 64.0f;
      }
      result.gravity = estimator_.update(result.timestamp,
          {result.acceleration[0], result.acceleration[1], result.acceleration[2]},
          {result.gyro[0], result.gyro[1], result.gyro[2]}, now);
    }
    if (result.gravity.x != 0 || result.gravity.y != 0 || result.gravity.z != 0) {
      movementGravity_ = result.gravity;
      movementGravityAtMs_ = now;
      haveGravity_ = true;
    }
    if (result.fresh && haveGravity_ && now - movementGravityAtMs_ < 2000) {
      result.verticalAccelerationG = (result.acceleration[0] * movementGravity_.x +
          result.acceleration[1] * movementGravity_.y + result.acceleration[2] * movementGravity_.z) / 1000.0f;
      result.motionValid = isfinite(result.verticalAccelerationG);
      result.rollDegrees = atan2f(movementGravity_.x, movementGravity_.z) * 57.2957795f;
    }
    return result;
  }

 private:
  bool read(uint8_t address, uint8_t reg, uint8_t* data, size_t size) {
    bus_.beginTransmission(address);
    bus_.write(reg);
    if (bus_.endTransmission(false) != 0 || bus_.requestFrom(address, size) != size) return false;
    for (size_t i = 0; i < size; ++i) {
      const int value = bus_.read();
      if (value < 0) return false;
      data[i] = static_cast<uint8_t>(value);
    }
    return true;
  }
  TwoWire& bus_;
  GravityEstimator estimator_;
  MagneticVector movementGravity_{0, 0, 0};
  uint32_t movementGravityAtMs_ = 0, lastTimestamp_ = 0, lastSuccessMs_ = 0;
  uint8_t address_ = 0;
  bool haveGravity_ = false, haveTimestamp_ = false;
};

}  // namespace nina
