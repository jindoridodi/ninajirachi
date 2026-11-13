#pragma once

#include <Wire.h>

namespace nina {

// Read the touch count directly to detect presses and releases.
inline bool touchIsDown() {
  Wire.beginTransmission(0x15);
  Wire.write(0x02);
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(0x15, 1) != 1) return false;
  return (Wire.read() & 0x0f) != 0;
}

inline void resetTouch() {
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  delay(10);
  digitalWrite(13, HIGH);
  delay(50);
}

inline uint16_t batteryMillivolts(uint32_t nowMs, bool refresh = false) {
  static uint16_t voltage = 0;
  static uint32_t sampledAtMs = 0;
  static bool sampled = false;
  if (refresh || !sampled || nowMs - sampledAtMs >= 1000) {
    // The documented divider is 200k/100k; use calibrated ADC millivolts.
    voltage = static_cast<uint16_t>(analogReadMilliVolts(1) * 3);
    sampledAtMs = nowMs;
    sampled = true;
  }
  return voltage;
}

}  // namespace nina
