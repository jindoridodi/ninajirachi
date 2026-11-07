#pragma once
#include <stdint.h>
#include <math.h>

namespace nina {

enum class Gaze { Left, Center, Right };
constexpr uint32_t kGazeFallbackMs = 2000;
constexpr uint32_t kDirectionSampleMaxAgeMs = 5000;
constexpr int kDirectionMinRssi = -85;
constexpr int kDirectionMinContrastDb = 6;

class DirectionEvidence {
 public:
  void select(uint64_t target) {
    if (target == target_) return;
    target_ = target;
    for (auto& bucket : buckets_) bucket = {};
    lastTurnMs_ = 0;
    hasHeading_ = false;
    hasConfidence_ = false;
    turned_ = false;
  }

  void observe(uint64_t peer, float heading, int rssi, uint32_t now) {
    if (!target_ || peer != target_ || !isfinite(heading) || rssi < -127 || rssi >= 0) return;
    int index = int(normalize(heading + 22.5f) / 45.0f);
    auto& bucket = buckets_[index];
    if (!bucket.count || now - bucket.lastMs >= kDirectionSampleMaxAgeMs) bucket = {};
    if (!bucket.count) bucket.minimum = bucket.maximum = rssi;
    if (rssi < bucket.minimum) bucket.minimum = rssi;
    if (rssi > bucket.maximum) bucket.maximum = rssi;
    if (bucket.count < 8) { bucket.total += rssi; ++bucket.count; }
    else bucket.total = (bucket.total * 7) / 8 + rssi;
    bucket.lastMs = now;
    if (hasHeading_ && fabsf(normalize(heading - lastHeading_ + 180) - 180) >= 15) {
      turned_ = true;
      lastHeading_ = heading;
    } else if (!hasHeading_) { lastHeading_ = heading; hasHeading_ = true; }
    int bestIndex;
    if (turned_ && confident(now, bestIndex)) {
      lastTurnMs_ = now;
      hasConfidence_ = true;
      turned_ = false;
    }
  }

  Gaze gaze(uint32_t now, bool validHeading, float heading) const {
    if (!target_ || !validHeading || !isfinite(heading) || !hasConfidence_ ||
        now - lastTurnMs_ >= kGazeFallbackMs) return Gaze::Center;
    int bestIndex;
    if (!confident(now, bestIndex)) return Gaze::Center;
    float difference = normalize(float(bestIndex * 45) - heading + 180) - 180;
    if (difference < -22.5f) return Gaze::Left;
    if (difference > 22.5f) return Gaze::Right;
    return Gaze::Center;
  }
 private:
  bool confident(uint32_t now, int& bestIndex) const {
    int count = 0, best = -128, worst = 0;
    bestIndex = 0;
    for (int i = 0; i < 8; ++i) {
      const auto& b = buckets_[i];
      if (b.count < 2 || b.maximum - b.minimum > 15 ||
          now - b.lastMs >= kDirectionSampleMaxAgeMs) continue;
      const int average = b.total / b.count;
      ++count;
      if (average > best) { best = average; bestIndex = i; }
      if (average < worst) worst = average;
    }
    return count >= 3 && best >= kDirectionMinRssi && best - worst >= kDirectionMinContrastDb;
  }
  struct Bucket { int total = 0; int minimum = 0; int maximum = 0; uint8_t count = 0; uint32_t lastMs = 0; };
  static float normalize(float value) {
    value = fmodf(value, 360.0f);
    return value < 0 ? value + 360.0f : value;
  }
  Bucket buckets_[8];
  uint64_t target_ = 0;
  uint32_t lastTurnMs_ = 0;
  float lastHeading_ = 0;
  bool hasHeading_ = false;
  bool hasConfidence_ = false;
  bool turned_ = false;
};
}
