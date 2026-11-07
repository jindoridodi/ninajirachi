#pragma once
#include "Arduino.h"
#include <array>
#include <vector>

class TwoWire {
 public:
  std::array<uint8_t, 256> registers{};
  std::vector<uint8_t> tx;
  uint8_t cursor = 0;
  bool nack = false, shortRead = false, badRead = false, ignoreWrites = false;
  unsigned transactions = 0;
  void beginTransmission(uint8_t) { ++transactions; tx.clear(); }
  size_t write(uint8_t value) { tx.push_back(value); return 1; }
  uint8_t endTransmission(bool = true) {
    if (nack) return 2;
    if (!tx.empty()) cursor = tx[0];
    if (tx.size() == 2 && !ignoreWrites) registers[cursor] = tx[1];
    return 0;
  }
  size_t requestFrom(uint8_t, size_t count) { return shortRead ? count - 1 : count; }
  int read() { return badRead ? -1 : registers[cursor++]; }
  void sample(int16_t x, int16_t y, int16_t z) {
    const int16_t values[]{x, y, z};
    for (size_t i = 0; i < 3; ++i) {
      registers[1 + 2 * i] = static_cast<uint16_t>(values[i]) & 255;
      registers[2 + 2 * i] = static_cast<uint16_t>(values[i]) >> 8;
    }
    registers[9] = 1;
  }
};
