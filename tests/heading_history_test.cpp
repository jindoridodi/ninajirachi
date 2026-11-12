#include "../firmware/src/behavior/HeadingHistory.h"
#include <cassert>

int main() {
  nina::HeadingHistory history;
  float heading = 0;
  history.record(100, true, 10);
  history.record(200, true, 90);
  assert(history.at(105, heading) && heading == 10);
  assert(history.at(195, heading) && heading == 90);
  assert(!history.at(150, heading));
  history.record(240, false, 0);
  assert(!history.at(240, heading));
  history.record(0xfffffff0u, true, 270);
  assert(history.at(5, heading) && heading == 270);
}
