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
inline int DEV_Module_Init() { return 0; }
inline bool psramInit() { return true; }
inline void* ps_malloc(size_t size) { return std::malloc(size); }
