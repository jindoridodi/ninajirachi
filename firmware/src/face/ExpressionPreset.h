#pragma once

#include <stdint.h>

namespace nina {

enum class ExpressionName { Idle, Curious, Happy, Surprised, Excited };

struct Color {
  uint32_t rgb;
};

struct FaceColors {
  Color background;
  Color face;
  Color eye;
  Color mouth;
  Color effect;
};

struct FaceMetrics {
  int16_t eyeWidth;
  int16_t eyeOpen;
  int16_t eyeOffsetX;
  int16_t eyeOffsetY;
  int16_t mouthWidth;
  int16_t mouthHeight;
};

struct ExpressionPreset {
  ExpressionName name;
  FaceColors colors;
  FaceMetrics metrics;
};

}  // namespace nina

#include "ExpressionPresets.generated.h"
