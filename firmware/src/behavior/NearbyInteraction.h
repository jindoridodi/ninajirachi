#pragma once

#include <stdint.h>
#include <stddef.h>
#include "DirectionEvidence.h"
#include "../discovery/BeaconProtocol.h"

namespace nina {

constexpr uint32_t kPeerGraceMs = 3000;
constexpr uint32_t kSustainedProximityMs = 10000;
constexpr int kTargetSwitchMarginDb = 6;
constexpr size_t kMaxPeers = 4;

struct NearbyInteractionState {
  size_t peerCount;
  uint64_t targetId;
  bool sustainedProximity;
  Gaze gaze;
  float relativeDirection;
};

class NearbyInteraction {
 public:
  NearbyInteractionState update(uint32_t now, uint64_t localId,
                                const DiscoveryObservation* observations, size_t count,
                                bool headingValid, float headingDegrees,
                                const DirectionObservation* directions = nullptr, size_t directionCount = 0) {
    updatePeers(now, localId, observations, count);
    direction_.select(targetId_);
    for (size_t i = 0; i < directionCount; ++i)
      direction_.observe(directions[i].peerId, directions[i].headingDegrees,
                         directions[i].rssi, directions[i].observedMs);
    const float direction = direction_.relativeDirection(now, headingValid, headingDegrees);
    return {peerCount_, targetId_, targetId_ && now - targetSinceMs_ >= kSustainedProximityMs,
            DirectionEvidence::gaze(direction), direction};
  }

 private:
  struct Peer { uint64_t id; uint32_t seen; int rssi; };

  static uint64_t observationId(const DiscoveryObservation& observation, uint32_t now, uint64_t localId) {
    return now - observation.receivedMs < 0x80000000u && observation.rssi < 0 &&
        observation.rssi >= -127 && observation.peerId != localId ? observation.peerId : 0;
  }

  void updatePeers(uint32_t now, uint64_t localId, const DiscoveryObservation* observations, size_t count) {
    for (size_t i = 0; i < kMaxPeers; ++i) {
      Peer& peer = peers_[i];
      bool hasRecentObservation = false;
      for (size_t j = 0; peer.id && j < count; ++j) {
        const auto& observation = observations[j];
        if (now - observation.receivedMs < kPeerGraceMs &&
            observationId(observation, now, localId) == peer.id) {
          hasRecentObservation = true;
          break;
        }
      }
      if (peer.id && now - peer.seen >= kPeerGraceMs && !hasRecentObservation) {
        if (peer.id == targetId_) { targetId_ = 0; direction_.select(0); }
        peer.id = 0;
      }
    }
    for (size_t j = 0; j < count; ++j) {
      const auto& observation = observations[j];
      const uint64_t id = observationId(observation, now, localId);
      if (!id) continue;
      Peer* slot = nullptr;
      for (size_t i = 0; i < kMaxPeers; ++i) {
        if (peers_[i].id == id) { slot = &peers_[i]; break; }
        if (!peers_[i].id && !slot) slot = &peers_[i];
      }
      if (slot && !slot->id && now - observation.receivedMs >= kPeerGraceMs) continue;
      if (slot && (!slot->id || now - observation.receivedMs <= now - slot->seen)) {
        if (slot->id && observation.receivedMs - slot->seen >= kPeerGraceMs && id == targetId_) {
          targetId_ = 0; direction_.select(0);
        }
        *slot = {id, observation.receivedMs, observation.rssi};
      }
    }
    for (size_t i = 0; i < kMaxPeers; ++i) {
      Peer& peer = peers_[i];
      if (peer.id && now - peer.seen >= kPeerGraceMs) {
        if (peer.id == targetId_) { targetId_ = 0; direction_.select(0); }
        peer.id = 0;
      }
    }
    Peer* current = nullptr;
    Peer* strongest = nullptr;
    peerCount_ = 0;
    for (size_t i = 0; i < kMaxPeers; ++i) {
      Peer& peer = peers_[i];
      if (!peer.id) continue;
      ++peerCount_;
      if (peer.id == targetId_) current = &peer;
      if (!strongest || peer.rssi > strongest->rssi) strongest = &peer;
    }
    if (!current || (strongest && strongest->rssi >= current->rssi + kTargetSwitchMarginDb)) {
      targetId_ = strongest ? strongest->id : 0;
      targetSinceMs_ = now;
    }
  }

  Peer peers_[kMaxPeers] = {};
  DirectionEvidence direction_;
  size_t peerCount_ = 0;
  uint64_t targetId_ = 0;
  uint32_t targetSinceMs_ = 0;
};

}  // namespace nina
