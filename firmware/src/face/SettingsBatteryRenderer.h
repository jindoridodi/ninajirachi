#pragma once

#include <stdio.h>
#include "../behavior/WearableBehavior.h"

namespace nina {

struct SettingsBatteryLines {
  char lines[7][48];
};

// The board draws these lines using its existing text API.
inline SettingsBatteryLines settingsBatteryLines(const SettingsBatteryDisplay& settings) {
  SettingsBatteryLines result{};
  const auto& battery = settings.battery;
  snprintf(result.lines[0], 48, "Battery / settings");
  if (battery.hasVoltage && battery.millivolts > 0)
    snprintf(result.lines[1], 48, "Voltage: %u.%03u V", unsigned(battery.millivolts / 1000),
             unsigned(battery.millivolts % 1000));
  else
    snprintf(result.lines[1], 48, "Voltage: Unknown");
  snprintf(result.lines[2], 48, "Charge: %s", battery.chargingKnown ?
           (battery.charging ? "Yes" : "No") : "Unknown");
  if (battery.hasEstimatedPercent && battery.estimatedPercent <= 100)
    snprintf(result.lines[3], 48, "Battery: %u%%", unsigned(battery.estimatedPercent));
  else
    snprintf(result.lines[3], 48, "Battery: Uncalibrated");
  snprintf(result.lines[4], 48, "ID: %.20s", battery.deviceId ? battery.deviceId : "Unknown");
  snprintf(result.lines[5], 48, "FW: %.20s", battery.firmwareId ? battery.firmwareId : "Unknown");
  snprintf(result.lines[6], 48, "Hold 3s to return");
  return result;
}

}  // namespace nina
