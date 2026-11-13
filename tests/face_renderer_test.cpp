#include "../firmware/src/face/FaceRenderer.h"
#include "../firmware/src/behavior/MovementBehavior.h"
#include <cassert>
#include <cstdio>

struct Pixels final : nina::DrawSurface {
  uint32_t pixels[240][240] = {};
  void fillRect(int x, int y, int w, int h, nina::Color c) override {
    for (int row = std::max(0, y); row < std::min(240, y+h); ++row)
      for (int col = std::max(0, x); col < std::min(240, x+w); ++col)
        pixels[row][col] = c.rgb;
  }
};

void preservesReferenceFrames() {
  // Captured from the original renderer before optimizing its geometry work.
  constexpr uint64_t expected[] = {
    13820374116385628818ULL, 15470676808309722200ULL, 9627484902246168821ULL,
    12325516582070104337ULL, 5968290619142240653ULL, 3904333700595754622ULL,
    7671733331946929656ULL,
  };
  nina::FaceRenderer renderer;
  Pixels frame;
  for (int i = 0; i < 7; ++i) {
    auto preset = nina::expressionPreset(static_cast<nina::ExpressionName>(i % 3));
    int size = 240;
    if (i >= 3) {
      constexpr int metrics[][3] = {{1,1,1}, {100,100,20}, {12,70,12}, {80,35,0}};
      preset.metrics.mouthWidth = metrics[i-3][0];
      preset.metrics.mouthHeight = metrics[i-3][1];
      preset.metrics.tongueOut = metrics[i-3][2];
      preset.colors.face = {0x83c723};
      preset.colors.mouth = {0xb65aff};
      if (i == 6) size = 200;
    }
    renderer.render(frame, preset, size, size);
    uint64_t hash = 1469598103934665603ULL;
    for (const auto& row : frame.pixels)
      for (uint32_t pixel : row) hash = (hash ^ pixel) * 1099511628211ULL;
    assert(hash == expected[i]);
  }
}

int main(int argc, char** argv) {
  preservesReferenceFrames();
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
  // Tongue pixels may only extend below the mouth curve, even as it deforms.
  for (int width : {12, 42, 80}) {
    for (int height : {14, 35, 70}) {
      auto expression = nina::idlePreset();
      expression.metrics.mouthWidth = width;
      expression.metrics.mouthHeight = height;
      Pixels normal, tongue;
      renderer.render(normal, expression, 240, 240);
      expression.metrics.tongueOut = 20;
      renderer.render(tongue, expression, 240, 240);
      for (unsigned i = 1; i < sizeof(nina::kFaceMouth)/sizeof(nina::kFaceMouth[0]); ++i) {
        const auto a = nina::faceMouthPoint(nina::kFaceMouth[i-1], expression.metrics);
        const auto b = nina::faceMouthPoint(nina::kFaceMouth[i], expression.metrics);
        for (int x = std::max(0, int(ceilf(a.x))); x < std::min(240, int(ceilf(b.x))); ++x) {
          const float mouthY = a.y + (b.y-a.y) * (x-a.x) / (b.x-a.x);
          for (int y = 135; y + 0.5f < mouthY && y < 240; ++y)
            assert(normal.pixels[y][x] == tongue.pixels[y][x]);
        }
      }
    }
  }
  bool softenedEdge = false;
  for (int y = 140; y < 170; ++y)
    for (int x = 80; x < 160; ++x)
      softenedEdge |= idle.pixels[y][x] != 0 && idle.pixels[y][x] != 0xffffff;
  assert(softenedEdge && "mouth edges should have partial pixel coverage");
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
    assert(frame.pixels[75][69] == jump.preset.colors.background.rgb);
    if (time >= 500) {
      assert(frame.pixels[164][120] == 0);
      assert(frame.pixels[160][113] == 0);
    }
    assert(frame.pixels[170][120] == 0xffffff);
  }
  // Every preset, including extreme authoring values, remains renderable.
  for (auto name : {nina::ExpressionName::Idle, nina::ExpressionName::Curious, nina::ExpressionName::Excited}) {
    Pixels frame;
    const auto& expression = nina::expressionPreset(name);
    renderer.render(frame, expression, 240, 240);
    for (int y = 0; y < 60; ++y)
      for (int x = 0; x < 240; ++x)
        assert(frame.pixels[y][x] == expression.colors.background.rgb);
  }
  preset.metrics.eyeWidth = preset.metrics.eyeOpen = 1;
  renderer.render(gaze, preset, 240, 240);
}
