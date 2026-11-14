#include "../firmware/src/board/RuntimeInputs.h"
#include "../firmware/src/board/RuntimeSchedule.h"
#include "../firmware/src/compass/CompassService.h"
#include <cassert>
#include <vector>

std::vector<uint8_t> Preferences::bytes;
bool Preferences::failOpen = false, Preferences::failWrite = false, Preferences::failRemove = false;
bool Preferences::verifiedKey = false;
uint8_t Preferences::verifiedAddress = 0;

struct Motion {
  nina::MagneticVector gravity{0, 0, 1};
  float acceleration[3] = {0, 0, 1000}, gyro[3] = {};
  uint32_t timestamp = 1;
  bool fresh = true, motionValid = true;
  float rollDegrees = 0, verticalAccelerationG = 1;
};
struct MotionSource { bool gravityValid = true; Motion update(uint32_t) { Motion m; if (!gravityValid) m.gravity = {0, 0, 0}; return m; } };
struct DiscoverySource {
  std::vector<nina::DiscoveryObservation> queued;
  bool receive(nina::DiscoveryObservation& result) {
    if (queued.empty()) return false;
    result = queued.front(); queued.erase(queued.begin()); return true;
  }
};
struct RadioSource {
  int starts = 0, stops = 0;
  std::vector<nina::RadioPacket> queued;
  bool receive(nina::RadioPacket& result) {
    if (queued.empty()) return false;
    result = queued.front(); queued.erase(queued.begin()); return true;
  }
  void update(uint32_t) {}
  bool begin(uint64_t) { ++starts; return true; }
  void end() { ++stops; }
};
struct Behavior {
  nina::WearableBehavior actual;
  bool lastTouchDown = false;
  std::vector<nina::DirectionObservation> received;
  void cancelTouch() { actual.cancelTouch(); }
  nina::WearableOutput update(const nina::WearableInput& in, const nina::DiscoveryObservation* peers,
                              size_t count, const nina::DirectionObservation* dirs, size_t directions) {
    lastTouchDown = in.touchDown;
    received.insert(received.end(), dirs, dirs + directions);
    return actual.update(in, peers, count, dirs, directions);
  }
};
struct Board {
  uint32_t time = 0, renderCost = 0;
  int touching = 0;
  uint64_t localIdentity = 1;
  nina::ExpressionName expression = nina::ExpressionName::Idle;
  nina::CompassService compass;
  MotionSource motionSampler;
  DiscoverySource discovery;
  RadioSource radio;
  Behavior behavior;
  nina::RuntimeInputs inputs;
  uint32_t now() const { return time; }
  int touch() { return touching; }
  nina::BatterySnapshot battery(uint32_t) { return {}; }
  uint16_t refreshBattery(uint32_t) { return 4000; }
  void sleep() { assert(false); }
  void radioFailure() { assert(false); }
  void logCompass(uint32_t, bool) {}
  void logMovement(uint32_t, const nina::WearableInput&, const Motion&, const nina::WearableOutput&) {}
  void update(uint32_t t) { inputs.update(*this, t); }
  void render() { time += renderCost; }
};

int main() {
  {
    Board touchBoard;
    touchBoard.time = 100; touchBoard.touching = 1; touchBoard.update(100);
    touchBoard.time = 150; touchBoard.touching = -1; touchBoard.update(150);
    touchBoard.time = 400; touchBoard.touching = 1; touchBoard.update(400);
    touchBoard.time = 450; touchBoard.touching = 0; touchBoard.update(450);
    for (int i = 0; i < 4; ++i) {
      touchBoard.time += 50; touchBoard.touching = 1; touchBoard.update(touchBoard.time);
      touchBoard.time += 50; touchBoard.touching = 0; touchBoard.update(touchBoard.time);
    }
    assert(touchBoard.inputs.output().screen == nina::ScreenName::Face);
  }
  {
    Board touchBoard;
    touchBoard.time = 100; touchBoard.touching = 1; touchBoard.update(100);
    touchBoard.time = 150; touchBoard.touching = -1; touchBoard.update(150);
    assert(touchBoard.behavior.lastTouchDown);
    touchBoard.time = 180; touchBoard.touching = 1; touchBoard.update(180);
    touchBoard.time = 400; touchBoard.touching = -1; touchBoard.update(400);
    assert(!touchBoard.behavior.lastTouchDown && !touchBoard.inputs.output().sleepRequested);
    touchBoard.time = 410; touchBoard.touching = 1; touchBoard.update(410);
    touchBoard.time = 420; touchBoard.touching = 0; touchBoard.update(420);
    for (int i = 0; i < 4; ++i) {
      touchBoard.time += 50; touchBoard.touching = 1; touchBoard.update(touchBoard.time);
      touchBoard.time += 50; touchBoard.touching = 0; touchBoard.update(touchBoard.time);
    }
    assert(touchBoard.inputs.output().screen == nina::ScreenName::Face);
  }
  {
    Board touchBoard;
    touchBoard.time = 100; touchBoard.touching = 1; touchBoard.update(100);
    touchBoard.time = 7090; touchBoard.update(7090);
    touchBoard.time = 7100; touchBoard.touching = -1; touchBoard.update(7100);
    assert(!touchBoard.inputs.output().sleepRequested);
  }
  TwoWire bus; bus.registers[0] = 0x80;
  Board b;
  assert(b.compass.begin(bus, {true, 0x2c}));
  assert(b.compass.startCalibration());
  for (unsigned i = 0; i < 120; ++i) {
    bus.sample(i & 1 ? 1000 : -1000, i & 2 ? 1000 : -1000, i & 4 ? 1000 : -1000);
    assert(b.compass.update(i, {0, 0, 1}));
  }
  assert(b.compass.saveCalibration());
  nina::RuntimeSchedule schedule;
  bus.sample(1000, 0, 100);
  b.discovery.queued.push_back({2, -45, 20});
  b.time = 20; schedule.run(b);
  assert(b.inputs.headingValid() && b.inputs.output().targetId == 2 && b.radio.starts == 1);
  bus.registers[9] = 0;
  b.radio.queued.push_back({2, -55, 39});
  b.time = 40; schedule.run(b);
  assert(b.inputs.headingValid() && b.compass.heading(40).sampledAtMs == 20);
  assert(b.behavior.received.size() == 1 && b.behavior.received[0].observedMs == 39);
  b.time = 60; schedule.run(b);
  b.radio.queued.push_back({2, -55, 80});
  b.time = 80; schedule.run(b);
  assert(b.inputs.headingValid() && b.behavior.received.size() == 1); // No fabricated sample at 40 or 60.
  bus.nack = true;
  b.radio.queued.push_back({2, -55, 100});
  b.time = 100; schedule.run(b);
  assert(!b.inputs.headingValid() && b.behavior.received.size() == 1);
  bus.nack = false;
  bus.sample(1000, 0, 100);
  b.renderCost = 55;
  b.time = 113; schedule.run(b);
  assert(b.time == 168);
  schedule.run(b);
  assert(b.compass.heading(168).sampledAtMs == 168);
  bus.registers[9] = 0;
  b.motionSampler.gravityValid = false;
  b.time = 188; schedule.run(b);
  assert(!b.inputs.headingValid());
  b.renderCost = 0;
  for (int i = 1; i <= 10; ++i) {
    b.time = 188 + i * 20;
    b.touching = (i % 2) != 0;
    schedule.run(b);
  }
  assert(b.inputs.output().screen == nina::ScreenName::SettingsBattery);
  b.time = 3020; schedule.run(b);
  assert(!b.inputs.output().targetId && b.radio.stops == 1);
}
