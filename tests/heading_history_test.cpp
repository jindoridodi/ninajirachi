#include "../firmware/src/behavior/HeadingHistory.h"
#include <cassert>

int main() {
  nina::HeadingHistory history;
  nina::DirectionObservation observation{};
  assert(!history.associate({2, -45, 100}, 100, observation));
  history.record(100, true, 10);
  history.record(200, true, 90);
  // Processing is delayed; the packet must retain its receive-time heading and timestamp.
  assert(history.associate({2, -45, 105}, 300, observation));
  assert(observation.peerId == 2 && observation.rssi == -45 &&
         observation.headingDegrees == 10 && observation.observedMs == 105);
  assert(history.associate({3, -60, 195}, 300, observation));
  assert(observation.peerId == 3 && observation.headingDegrees == 90);
  assert(!history.associate({2, -45, 150}, 300, observation));
  assert(history.associate({2, -45, 100}, 350, observation));
  assert(!history.associate({2, -45, 100}, 351, observation));
  assert(!history.associate({2, -45, 200}, 199, observation));
  history.record(240, false, 0);
  assert(!history.associate({2, -45, 239}, 300, observation));
  nina::HeadingHistory tolerance;
  tolerance.record(100, true, 10);
  assert(tolerance.associate({2, -45, 140}, 140, observation));
  assert(!tolerance.associate({2, -45, 141}, 141, observation));
  nina::HeadingHistory wrapped;
  wrapped.record(0xfffffff0u, true, 270);
  assert(wrapped.associate({2, -45, 5}, 20, observation));
  assert(observation.headingDegrees == 270 && observation.observedMs == 5);
  assert(!wrapped.associate({2, -45, 0xfffffff0u}, 235, observation));
}
