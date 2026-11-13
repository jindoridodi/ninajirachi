#pragma once

#include "ExpressionPreset.h"
#include "FaceLayout.h"
#include <math.h>
#include <algorithm>

namespace nina {

class DrawSurface {
 public:
  virtual ~DrawSurface() = default;
  virtual void fillRect(int x, int y, int width, int height, Color color) = 0;
  virtual Color pixel(int x, int y) const = 0;
};

class FaceRenderer {
 public:
  void render(DrawSurface& surface, const ExpressionPreset& preset, int width, int height) const {
    const int ox = (width - 240) / 2, oy = (height - 240) / 2;
    const auto& m = preset.metrics;
    const auto& c = preset.colors;
    surface.fillRect(0, 0, width, height, c.background);
    fillFace(surface, ox, oy, c.face);
    for (unsigned i = 1; i < sizeof(kFaceOutline) / sizeof(kFaceOutline[0]); ++i)
      stroke(surface, kFaceOutline[i-1], kFaceOutline[i], ox, oy, kFaceOutlineWidth, c.face);

    for (const auto& eye : kFaceEyes) {
      const auto layout = faceEye(eye, m);
      roundedRect(surface, ox + layout.x, oy + layout.y, layout.width, layout.height,
                  kFaceEyeRadius, c.eye);
    }
    drawMouth(surface, preset, ox, oy, width, height);
  }

  static void fillFace(DrawSurface& surface, int ox, int oy, Color color) {
    struct Spans {
      int16_t left[240], right[240];
      Spans() {
        for (int y = 0; y < 240; ++y) {
          left[y] = 240; right[y] = -1;
          for (unsigned i = 1; i < sizeof(kFaceOutline) / sizeof(kFaceOutline[0]); ++i) {
            const FacePoint a = kFaceOutline[i-1], b = kFaceOutline[i];
            if ((a.y <= y && b.y > y) || (b.y <= y && a.y > y)) {
              const int x = a.x + (y-a.y)*(b.x-a.x)/(b.y-a.y);
              left[y] = std::min(int(left[y]), x); right[y] = std::max(int(right[y]), x);
            }
          }
        }
      }
    };
    static const Spans spans;
    for (int y = 0; y < 240; ++y)
      if (spans.right[y] >= spans.left[y])
        surface.fillRect(ox + spans.left[y], oy + y, spans.right[y]-spans.left[y]+1, 1, color);
  }

 private:
  static float coverage(float distance) {
    return std::max(0.0f, std::min(1.0f, 0.5f - distance));
  }

  static Color blend(Color foreground, Color background, float alpha) {
    uint32_t rgb = 0;
    for (int shift : {0, 8, 16}) {
      const int a = (foreground.rgb >> shift) & 255, b = (background.rgb >> shift) & 255;
      rgb |= uint32_t(lroundf(b + (a-b)*alpha)) << shift;
    }
    return {rgb};
  }

  static void drawMouth(DrawSurface& surface, const ExpressionPreset& preset,
                        int ox, int oy, int width, int height) {
    MouthPoint points[sizeof(kFaceMouth) / sizeof(kFaceMouth[0])];
    float left = 240, right = 0, top = 240, bottom = 0;
    for (unsigned i = 0; i < sizeof(points)/sizeof(points[0]); ++i) {
      points[i] = faceMouthPoint(kFaceMouth[i], preset.metrics);
      left = std::min(left, points[i].x); right = std::max(right, points[i].x);
      top = std::min(top, points[i].y); bottom = std::max(bottom, points[i].y);
    }
    const float radius = kFaceMouthWidth / 2.0f;
    left -= radius + 1; right += radius + 1;
    top -= radius + 1; bottom += radius + 1;
    const auto tongue = faceTongue(preset.metrics);
    if (tongue.visible) {
      left = std::min(left, float(tongue.x)); right = std::max(right, float(tongue.x + tongue.width));
      bottom = std::max(bottom, float(tongue.y + tongue.height));
    }
    for (int x = std::max(int(floorf(left)), -ox); x < std::min(int(ceilf(right)), width-ox); ++x) {
      const float px = x + 0.5f;
      float mouthY = points[0].y;
      for (unsigned i = 1; i < sizeof(points)/sizeof(points[0]); ++i) {
        const auto a = points[i-1], b = points[i];
        if (a.x <= px && px <= b.x && b.x > a.x) {
          mouthY = a.y + (b.y-a.y)*(px-a.x)/(b.x-a.x);
          break;
        }
      }
      for (int y = std::max(int(floorf(top)), -oy); y < std::min(int(ceilf(bottom)), height-oy); ++y) {
        const float py = y + 0.5f;
        float distanceSquared = 1e9f;
        for (unsigned i = 1; i < sizeof(points)/sizeof(points[0]); ++i) {
          const auto a = points[i-1], b = points[i];
          if (px < std::min(a.x, b.x) - radius - 0.5f ||
              px > std::max(a.x, b.x) + radius + 0.5f) continue;
          const float dx = b.x-a.x, dy = b.y-a.y, lengthSquared = dx*dx + dy*dy;
          const float t = lengthSquared > 0 ? std::max(0.0f, std::min(1.0f,
              ((px-a.x)*dx + (py-a.y)*dy)/lengthSquared)) : 0;
          const float ex = px-a.x-t*dx, ey = py-a.y-t*dy;
          distanceSquared = std::min(distanceSquared, ex*ex + ey*ey);
        }
        const float mouthAlpha = coverage(sqrtf(distanceSquared) - radius);
        float tongueAlpha = 0;
        if (tongue.visible) {
          const float r = std::min(float(tongue.radius), std::min(tongue.width, tongue.height)/2.0f);
          const float dx = fabsf(px - (tongue.x + tongue.width/2.0f)) - (tongue.width/2.0f-r);
          const float dy = fabsf(py - (tongue.y + tongue.height/2.0f)) - (tongue.height/2.0f-r);
          const float outsideX = std::max(0.0f, dx), outsideY = std::max(0.0f, dy);
          const float distance = sqrtf(outsideX*outsideX + outsideY*outsideY) +
              std::min(0.0f, std::max(dx, dy)) - r;
          tongueAlpha = std::min(coverage(distance), coverage(mouthY-py));
        }
        if (mouthAlpha > 0 || tongueAlpha > 0) {
          const Color under = blend({0}, surface.pixel(ox+x, oy+y), tongueAlpha);
          surface.fillRect(ox+x, oy+y, 1, 1, blend(preset.colors.mouth, under, mouthAlpha));
        }
      }
    }
  }

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
