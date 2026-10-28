#pragma once

#include "ExpressionPreset.h"
#include <math.h>
#include <algorithm>

namespace nina {

enum class FaceLayer { Background, Face, Eyes, Eyelids, Pupils, Mouth, Effects };

class DrawSurface {
 public:
  virtual ~DrawSurface() = default;
  virtual void beginLayer(FaceLayer layer) = 0;
  virtual void fillCircle(int x, int y, int radius, Color color) = 0;
  virtual void fillEllipse(int x, int y, int width, int height, Color color) = 0;
  virtual void fillRect(int x, int y, int width, int height, Color color) = 0;
};

class FaceRenderer {
 public:
  void render(DrawSurface& surface, const ExpressionPreset& preset, int width, int height) const {
    const int centerX = width / 2;
    const int centerY = height / 2;
    const int eyeY = centerY - 28;
    const int leftEyeX = centerX - 48;
    const int rightEyeX = centerX + 48;
    const FaceMetrics& metrics = preset.metrics;
    const FaceColors& colors = preset.colors;

    surface.beginLayer(FaceLayer::Background);
    surface.fillRect(0, 0, width, height, colors.background);

    surface.beginLayer(FaceLayer::Face);
    surface.fillCircle(centerX, centerY, metrics.faceRadius, colors.face);

    surface.beginLayer(FaceLayer::Eyes);
    surface.fillEllipse(leftEyeX, eyeY, metrics.eyeWidth, metrics.eyeOpen, colors.eye);
    surface.fillEllipse(rightEyeX, eyeY, metrics.eyeWidth, metrics.eyeOpen, colors.eye);

    surface.beginLayer(FaceLayer::Eyelids);
    surface.fillRect(leftEyeX - metrics.eyeWidth / 2, eyeY - metrics.eyeOpen / 2,
                     metrics.eyeWidth, metrics.eyelidHeight, colors.eyelid);
    surface.fillRect(rightEyeX - metrics.eyeWidth / 2, eyeY - metrics.eyeOpen / 2,
                     metrics.eyeWidth, metrics.eyelidHeight, colors.eyelid);

    surface.beginLayer(FaceLayer::Pupils);
    if (metrics.pupilRadius > 0 && metrics.eyeWidth > 0 && metrics.eyeOpen > 3) {
      for (int eyeX = leftEyeX; eyeX <= rightEyeX; eyeX += rightEyeX - leftEyeX) {
        for (int row = -metrics.pupilRadius; row <= metrics.pupilRadius; ++row) {
          const int y = eyeY + metrics.pupilOffsetY + row;
          if (y < eyeY - metrics.eyeOpen / 2 + metrics.eyelidHeight) continue;
          const float ratio = float(y - eyeY) / (metrics.eyeOpen / 2.0f);
          if (fabsf(ratio) > 1) continue;
          const int eyeSpan = int(metrics.eyeWidth / 2.0f * sqrtf(1 - ratio * ratio));
          const int pupilSpan = int(sqrtf(float(metrics.pupilRadius * metrics.pupilRadius - row * row)));
          const int left = std::max(eyeX - eyeSpan, eyeX + metrics.pupilOffsetX - pupilSpan);
          const int right = std::min(eyeX + eyeSpan, eyeX + metrics.pupilOffsetX + pupilSpan);
          if (left <= right) surface.fillRect(left, y, right - left + 1, 1, colors.pupil);
        }
      }
    }

    surface.beginLayer(FaceLayer::Mouth);
    surface.fillEllipse(centerX, centerY + 54, metrics.mouthWidth, metrics.mouthHeight,
                        colors.mouth);

    surface.beginLayer(FaceLayer::Effects);
    if (metrics.effectRadius > 0) {
      surface.fillCircle(centerX, centerY - 88, metrics.effectRadius, colors.effect);
    }
  }
};

}  // namespace nina
