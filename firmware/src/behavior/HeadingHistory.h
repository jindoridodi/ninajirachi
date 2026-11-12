#pragma once
#include <stdint.h>

namespace nina {

class HeadingHistory {
 public:
  void record(uint32_t time, bool valid, float degrees) {
    samples_[next_] = {time, degrees, valid};
    next_ = (next_ + 1) % 16;
    if (count_ < 16) ++count_;
  }

  bool at(uint32_t time, float& degrees) const {
    uint32_t closest = 41;
    const Sample* match = nullptr;
    for (unsigned i = 0; i < count_; ++i) {
      const uint32_t delta = time - samples_[i].time;
      const uint32_t distance = delta < 0x80000000u ? delta : 0u - delta;
      if (distance < closest) { closest = distance; match = &samples_[i]; }
    }
    if (!match || !match->valid) return false;
    degrees = match->degrees;
    return true;
  }

 private:
  struct Sample { uint32_t time; float degrees; bool valid; };
  Sample samples_[16] = {};
  unsigned next_ = 0, count_ = 0;
};

}
