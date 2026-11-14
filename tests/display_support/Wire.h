#pragma once
#include <cstdint>
class TwoWire {
 public:
  void beginTransmission(uint8_t) {}
  void write(uint8_t) {}
  void write(const uint8_t*, uint32_t) {}
  void endTransmission() {}
  void requestFrom(uint8_t, uint32_t) {}
  uint8_t read() { return 0; }
  void setPins(int, int) {}
  void setClock(int) {}
  void begin() {}
  void end() {}
};
extern TwoWire Wire;
