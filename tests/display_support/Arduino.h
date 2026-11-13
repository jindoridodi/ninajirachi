#pragma once
#include <cstdint>
#include <cstdio>
using byte = uint8_t;
using uint = unsigned int;
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
inline void digitalWrite(uint16_t, uint8_t) {}
inline int digitalRead(uint16_t) { return 0; }
inline void pinMode(uint16_t, uint16_t) {}
inline void analogWrite(uint8_t, int) {}
inline int analogReadMilliVolts(int) { return 0; }
inline void delay(uint32_t) {}
inline void delayMicroseconds(uint32_t) {}
struct SerialType { void begin(int) {} };
extern SerialType Serial;
