#pragma once

#include <Arduino.h>
#include <Wire.h>

#include "CompassCalibration.h"

namespace nina {

class Qmc5883p {
 public:
  static bool probe(TwoWire& bus, uint8_t address) {
    bus.beginTransmission(address);
    return bus.endTransmission() == 0;
  }

  bool begin(TwoWire& bus, uint8_t address) {
    bus_ = &bus;
    address_ = address;

    uint8_t chipId = 0;
    if (!readRegister(kChipIdRegister, chipId) || chipId != kExpectedChipId) return false;

    return writeRegister(kControl1Register, kNormalMode);
  }

  bool read(MagneticSample& sample) {
    uint8_t status = 0;
    if (!readRegister(kStatusRegister, status) || (status & kOverflowMask) != 0 ||
        (status & kDataReadyMask) == 0) {
      return false;
    }

    uint8_t bytes[6] = {};
    if (!readRegisters(kDataRegister, bytes, sizeof(bytes))) return false;

    sample = {toInt16(bytes[0], bytes[1]), toInt16(bytes[2], bytes[3]),
              toInt16(bytes[4], bytes[5])};
    return true;
  }

 private:
  static constexpr uint8_t kExpectedChipId = 0x80;
  static constexpr uint8_t kChipIdRegister = 0x00;
  static constexpr uint8_t kDataRegister = 0x01;
  static constexpr uint8_t kStatusRegister = 0x09;
  static constexpr uint8_t kControl1Register = 0x0A;
  static constexpr uint8_t kNormalMode = 0x01;
  static constexpr uint8_t kDataReadyMask = 0x01;
  static constexpr uint8_t kOverflowMask = 0x02;

  static int16_t toInt16(uint8_t low, uint8_t high) {
    return static_cast<int16_t>(static_cast<uint16_t>(low) | (static_cast<uint16_t>(high) << 8));
  }

  bool readRegister(uint8_t registerAddress, uint8_t& value) {
    return readRegisters(registerAddress, &value, 1);
  }

  bool readRegisters(uint8_t registerAddress, uint8_t* values, size_t count) {
    if (bus_ == nullptr) return false;

    bus_->beginTransmission(address_);
    bus_->write(registerAddress);
    if (bus_->endTransmission(false) != 0 || bus_->requestFrom(address_, count) != count) return false;

    for (size_t index = 0; index < count; ++index) values[index] = bus_->read();
    return true;
  }

  bool writeRegister(uint8_t registerAddress, uint8_t value) {
    if (bus_ == nullptr) return false;

    bus_->beginTransmission(address_);
    bus_->write(registerAddress);
    bus_->write(value);
    return bus_->endTransmission() == 0;
  }

  TwoWire* bus_ = nullptr;
  uint8_t address_ = 0;
};

}  // namespace nina
