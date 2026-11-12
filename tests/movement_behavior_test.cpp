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
  // A jump has the same jump shape even when it interrupts a blink.
  nina::MovementBehavior blinkJump;
  auto blink = nina::idlePreset();
  blink.metrics.eyeOpen = 3;
  blinkJump.update({0, 0.4f}, blink, 0);
  const auto pop = blinkJump.update({0, 2.1f}, blink, 100);
  const auto opening = blinkJump.update({0, 1}, blink, 175);
  const auto hold = blinkJump.update({0, 1}, blink, 400);
  const auto extended = blinkJump.update({0, 1}, blink, 500);
  const auto fade = blinkJump.update({0, 1}, blink, 1850);
  const auto last = blinkJump.update({0, 1}, blink, 2099);
  if (!expect(pop.preset.metrics.eyeOpen == 3 && opening.preset.metrics.eyeOpen > 3 &&
              opening.preset.metrics.eyeOpen < 48 && hold.preset.metrics.eyeOpen == 48,
              "jump opens blinking eyes smoothly to a restrained expression") ||
      !expect(pop.preset.metrics.mouthWidth == blink.metrics.mouthWidth &&
              pop.preset.metrics.mouthHeight == blink.metrics.mouthHeight && pop.preset.metrics.tongueOut == 0 &&
              opening.preset.metrics.tongueOut > 0 &&
              opening.preset.metrics.tongueOut < hold.preset.metrics.tongueOut &&
              hold.preset.metrics.tongueOut < extended.preset.metrics.tongueOut &&
              extended.preset.metrics.tongueOut == 12,
              "jump preserves the mouth and sticks a tongue out") ||
      !expect(fade.preset.metrics.eyeOpen < hold.preset.metrics.eyeOpen &&
              fade.preset.metrics.eyeOpen > blink.metrics.eyeOpen,
              "jump eases back toward the live underlying expression") ||
      !expect(last.jumpOverlayActive && last.preset.metrics.eyeOpen == blink.metrics.eyeOpen &&
              last.preset.metrics.mouthHeight == blink.metrics.mouthHeight && last.preset.metrics.tongueOut == 0 &&
              last.preset.colors.background.rgb == blink.colors.background.rgb,
              "face and flash reach the underlying expression before overlay ends")) return 1;

  nina::MovementBehavior flashes;
  const auto base = nina::idlePreset();
  flashes.triggerJump(100);
  const auto firstFlash = flashes.update({0, 1}, base, 100).preset.colors.background.rgb;
  const auto secondFlash = flashes.update({0, 1}, base, 200).preset.colors.background.rgb;
  flashes.triggerJump(300);
  const auto nextStart = flashes.update({0, 1}, base, 300).preset.colors.background.rgb;
  const auto nextSecond = flashes.update({0, 1}, base, 400).preset.colors.background.rgb;
  if (!expect(firstFlash != secondFlash && firstFlash != nextStart && nextSecond != secondFlash,
              "jump flashes quickly with a varied color order") ||
      !expect((firstFlash & 0xff0000) == 0xff0000 || (firstFlash & 0xff00) == 0xff00 ||
              (firstFlash & 0xff) == 0xff, "jump colors stay bright")) return 1;

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
           expect(heldJump.preset.metrics.eyeOpen > focused.metrics.eyeOpen,
                  "jump widens the eyes") &&
           expect(jumped.preset.metrics.mouthHeight == focused.metrics.mouthHeight && heldJump.preset.metrics.tongueOut > 0,
                  "jump keeps the mouth and adds a tongue") &&
           expect(heldJump.jumpOverlayActive, "one continuous jump signal stays one overlay") &&
           expect(!resumed.jumpOverlayActive && resumed.preset.metrics.mouthWidth == 64,
                  "overlay resumes the current underlying preset") &&
           expect(!debounced.jumpOverlayActive, "cooldown debounces a second jump") &&
           expect(nextJump.jumpOverlayActive, "a later jump can react again"));
}
