#pragma once

#include <Preferences.h>
#include "Qmc5883p.h"

namespace nina {

struct CompassConfig {
  bool verified;
  uint8_t address;
  CompassConfig(bool busAndChipVerified = false, uint8_t scannedAddress = 0x2C)
      : verified(busAndChipVerified), address(scannedAddress) {}
};

struct CompassHeading {
  bool valid;
  float degrees;
  uint32_t sampledAtMs;
};

class CompassService {
 public:
  bool restore(TwoWire& bus, uint8_t address) {
    Preferences prefs;
    if (!prefs.begin("nina-verified", true)) return false;
    const bool verified = prefs.getUChar("address", 0) == address;
    prefs.end();
    return verified && Qmc5883p::probe(bus, address) && begin(bus, {true, address});
  }

  bool verify(TwoWire& bus, uint8_t address, bool scanned) {
    if (!scanned) return false;
    Preferences prefs;
    if (!prefs.begin("nina-verified", false)) return false;
    const bool forgotten = prefs.isKey("address") && prefs.getUChar("address", 0) == 0;
    prefs.end();
    if ((forgotten && !clearCalibration()) || !begin(bus, {true, address})) return false;
    if (!prefs.begin("nina-verified", false)) { begin(bus); return false; }
    const bool saved = prefs.putUChar("address", address) == 1;
    prefs.end();
    if (!saved) begin(bus);
    return saved;
  }

  bool forgetVerification(TwoWire& bus, bool& calibrationCleared) {
    begin(bus);
    calibrationCleared = false;
    Preferences prefs;
    if (!prefs.begin("nina-verified", false)) return false;
    const bool forgotten = prefs.putUChar("address", 0) == 1;
    prefs.end();
    if (forgotten) calibrationCleared = clearCalibration();
    return forgotten;
  }

  bool begin(TwoWire& bus, CompassConfig config = {}) {
    if (storageOpen_) storage_.end();
    storageOpen_ = false;
    calibration_ = CompassCalibration{};
    collecting_ = false;
    samples_ = 0;
    enabled_ = config.verified && sensor_.begin(bus, config.address);
    valid_ = false;
    if (enabled_) {
      storageOpen_ = storage_.begin("nina-compass", false);
      Record record{};
      if (storageOpen_ && storage_.getBytesLength("cal-v1") == sizeof(record) &&
          storage_.getBytes("cal-v1", &record, sizeof(record)) == sizeof(record) &&
          record.version == kStorageVersion && usable(record.data)) {
        calibration_ = CompassCalibration(record.data);
      }
    }
    return enabled_;
  }

  bool update(uint32_t nowMs, MagneticVector gravityInG) {
    valid_ = false;
    if (!enabled_ || !sensor_.read(raw_)) return false;
    sampledAtMs_ = nowMs;
    if (collecting_) {
      collection_.observe(raw_);
      if (samples_ < UINT32_MAX) ++samples_;
      return true;
    }
    degrees_ = calibration_.headingDegrees(raw_, gravityInG);
    valid_ = std::isfinite(degrees_);
    return true;
  }

  CompassHeading heading(uint32_t nowMs) const {
    return {valid_ && static_cast<uint32_t>(nowMs - sampledAtMs_) <= kHeadingMaxAgeMs,
            degrees_, sampledAtMs_};
  }
  MagneticSample raw() const { return raw_; }
  static constexpr uint32_t kHeadingMaxAgeMs = 250;
  static constexpr uint32_t kCalibrationMinSamples = 120;
  static constexpr int kCalibrationMinSpan = 200;

  bool startCalibration() {
    if (!enabled_) return false;
    collection_ = CompassCalibration{};
    samples_ = 0;
    collecting_ = true;
    valid_ = false;
    return true;
  }

  bool saveCalibration(float headingOffsetDegrees = 0) {
    if (!collecting_ || samples_ < kCalibrationMinSamples || !storageOpen_) return false;
    CompassCalibrationData data = collection_.data();
    data.headingOffsetDegrees = headingOffsetDegrees;
    if (!usable(data)) return false;
    Record record{};
    record.version = kStorageVersion;
    record.data = data;
    if (storage_.putBytes("cal-v1", &record, sizeof(record)) != sizeof(record)) return false;
    calibration_ = CompassCalibration(data);
    collecting_ = false;
    valid_ = false;
    return true;
  }

  void cancelCalibration() { collecting_ = false; valid_ = false; }
  bool calibrated() const { return usable(calibration_.data()); }
  bool collecting() const { return collecting_; }
  uint32_t calibrationSamples() const { return samples_; }

 private:
  static bool clearCalibration() {
    Preferences prefs;
    if (!prefs.begin("nina-compass", false)) return false;
    const bool ok = !prefs.isKey("cal-v1") || prefs.remove("cal-v1");
    prefs.end();
    return ok;
  }
  struct Record { uint32_t version; CompassCalibrationData data; };
  static constexpr uint32_t kStorageVersion = 0x4E430001;
  static bool usable(const CompassCalibrationData& data) {
    return CompassCalibration(data).ready() &&
           static_cast<int>(data.maximum.x) - data.minimum.x >= kCalibrationMinSpan &&
           static_cast<int>(data.maximum.y) - data.minimum.y >= kCalibrationMinSpan &&
           static_cast<int>(data.maximum.z) - data.minimum.z >= kCalibrationMinSpan;
  }
  Preferences storage_;
  Qmc5883p sensor_;
  CompassCalibration calibration_, collection_;
  MagneticSample raw_{};
  bool enabled_ = false, valid_ = false;
  uint32_t sampledAtMs_ = 0;
  float degrees_ = 0;
  bool storageOpen_ = false, collecting_ = false;
  uint32_t samples_ = 0;
};

}  // namespace nina
