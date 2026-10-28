#pragma once

#include <stdint.h>

namespace nina {

enum class ExpressionName { Idle };

struct Color {
  uint32_t rgb;
};

struct FaceColors {
  Color background;
  Color face;
  Color eye;
  Color eyelid;
  Color pupil;
  Color mouth;
  Color effect;
};

struct FaceMetrics {
  int16_t faceRadius;
  int16_t eyeWidth;
  int16_t eyeOpen;
  int16_t eyelidHeight;
  int16_t pupilRadius;
  int16_t pupilOffsetX;
  int16_t pupilOffsetY;
  int16_t mouthWidth;
  int16_t mouthHeight;
  int16_t effectRadius;
};

struct ExpressionPreset {
  ExpressionName name;
  FaceColors colors;
  FaceMetrics metrics;
};

}  // namespace nina

#include "ExpressionPresets.generated.h"
