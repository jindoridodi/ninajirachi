#pragma once

#include "ExpressionPreset.h"
#include "FaceLayout.h"
#include <math.h>
#include <algorithm>

namespace nina {

enum class FaceLayer { Background, Face, Eyes, Mouth };

class DrawSurface {
 public:
  virtual ~DrawSurface() = default;
  virtual void beginLayer(FaceLayer layer) = 0;
  virtual void fillRect(int x, int y, int width, int height, Color color) = 0;
};

class FaceRenderer {
 public:
  void render(DrawSurface& surface, const ExpressionPreset& preset, int width, int height) const {
    const int ox = (width - 240) / 2, oy = (height - 240) / 2;
    const auto& m = preset.metrics;
    const auto& c = preset.colors;
    surface.beginLayer(FaceLayer::Background);
    surface.fillRect(0, 0, width, height, c.background);
    surface.beginLayer(FaceLayer::Face);
    fillFace(surface, ox, oy, c.face);
    for (unsigned i = 1; i < sizeof(kFaceOutline) / sizeof(kFaceOutline[0]); ++i)
      stroke(surface, kFaceOutline[i-1], kFaceOutline[i], ox, oy, kFaceOutlineWidth, c.face);

    surface.beginLayer(FaceLayer::Eyes);
    for (const auto& eye : kFaceEyes) {
      const auto layout = faceEye(eye, m);
      roundedRect(surface, ox + layout.x, oy + layout.y, layout.width, layout.height,
                  kFaceEyeRadius, c.eye);
    }
    surface.beginLayer(FaceLayer::Mouth);
    drawTongue(surface, preset.metrics, ox, oy);
    FacePoint previous = faceMouthPoint(kFaceMouth[0], m);
    for (unsigned i = 1; i < sizeof(kFaceMouth) / sizeof(kFaceMouth[0]); ++i) {
      const FacePoint next = faceMouthPoint(kFaceMouth[i], m);
      stroke(surface, previous, next, ox, oy, kFaceMouthWidth, c.mouth);
      previous = next;
    }
  }

  static void drawTongue(DrawSurface& surface, const FaceMetrics& metrics, int ox, int oy) {
    const auto tongue = faceTongue(metrics);
    if (tongue.visible)
      roundedRect(surface, ox + tongue.x, oy + tongue.y, tongue.width, tongue.height,
                  tongue.radius, {0});
  }

  static void fillFace(DrawSurface& surface, int ox, int oy, Color color) {
    for (int y = 0; y < 240; ++y) {
      int left = 240, right = -1;
      for (unsigned i = 1; i < sizeof(kFaceOutline) / sizeof(kFaceOutline[0]); ++i) {
        const FacePoint a = kFaceOutline[i-1], b = kFaceOutline[i];
        if ((a.y <= y && b.y > y) || (b.y <= y && a.y > y)) {
          const int x = a.x + (y-a.y)*(b.x-a.x)/(b.y-a.y);
          left = std::min(left, x); right = std::max(right, x);
        }
      }
      if (right >= left) surface.fillRect(ox + left, oy + y, right-left+1, 1, color);
    }
  }

 private:
  static void roundedRect(DrawSurface& s, int x, int y, int w, int h, int radius, Color c) {
    const float r = std::min(float(radius), std::min(w, h) / 2.0f);
    for (int row = 0; row < h; ++row) {
      const float dy = std::max(0.0f, r - std::min(row + 0.5f, h - row - 0.5f));
      const int inset = int(ceilf(r - sqrtf(std::max(0.0f, r*r - dy*dy)) - 0.5f));
      s.fillRect(x + inset, y + row, w - inset*2, 1, c);
    }
  }

  static void stroke(DrawSurface& s, FacePoint a, FacePoint b, int ox, int oy,
                     int width, Color c) {
    const int steps = std::max(1, std::max(abs(b.x-a.x), abs(b.y-a.y)));
    for (int step = 0; step <= steps; ++step) {
      const int x = ox + a.x + (b.x-a.x)*step/steps;
      const int y = oy + a.y + (b.y-a.y)*step/steps;
      if (width == 1) { s.fillRect(x, y, 1, 1, c); continue; }
      const float r = width / 2.0f;
      for (int row = 0; row < width; ++row) {
        const float dy = row + 0.5f - r;
        const int span = int(sqrtf(r*r - dy*dy));
        s.fillRect(x - span, y + row - width/2, span*2 + 1, 1, c);
      }
    }
  }
};

}  // namespace nina
