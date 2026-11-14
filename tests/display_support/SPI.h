#pragma once
#include <cstdint>
#include <vector>
#define VSPI 0
#define MSBFIRST 0
#define SPI_MODE0 0
struct SPISettings { SPISettings(int, int, int) {} };
extern std::vector<uint8_t> sent;
extern std::vector<uint32_t> writes;
class SPIClass {
 public:
  SPIClass(int = 0) {}
  void begin(int, int, int, int) {}
  int pinSS() { return 0; }
  void beginTransaction(SPISettings) {}
  void end() {}
  uint8_t transfer(uint8_t v) { return v; }
  void transfer(void* data, uint32_t size) {
    auto* bytes = static_cast<uint8_t*>(data);
    sent.insert(sent.end(), bytes, bytes + size);
    writes.push_back(size);
    for (uint32_t i = 0; i < size; ++i) bytes[i] = 0;
  }
  void writeBytes(const uint8_t* bytes, uint32_t size) {
    sent.insert(sent.end(), bytes, bytes + size);
    writes.push_back(size);
  }
};
