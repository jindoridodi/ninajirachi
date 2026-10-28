#pragma once

#include <Wire.h>
#include "../waveshare/QMI8658.h"

namespace nina {

// Read the touch count directly: interrupt events alone cannot measure a held finger.
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

inline uint16_t batteryMillivolts() {
  // The documented divider is 200k/100k; calibrated ADC millivolts avoid assuming 3.3V full scale.
  return static_cast<uint16_t>(analogReadMilliVolts(1) * 3);
}

}  // namespace nina
