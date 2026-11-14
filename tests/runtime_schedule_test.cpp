#include "../firmware/src/board/RuntimeSchedule.h"
#include "../firmware/src/behavior/WearableBehavior.h"
#include "../firmware/src/behavior/HeadingHistory.h"
#include <cassert>
#include <vector>

struct FakeRuntime {
  uint32_t time = 0, renderCost = 0;
  std::vector<uint32_t> sampled, rendered;
  nina::WearableBehavior behavior;
  nina::HeadingHistory headings;
  nina::WearableOutput output{};
  bool touch = false;
  uint32_t now() const { return time; }
  void update(uint32_t now) {
    sampled.push_back(now);
    nina::WearableInput input(now, touch, nina::ExpressionName::Idle, {}, 1);
    const nina::DiscoveryObservation peer{2, -50, 20};
    output = behavior.update(input, &peer, now == 20 ? 1 : 0);
    headings.record(now, now == 20, 90);
  }
  void render() { rendered.push_back(time); time += renderCost; }
};

int main() {
  nina::RuntimeSchedule schedule;
  FakeRuntime r;
  for (r.time = 0; r.time <= 100; ++r.time) schedule.run(r);
  assert((r.sampled == std::vector<uint32_t>{20, 40, 60, 80, 100}));
  assert((r.rendered == std::vector<uint32_t>{33, 66, 99}));
  assert(r.output.targetId == 2);
  nina::DirectionObservation direction{};
  assert(!r.headings.associate({2, -45, 100}, 100, direction));
  r.time = 3020; schedule.run(r);
  assert(r.output.targetId == 0);

  nina::RuntimeSchedule gestures;
  FakeRuntime tapped;
  for (int i = 1; i <= 10; ++i) {
    tapped.time = i * 20;
    tapped.touch = (i % 2) != 0;
    gestures.run(tapped);
  }
  assert(tapped.output.screen == nina::ScreenName::SettingsBattery);
  tapped.behavior.triggerJump(200);
  tapped.time = 220; gestures.run(tapped);
  assert(tapped.output.jumpOverlayActive);

  nina::RuntimeSchedule slow;
  FakeRuntime blocked;
  blocked.renderCost = 55;
  blocked.time = 20; slow.run(blocked);
  blocked.time = 33; slow.run(blocked);
  assert(blocked.time == 88);
  slow.run(blocked);
  assert((blocked.sampled == std::vector<uint32_t>{20, 88}));
  assert((blocked.rendered == std::vector<uint32_t>{33, 88}));
  assert(blocked.time == 143);
  slow.run(blocked);
  assert(blocked.sampled.back() == 143);
  assert(blocked.rendered.back() == 143);

  nina::RuntimeSchedule wrapped;
  FakeRuntime acrossWrap;
  acrossWrap.time = 0xfffffff0u; wrapped.run(acrossWrap);
  acrossWrap.time = 5; wrapped.run(acrossWrap);
  assert((acrossWrap.sampled == std::vector<uint32_t>{0xfffffff0u, 5}));
  assert((acrossWrap.rendered == std::vector<uint32_t>{0xfffffff0u}));
  acrossWrap.time = 17; wrapped.run(acrossWrap);
  assert((acrossWrap.rendered == std::vector<uint32_t>{0xfffffff0u, 17}));
}
