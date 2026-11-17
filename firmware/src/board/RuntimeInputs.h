#pragma once
#include "../compass/CompassService.h"
#include "../behavior/WearableBehavior.h"
#include "../behavior/HeadingHistory.h"

namespace nina {
class RuntimeInputs {
 public:
  template <typename Board>
  void update(Board& board, uint32_t now) {
    const auto motion = board.motionSampler.update(now);
    const bool sampled = board.compass.update(now, motion.gravity);
    const auto sampledHeading = board.compass.heading(now);
    if (sampled || board.compass.readStatus() != CompassReadStatus::NoData || !sampledHeading.valid)
      headingHistory_.record(sampled ? sampledHeading.sampledAtMs : now, sampledHeading.valid, sampledHeading.degrees);
    board.logCompass(now, sampled);

    const int touch = board.touch();
    if ((touch < 0 || touchReadFailed_) && now - lastTouchReadMs_ > 200) {
      touchDown_ = false;
      board.behavior.cancelTouch();
    }
    touchReadFailed_ = touch < 0;
    if (touch >= 0) {
      touchDown_ = touch == 1;
      lastTouchReadMs_ = now;
    }
    WearableInput input(now, touchDown_, board.expression, board.battery(now), board.localIdentity);
    input.touchValid = touch >= 0;
    input.calibrating = board.compass.collecting();
    input.motionValid = motion.motionValid;
    input.accelerationValid = motion.fresh;
    input.accelerationMagnitudeG = sqrtf(motion.acceleration[0] * motion.acceleration[0] +
        motion.acceleration[1] * motion.acceleration[1] + motion.acceleration[2] * motion.acceleration[2]) / 1000.0f;
    input.rollDegrees = motion.rollDegrees;
    input.verticalAccelerationG = motion.verticalAccelerationG;
    DiscoveryObservation observations[8];
    size_t count = 0;
    while (count < 8 && board.discovery.receive(observations[count])) ++count;
    DirectionObservation directions[8];
    size_t directionCount = 0;
    if (radioReady_) {
      board.radio.update(board.now());
      RadioPacket packet;
      while (directionCount < 8 && board.radio.receive(packet)) {
        if (headingHistory_.associate(packet, board.now(), directions[directionCount])) ++directionCount;
      }
    }
    input.elapsedMs = board.now();
    const auto heading = board.compass.heading(input.elapsedMs);
    input.headingValid = heading.valid;
    input.headingDegrees = heading.degrees;
    auto output = board.behavior.update(input, observations, count, directions, directionCount);
    if (calibrationVisible_ && board.compass.collecting() != wasCollecting_)
      calibrationMessage_ = board.compass.collecting() ? "ROTATE ALL AXES" : "HOLD TO START";
    if (!calibrationVisible_ && output.screen == ScreenName::CompassCalibration)
      calibrationMessage_ = board.compass.collecting() ? "ROTATE ALL AXES" : "HOLD TO START";
    if (calibrationVisible_ && output.screen != ScreenName::CompassCalibration && board.compass.collecting())
      board.compass.cancelCalibration();
    if (output.calibrationRequested) {
      if (board.compass.collecting()) {
        if (board.compass.saveCalibration()) calibrationMessage_ = "CAL SAVED";
        else calibrationMessage_ = board.compass.saveStatus() == CalibrationSaveStatus::Storage ? "SAVE FAILED" : "ROTATE MORE";
      } else {
        calibrationMessage_ = board.compass.startCalibration() ? "ROTATE ALL AXES" : "COMPASS NOT READY";
      }
    }
    calibrationVisible_ = output.screen == ScreenName::CompassCalibration;
    wasCollecting_ = board.compass.collecting();
    if (output.sleepRequested) board.sleep();
    if (output.screen == ScreenName::SettingsBattery && !settingsVisible_)
      output.battery.millivolts = board.refreshBattery(input.elapsedMs);
    settingsVisible_ = output.screen == ScreenName::SettingsBattery;
    if (output.peerCount && !radioReady_ &&
        (lastRadioAttemptMs_ == 0 || input.elapsedMs - lastRadioAttemptMs_ >= 3000)) {
      lastRadioAttemptMs_ = input.elapsedMs ? input.elapsedMs : 1;
      radioReady_ = board.radio.begin(board.localIdentity);
      if (!radioReady_) board.radioFailure();
    } else if (!output.peerCount) {
      if (radioReady_) board.radio.end();
      radioReady_ = false;
      lastRadioAttemptMs_ = 0;
    }
    board.logMovement(now, input, motion, output);
    latestOutput_ = output;
    latestHeadingValid_ = input.headingValid;
  }
  const WearableOutput& output() const { return latestOutput_; }
  bool headingValid() const { return latestHeadingValid_; }
  const char* calibrationMessage() const { return calibrationMessage_; }

 private:
  HeadingHistory headingHistory_;
  WearableOutput latestOutput_{};
  uint32_t lastTouchReadMs_ = 0, lastRadioAttemptMs_ = 0;
  bool touchDown_ = false, touchReadFailed_ = false, radioReady_ = false;
  bool settingsVisible_ = false, latestHeadingValid_ = false, calibrationVisible_ = false, wasCollecting_ = false;
  const char* calibrationMessage_ = "HOLD TO START";
};
}
