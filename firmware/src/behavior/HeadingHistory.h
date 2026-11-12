#pragma once
#include <stdint.h>
#include "DirectionEvidence.h"
#include "../discovery/BeaconProtocol.h"

namespace nina {

class HeadingHistory {
 public:
  void record(uint32_t time, bool valid, float degrees) {
    samples_[next_] = {time, degrees, valid};
    next_ = (next_ + 1) % 16;
    if (count_ < 16) ++count_;
  }

  bool associate(const RadioPacket& packet, uint32_t now, DirectionObservation& observation) const {
    if (now - packet.receivedMs > 250) return false;
    float heading;
    if (!at(packet.receivedMs, heading)) return false;
    observation = {packet.peer, packet.rssi, heading, packet.receivedMs};
    return true;
  }

 private:
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

  struct Sample { uint32_t time; float degrees; bool valid; };
  Sample samples_[16] = {};
  unsigned next_ = 0, count_ = 0;
};

}
