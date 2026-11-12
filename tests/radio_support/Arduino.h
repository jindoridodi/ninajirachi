#pragma once
#include <cstdint>
#include <cstddef>
struct SerialType { int messages = 0; template<class... T> void printf(const char*, T...) { ++messages; } void println(const char*) { ++messages; } };
extern SerialType Serial;
inline uint32_t millis() { return 123; }
using esp_err_t = int;
constexpr int ESP_OK = 0;
