#pragma once
#include <cstring>
#include <vector>
#include "Arduino.h"

class Preferences {
 public:
  static std::vector<uint8_t> bytes;
  static bool failOpen, failWrite;
  static bool failRemove, verifiedKey;
  static uint8_t verifiedAddress;
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
    if (failWrite || failRemove) return false;
    bytes.clear();
    return true;
  }
  bool isKey(const char* key) { return std::strcmp(key, "address") == 0 ? verifiedKey : !bytes.empty(); }
  uint8_t getUChar(const char*, uint8_t fallback = 0) { return verifiedKey ? verifiedAddress : fallback; }
  size_t putUChar(const char*, uint8_t value) {
    if (failWrite) return 0;
    verifiedKey = true;
    verifiedAddress = value;
    return 1;
  }
  void end() {}
};
