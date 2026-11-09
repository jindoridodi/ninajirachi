#include "../firmware/src/face/FaceRenderer.h"
#include "../firmware/src/behavior/MovementBehavior.h"
#include <cassert>
#include <cstdio>

struct Pixels final : nina::DrawSurface {
  uint32_t pixels[240][240] = {};
  void beginLayer(nina::FaceLayer) override {}
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
  assert(idle.pixels[78][120] == 0xffffff);
  assert(idle.pixels[111][69] == 0);
  assert(idle.pixels[111][171] == 0);
  assert(idle.pixels[136][120] == 0xffffff);
  assert(idle.pixels[158][104] == 0);
  assert(idle.pixels[176][120] == 0xffffff);
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
  assert(blink.pixels[96][69] == 0xffffff && idle.pixels[96][69] == 0);
  preset.metrics.eyeOpen = 42;
  preset.metrics.eyeOffsetX = 10;
  Pixels gaze;
  renderer.render(gaze, preset, 240, 240);
  assert(gaze.pixels[111][60] == 0xffffff && idle.pixels[111][60] == 0);
  nina::MovementBehavior movement;
  movement.update({0, 0.4f}, preset, 0);
  for (uint32_t time = 100; time < 1300; time += 200) {
    const auto jump = movement.update({0, 2.1f}, preset, time);
    Pixels frame;
    renderer.render(frame, jump.preset, 240, 240);
    assert(jump.jumpOverlayActive && frame.pixels[0][0] != 0 && frame.pixels[0][0] != 0xffffff);
    assert(frame.pixels[136][120] == 0xffffff);
  }
  // Every preset, including extreme authoring values, remains renderable.
  for (int i = 0; i < 5; ++i) {
    Pixels frame;
    const auto& expression = nina::expressionPreset(static_cast<nina::ExpressionName>(i));
    renderer.render(frame, expression, 240, 240);
    for (int y = 0; y < 60; ++y)
      for (int x = 0; x < 240; ++x)
        assert(frame.pixels[y][x] == expression.colors.background.rgb);
  }
  preset.metrics.eyeWidth = preset.metrics.eyeOpen = 1;
  renderer.render(gaze, preset, 240, 240);
}
