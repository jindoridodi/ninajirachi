#pragma once

#include <stdint.h>

namespace nina {

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

}  // namespace nina
