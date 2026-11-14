#include "../firmware/src/board/SleepRuntime.h"
#include "../firmware/src/behavior/WearableBehavior.h"
#include <cassert>
#include <string>

struct Fake {
  std::string calls;
  uint32_t time = 0, releaseAt = UINT32_MAX;
  nina::WearableBehavior behavior;
  bool touch = false, irqLow = false;
  uint32_t errorFrom = UINT32_MAX, errorUntil = UINT32_MAX;
  int cause = 0;
  void prepareWake() { calls += "P"; }
  int wakeCause() { return cause; }
  void beginTouch() { calls += "B"; }
  int touchReading() { return time >= errorFrom && time < errorUntil ? -1 : int(touch && time < releaseAt); }
  bool touchIrqLow() { return irqLow; }
  void darken() { calls += "D"; }
  void holdBacklight() { calls += "H"; }
  void disableWake() { calls += "X"; }
  void timerWake() { calls += "T"; }
  void touchWake() { calls += "E"; }
  void deepSleep() { calls += "S"; }
  uint32_t now() { return time; }
  void wait() { time += 20; }
  void suppressTouch() { calls += "I"; behavior.ignoreTouchUntilRelease(); }
  void restoreTouchPower() { calls += "R"; }
};
int main() {
  Fake f; bool waiting = false;
  nina::enterSleep(f, waiting, true);
  assert(waiting && f.calls == "DHXTS");
  f = Fake{}; f.cause = 1; f.touch = true;
  assert(!nina::wakeRuntime(f, waiting, 7000));
  assert(waiting && f.calls == "PBDHXTS");
  f = Fake{}; f.cause = 1;
  assert(!nina::wakeRuntime(f, waiting, 7000));
  assert(!waiting && f.calls == "PBDHXES");
  f = Fake{}; f.cause = 2; f.touch = false;
  assert(!nina::wakeRuntime(f, waiting, 7000));
  assert(waiting && f.calls == "PDBDHXTS");
  f = Fake{}; f.cause = 2; f.touch = true; f.releaseAt = 6980;
  assert(!nina::wakeRuntime(f, waiting, 7000));
  assert(waiting && f.time == 6980 && f.calls == "PDBDHXTS");
  f = Fake{}; f.cause = 2; f.touch = true;
  assert(nina::wakeRuntime(f, waiting, 7000));
  assert(!waiting && f.time == 7000 && f.calls == "PDBIR");
  f = Fake{}; f.cause = 2; f.touch = true; f.errorFrom = 100; f.errorUntil = 200;
  assert(nina::wakeRuntime(f, waiting, 7000));
  f = Fake{}; f.cause = 2; f.touch = true; f.errorFrom = 100; f.errorUntil = 1000;
  assert(!nina::wakeRuntime(f, waiting, 7000));
  f = Fake{}; f.cause = 2; f.touch = true;
  assert(nina::wakeRuntime(f, waiting, 7000));
  nina::WearableInput held(15000, true, nina::ExpressionName::Idle, {});
  assert(!f.behavior.update(held, nullptr, 0).sleepRequested);
  held.touchDown = false; f.behavior.update(held, nullptr, 0);
  held.touchDown = true; held.elapsedMs = 15100; f.behavior.update(held, nullptr, 0);
  held.elapsedMs = 22100; assert(f.behavior.update(held, nullptr, 0).sleepRequested);
}
