#pragma once
#include <cstring>
#include <vector>
#include "Arduino.h"

class Preferences {
 public:
  static std::vector<uint8_t> bytes;
  static bool failOpen, failWrite;
  bool begin(const char*, bool = false) { return !failOpen; }
  size_t getBytesLength(const char*) { return bytes.size(); }
  size_t getBytes(const char*, void* data, size_t size) {
    if (size != bytes.size()) return 0;
    std::memcpy(data, bytes.data(), size);
    return size;
  }
  size_t putBytes(const char*, const void* data, size_t size) {
    if (failWrite) return 0;
    bytes.assign(static_cast<const uint8_t*>(data), static_cast<const uint8_t*>(data) + size);
    return size;
  }
  bool remove(const char*) {
    if (failWrite) return false;
    bytes.clear();
    return true;
  }
  void end() {}
};
