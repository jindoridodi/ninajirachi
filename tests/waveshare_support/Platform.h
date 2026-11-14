#pragma once

// Replace only hardware access; GUI_Paint.cpp remains the unmodified vendor rasterizer.
#define _DEV_CONFIG_H_
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#define UBYTE uint8_t
#define UWORD uint16_t
#define UDOUBLE uint32_t
using std::min;
using std::max;
extern bool failPsram, failAllocation, backlight;
#define LOW 0
#define OUTPUT 1
inline void digitalWrite(int, int value) { backlight = value != LOW; }
inline void pinMode(int, int) {}
inline int DEV_Module_Init() { backlight = true; return 0; }
inline bool psramInit() { return !failPsram; }
inline void* ps_malloc(size_t size) { return failAllocation ? nullptr : std::malloc(size); }
