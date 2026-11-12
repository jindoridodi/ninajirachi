#pragma once
#include <cstdint>
#include <cstddef>
struct SerialType { template<class... T> void printf(const char*, T...) {} void println(const char*) {} };
extern SerialType Serial;
inline uint32_t millis() { return 123; }
using esp_err_t = int;
constexpr int ESP_OK = 0;
