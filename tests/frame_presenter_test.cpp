#include "../firmware/src/face/FramePresenter.h"
#include "../firmware/src/board/RuntimeSchedule.h"
#include <cassert>
#include <cmath>
#include <cstdio>

struct Display {
  int draws = 0, presents = 0;
  void draw(const nina::WearableOutput&, bool, const char*) { ++draws; }
  void present() { ++presents; }
};

struct Runtime {
  uint32_t time = 0;
  int updates = 0;
  nina::WearableBehavior behavior;
  nina::WearableOutput output{};
  nina::FramePresenter presenter;
  Display display;
  uint32_t now() const { return time; }
  void update(uint32_t t) {
    ++updates;
    nina::WearableInput in(t, false, nina::ExpressionName::Idle, {}, 1);
    nina::DiscoveryObservation peer{2, -40, 20};
    output = behavior.update(in, &peer, t == 20 ? 1 : 0);
  }
  void render() { presenter.present(display, output, false, "CAL UNKNOWN"); }
};

int main() {
  nina::FramePresenter presenter;
  Display display;
  nina::WearableOutput o{};
  o.screen = nina::ScreenName::Face;
  o.face = nina::idlePreset();
  auto show = [&]() { return presenter.present(display, o, true, "CAL SAVED"); };
  assert(show()); assert(!show());
  o.face.colors.background.rgb ^= 1; assert(show());
  o.face.colors.face.rgb ^= 1; assert(show());
  o.face.colors.eye.rgb ^= 1; assert(show());
  o.face.colors.mouth.rgb ^= 1; assert(show());
  ++o.face.metrics.eyeWidth; assert(show());
  ++o.face.metrics.eyeOpen; assert(show());
  ++o.face.metrics.eyeOffsetX; assert(show());
  ++o.face.metrics.eyeOffsetY; assert(show());
  ++o.face.metrics.mouthWidth; assert(show());
  ++o.face.metrics.mouthHeight; assert(show());
  ++o.face.metrics.tongueOut; assert(show()); assert(!show());
  o.screen = nina::ScreenName::SettingsBattery; assert(show()); assert(!show());
  o.battery.millivolts = 4000; assert(show()); assert(!show());
  o.screen = nina::ScreenName::PeerDirection;
  o.relativeDirection = NAN; assert(show()); assert(!show());
  o.targetId = 2; assert(show()); assert(!show());
  assert(presenter.present(display, o, false, "CAL SAVED"));
  assert(presenter.present(display, o, false, "CAL INVALID"));
  o.relativeDirection = 45; assert(show()); assert(!show());
  o.relativeDirection = -45; assert(show());
  o.screen = nina::ScreenName::Face; assert(show());
  assert(display.draws == display.presents);

  Runtime r; nina::RuntimeSchedule schedule;
  for (r.time = 0; r.time <= 3100; ++r.time) schedule.run(r);
  assert(r.updates == 155 && r.output.targetId == 0);
  assert(r.display.presents < 3100 / 33);

  nina::WearableBehavior interaction; nina::FramePresenter peerFrames; Display peerDisplay;
  interaction.toggleDirection(100);
  nina::WearableInput peerInput(100, false, nina::ExpressionName::Idle, {}, 1);
  peerInput.headingValid = true; peerInput.headingDegrees = 90;
  const nina::DiscoveryObservation discovery{2, -40, 100};
  const nina::DirectionObservation directions[] = {
      {2, -40, 0, 100}, {2, -40, 0, 100}, {2, -65, 90, 100},
      {2, -65, 90, 100}, {2, -80, 180, 100}, {2, -80, 180, 100}};
  auto peerOutput = interaction.update(peerInput, &discovery, 1, directions, 6);
  assert(peerOutput.gaze == nina::Gaze::Left);
  assert(peerFrames.present(peerDisplay, peerOutput, true, "CAL SAVED"));
  for (uint32_t t = 200; t < 2100; t += 100) {
    peerInput.elapsedMs = t;
    peerOutput = interaction.update(peerInput, nullptr, 0);
    assert(!peerFrames.present(peerDisplay, peerOutput, true, "CAL SAVED"));
  }
  peerInput.elapsedMs = 2100;
  peerOutput = interaction.update(peerInput, nullptr, 0);
  assert(peerOutput.gaze == nina::Gaze::Center &&
         peerFrames.present(peerDisplay, peerOutput, true, "CAL SAVED"));

  nina::WearableBehavior taps; nina::FramePresenter tapFrames; Display tapDisplay;
  for (uint32_t t = 0; t < 1000; t += 100) {
    nina::WearableInput in(t, (t / 100) % 2 == 0, nina::ExpressionName::Idle, {});
    const auto frame = taps.update(in, nullptr, 0);
    tapFrames.present(tapDisplay, frame, false, "CAL UNKNOWN");
  }
  const auto settings = taps.update({1000, false, nina::ExpressionName::Idle, {}}, nullptr, 0);
  assert(settings.screen == nina::ScreenName::SettingsBattery);
  assert(!tapFrames.present(tapDisplay, settings, false, "CAL UNKNOWN"));
  assert(!tapFrames.present(tapDisplay, settings, false, "CAL UNKNOWN"));
  const auto timedOut = taps.update({7900, false, nina::ExpressionName::Idle, {}}, nullptr, 0);
  assert(timedOut.screen == nina::ScreenName::Face);
  assert(tapFrames.present(tapDisplay, timedOut, false, "CAL UNKNOWN"));
  nina::WearableBehavior idle; nina::FramePresenter replay; Display replayDisplay;
  int frames = 0;
  for (uint32_t t = 0; t < 5600; t += 33) {
    nina::WearableInput in(t, false, nina::ExpressionName::Idle, {}, 1);
    const auto frame = idle.update(in, nullptr, 0);
    replay.present(replayDisplay, frame, false, "CAL UNKNOWN"); ++frames;
  }
  std::printf("idle replay: %d input frames, %d presentations, %d skipped\n", frames,
              replayDisplay.presents, frames - replayDisplay.presents);
  assert(frames == 170 && replayDisplay.presents < frames);
}
