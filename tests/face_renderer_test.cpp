#include "../firmware/src/face/FaceRenderer.h"
#include <cassert>
#include <cstdio>

struct Pixels final : nina::DrawSurface {
  uint32_t pixels[240][240] = {};
  void beginLayer(nina::FaceLayer) override {}
  void fillCircle(int x, int y, int radius, nina::Color c) override {
    for (int dy = -radius; dy <= radius; ++dy)
      for (int dx = -radius; dx <= radius; ++dx)
        if (dx*dx + dy*dy <= radius*radius) fillRect(x+dx, y+dy, 1, 1, c);
  }
  void fillEllipse(int, int, int, int, nina::Color) override {
    assert(false && "placeholder oval renderer must not be used");
  }
  void fillRect(int x, int y, int w, int h, nina::Color c) override {
    for (int row = std::max(0, y); row < std::min(240, y+h); ++row)
      for (int col = std::max(0, x); col < std::min(240, x+w); ++col)
        pixels[row][col] = c.rgb;
  }
};

int main(int argc, char** argv) {
  nina::FaceRenderer renderer;
  auto preset = nina::idlePreset();
  Pixels idle;
  renderer.render(idle, preset, 240, 240);
  assert(idle.pixels[62][120] == 0x0000EE);  // Flat top.
  assert(idle.pixels[95][69] == 0x0000EE);  // Solid left eye.
  assert(idle.pixels[95][171] == 0x0000EE);
  assert(idle.pixels[120][120] == 0);       // No filled face disc.
  assert(idle.pixels[142][104] == 0x0000EE);// Left mouth lobe.
  assert(idle.pixels[160][120] == 0);
  if (argc == 2) {
    FILE* image = fopen(argv[1], "wb");
    assert(image);
    fprintf(image, "P6\n240 240\n255\n");
    for (const auto& row : idle.pixels)
      for (uint32_t p : row) {
        const unsigned char rgb[] = {static_cast<unsigned char>(p >> 16),
          static_cast<unsigned char>(p >> 8), static_cast<unsigned char>(p)};
        fwrite(rgb, 1, 3, image);
      }
    fclose(image);
  }
  preset.metrics.eyeOpen = 3;
  Pixels blink;
  renderer.render(blink, preset, 240, 240);
  assert(blink.pixels[80][69] == 0 && idle.pixels[80][69] == 0x0000EE);
  preset.metrics.eyeOpen = 42;
  preset.metrics.pupilOffsetX = 10;
  Pixels gaze;
  renderer.render(gaze, preset, 240, 240);
  assert(gaze.pixels[95][60] == 0 && idle.pixels[95][60] == 0x0000EE);
  // Every preset, including extreme authoring values, remains renderable.
  for (int i = 0; i < 5; ++i) {
    Pixels frame;
    renderer.render(frame, nina::expressionPreset(static_cast<nina::ExpressionName>(i)), 240, 240);
  }
  preset.metrics.eyeWidth = preset.metrics.eyeOpen = 1;
  renderer.render(gaze, preset, 240, 240);
}
