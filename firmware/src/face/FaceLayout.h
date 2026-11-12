#pragma once

#include <algorithm>
#include "ExpressionPreset.h"
#include "FaceGeometry.generated.h"

namespace nina {

struct FaceEye { int x, y, width, height; };

inline FaceEye faceEye(const int16_t (&eye)[4], const FaceMetrics& metrics) {
  const int width = std::max(1, eye[2] * metrics.eyeWidth / 36);
  const int height = std::max(1, eye[3] * metrics.eyeOpen / 42);
  return {eye[0] + (eye[2] - width) / 2 + metrics.eyeOffsetX,
          eye[1] + (eye[3] - height) / 2 + metrics.eyeOffsetY, width, height};
}

inline MouthPoint faceMouthPoint(MouthPoint point, const FaceMetrics& metrics) {
  return {120 + (point.x - 120) * metrics.mouthWidth / 42,
          kFaceMouth[0].y + (point.y - kFaceMouth[0].y) * metrics.mouthHeight / 14};
}

struct FaceTongue { int x, y, width, height, radius; bool visible; };

inline FaceTongue faceTongue(const FaceMetrics& metrics) {
  const auto anchor = faceMouthPoint(kFaceMouth[sizeof(kFaceMouth) / sizeof(kFaceMouth[0]) / 2], metrics);
  return {int(anchor.x) - 9, int(anchor.y) + 8, 18, std::max(1, int(metrics.tongueOut) + 4), 9,
          metrics.tongueOut > 0};
}

}  // namespace nina
