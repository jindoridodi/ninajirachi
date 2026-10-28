#include "../firmware/src/face/FaceRenderer.h"

#include <cstdio>

namespace {

struct RecordingSurface final : nina::DrawSurface {
  int backgroundCount = 0;
  int faceCount = 0;
  int eyeCount = 0;
  int eyelidCount = 0;
  int pupilCount = 0;
  int mouthCount = 0;
  int effectCount = 0;
  int backgroundWidth = 0;
  int backgroundHeight = 0;
  int faceX = 0;
  int faceY = 0;
  int faceRadius = 0;
  nina::FaceLayer currentLayer = nina::FaceLayer::Background;

  void beginLayer(nina::FaceLayer layer) override { currentLayer = layer; }
  void fillCircle(int x, int y, int radius, nina::Color) override {
    if (currentLayer == nina::FaceLayer::Face) {
      faceX = x;
      faceY = y;
      faceRadius = radius;
    }
    count();
  }
  void fillEllipse(int, int, int, int, nina::Color) override { count(); }
  void fillRect(int, int, int width, int height, nina::Color) override {
    if (currentLayer == nina::FaceLayer::Background) {
      backgroundWidth = width;
      backgroundHeight = height;
    }
    count();
  }

  void count() {
    switch (currentLayer) {
      case nina::FaceLayer::Background: ++backgroundCount; break;
      case nina::FaceLayer::Face: ++faceCount; break;
      case nina::FaceLayer::Eyes: ++eyeCount; break;
      case nina::FaceLayer::Eyelids: ++eyelidCount; break;
      case nina::FaceLayer::Pupils: ++pupilCount; break;
      case nina::FaceLayer::Mouth: ++mouthCount; break;
      case nina::FaceLayer::Effects: ++effectCount; break;
    }
  }
};

bool expect(bool condition, const char* message) {
  if (condition) return true;
  std::fprintf(stderr, "failed: %s\n", message);
  return false;
}

}  // namespace

int main() {
  const nina::ExpressionPreset& idle = nina::idlePreset();
  RecordingSurface surface;
  nina::FaceRenderer renderer;

  renderer.render(surface, idle, 240, 240);

  return !(expect(idle.name == nina::ExpressionName::Idle, "idle preset is named") &&
           expect(surface.backgroundCount == 1, "renders a background") &&
           expect(surface.backgroundWidth == 240 && surface.backgroundHeight == 240,
                  "fills the 240x240 display") &&
           expect(surface.faceCount == 1, "renders a face shape") &&
           expect(surface.faceX == 120 && surface.faceY == 120 && surface.faceRadius == 112,
                  "centers the idle face") &&
           expect(surface.eyeCount == 2, "renders both eyes") &&
           expect(surface.eyelidCount == 2, "renders both eyelids") &&
           expect(surface.pupilCount == 2, "renders both pupils") &&
           expect(surface.mouthCount == 1, "renders a mouth") &&
           expect(surface.effectCount == 0, "idle has no decorative effect"));
}
