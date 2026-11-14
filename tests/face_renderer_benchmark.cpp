#include "../firmware/src/face/FaceRenderer.h"
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

struct Pixels : nina::DrawSurface {
  uint32_t pixels[240][240] = {};
  nina::Color pixel(int x, int y) const override { return {pixels[y][x]}; }
  void fillRect(int x, int y, int w, int h, nina::Color color) override {
    for (int row = std::max(0, y); row < std::min(240, y+h); ++row)
      for (int col = std::max(0, x); col < std::min(240, x+w); ++col)
        pixels[row][col] = color.rgb;
  }
};

int main(int argc, char** argv) {
  const int repetitions = argc > 1 ? std::atoi(argv[1]) : 1;
  if (repetitions < 1) return 1;
  nina::FaceRenderer renderer;
  Pixels frame;
  uint64_t hash = 1469598103934665603ULL;
  const auto start = std::chrono::steady_clock::now();
  for (int repeat = 0; repeat < repetitions; ++repeat) {
    for (int name = 0; name < 3; ++name) {
      for (int width : {1, 42, 100}) for (int height : {1, 35, 100}) for (int tongue : {0, 20}) {
        auto preset = nina::expressionPreset(static_cast<nina::ExpressionName>(name));
        preset.metrics.mouthWidth = width;
        preset.metrics.mouthHeight = height;
        preset.metrics.tongueOut = tongue;
        preset.metrics.eyeOffsetY = tongue ? 45 : 0;
        preset.colors.background = {0x123456};
        preset.colors.face = {0x83c723};
        preset.colors.eye = {0xff4500};
        preset.colors.mouth = {0xb65aff};
        renderer.render(frame, preset, 240, 240);
        for (const auto& row : frame.pixels)
          for (uint32_t pixel : row) hash = (hash ^ pixel) * 1099511628211ULL;
      }
    }
  }
  const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now() - start).count();

  if (repetitions == 1 && hash != 14833825985166619602ULL) return 1;
  std::printf("%d renders: %lld us; hash %llu\n", repetitions * 54,
              static_cast<long long>(elapsed), static_cast<unsigned long long>(hash));
}
