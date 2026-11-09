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

  const nina::MovementRenderState bump = movement.update({0.0f, 2.1f}, focused, 50);
  movement.update({0.0f, 0.4f}, focused, 75);
  const nina::MovementRenderState jumped = movement.update({0.0f, 2.1f}, focused, 100);
  const nina::MovementRenderState heldJump = movement.update({0.0f, 2.1f}, focused, 150);
  const nina::MovementRenderState nextColor = movement.update({0.0f, 2.1f}, focused, 300);
  const uint32_t overlayEnd = 100 + nina::kDefaultMovementTuning.jumpOverlayDurationMs;
  const nina::MovementRenderState resumed = movement.update({0.0f, 1.0f}, focused, overlayEnd);

  movement.update({0.0f, 1.0f}, focused, overlayEnd + 100);
  const nina::MovementRenderState debounced = movement.update({0.0f, 2.1f}, focused, overlayEnd + 200);
  movement.update({0.0f, 1.0f}, focused, overlayEnd + 400);
  movement.update({0.0f, 0.4f}, focused, overlayEnd + 450);
  const nina::MovementRenderState nextJump = movement.update({0.0f, 2.1f}, focused, overlayEnd + 500);
  nina::MovementBehavior wrapped;
  const auto idleLate = wrapped.update({0, 1}, idle, 0xfffffff0u);
  wrapped.update({0, 0}, idle, 0xfffffff5u);
  const auto wrappedJump = wrapped.update({0, 2.1f}, idle, 10);
  const auto wrappedEnd = wrapped.update({0, 1}, idle, 10 + nina::kDefaultMovementTuning.jumpOverlayDurationMs);

  return !(expect(!idleLate.jumpOverlayActive && wrappedJump.jumpOverlayActive && !wrappedEnd.jumpOverlayActive,
                  "jump timing remains valid after long idle and clock wrap") &&
           expect(!bump.jumpOverlayActive, "single impact is not a jump") &&
           expect(tilted.preset.name == idle.name, "tilt preserves the underlying expression") &&
           expect(tilted.preset.metrics.eyeOffsetX > idle.metrics.eyeOffsetX,
                  "tilt subtly shifts eyes") &&
           expect(jumped.jumpOverlayActive, "jump starts a transient overlay") &&
           expect(jumped.preset.colors.background.rgb != focused.colors.background.rgb &&
                  nextColor.preset.colors.background.rgb != jumped.preset.colors.background.rgb,
                  "jump cycles background colors") &&
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
