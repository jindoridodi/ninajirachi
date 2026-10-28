#include "src/board/WaveshareSurface.h"
#include "src/behavior/WearableBehavior.h"

namespace {

nina::WaveshareSurface surface;
nina::FaceRenderer renderer;
nina::WearableBehavior behavior;

}  // namespace

void setup() {
  Serial.begin(115200);
  if (!surface.begin()) {
    Serial.println("Display initialization failed");
    return;
  }
}

void loop() {
  const nina::BatterySnapshot battery = {0, false, false, 0, "prototype"};
  const nina::WearableOutput output = behavior.update(
      {millis(), false, nina::ExpressionName::Idle, battery});
  if (output.screen == nina::ScreenName::Face) {
    renderer.render(surface, output.face, 240, 240);
    surface.present();
  }
}
