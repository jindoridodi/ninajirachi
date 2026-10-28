#include "../firmware/src/behavior/MovementBehavior.h"

#include <cstdio>

namespace {

bool expect(bool condition, const char* message) {
  if (condition) return true;
  std::fprintf(stderr, "failed: %s\n", message);
  return false;
}

}  // namespace

int main() {
  nina::MovementBehavior movement;
  const nina::ExpressionPreset& idle = nina::idlePreset();

  const nina::MovementRenderState tilted = movement.update({10.0f, 1.0f}, idle, 0);

  nina::ExpressionPreset focused = idle;
  focused.metrics.mouthWidth = 64;

  const nina::MovementRenderState jumped = movement.update({0.0f, 2.1f}, focused, 100);
  const nina::MovementRenderState heldJump = movement.update({0.0f, 2.1f}, focused, 150);
  const nina::MovementRenderState resumed = movement.update({0.0f, 1.0f}, focused, 500);

  movement.update({0.0f, 1.0f}, focused, 600);
  const nina::MovementRenderState debounced = movement.update({0.0f, 2.1f}, focused, 700);
  movement.update({0.0f, 1.0f}, focused, 900);
  const nina::MovementRenderState nextJump = movement.update({0.0f, 2.1f}, focused, 1000);

  return !(expect(tilted.preset.name == idle.name, "tilt preserves the underlying expression") &&
           expect(tilted.preset.metrics.pupilOffsetX > idle.metrics.pupilOffsetX,
                  "tilt subtly shifts pupils") &&
           expect(jumped.jumpOverlayActive, "jump starts a transient overlay") &&
           expect(jumped.preset.colors.background.rgb == focused.colors.effect.rgb,
                  "jump flashes the background") &&
           expect(jumped.preset.metrics.eyeOpen > focused.metrics.eyeOpen,
                  "jump shows surprised eyes") &&
           expect(jumped.preset.metrics.mouthHeight > focused.metrics.mouthHeight,
                  "jump opens the mouth") &&
           expect(heldJump.jumpOverlayActive, "one continuous jump signal stays one overlay") &&
           expect(!resumed.jumpOverlayActive && resumed.preset.metrics.mouthWidth == 64,
                  "overlay resumes the current underlying preset") &&
           expect(!debounced.jumpOverlayActive, "cooldown debounces a second jump") &&
           expect(nextJump.jumpOverlayActive, "a later jump can react again"));
}
