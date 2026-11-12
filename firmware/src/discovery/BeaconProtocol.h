#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace nina {
enum class BeaconType : uint8_t { Discovery = 1, Direction = 2 };
constexpr size_t kBeaconSize = 12;

inline void encodeBeacon(uint8_t* data, BeaconType type, uint64_t identity) {
  memcpy(data, "NINA", 4);
  data[4] = 1;
  data[5] = static_cast<uint8_t>(type);
  for (int i = 11; i >= 6; --i) { data[i] = identity & 0xff; identity >>= 8; }
}

inline uint64_t decodeBeacon(const uint8_t* data, size_t size, BeaconType type) {
  if (!data || size != kBeaconSize || memcmp(data, "NINA", 4) || data[4] != 1 ||
      data[5] != static_cast<uint8_t>(type)) return 0;
  uint64_t identity = 0;
  for (size_t i = 6; i < kBeaconSize; ++i) identity = (identity << 8) | data[i];
  return identity;
}

struct DiscoveryObservation { uint64_t peerId; int rssi; uint32_t receivedMs; };
}
