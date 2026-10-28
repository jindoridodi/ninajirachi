#include "src/board/WaveshareSurface.h"

namespace {

nina::WaveshareSurface surface;
nina::FaceRenderer renderer;

}  // namespace

void setup() {
  Serial.begin(115200);
  if (!surface.begin()) {
    Serial.println("Display initialization failed");
    return;
  }
  renderer.render(surface, nina::idlePreset(), 240, 240);
  surface.present();
}

void loop() {}
