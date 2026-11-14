#pragma once
#include "../compass/CompassService.h"
#include "../behavior/WearableBehavior.h"
#include "../behavior/HeadingHistory.h"

namespace nina {
template <typename Board>
void updateRuntimeInputs(Board& board, uint32_t now) {
  const auto motion = board.motionSampler.update(now);
  const bool sampled = board.compass.update(now, motion.gravity);
  const auto sampledHeading = board.compass.heading(now);
  if (sampled || board.compass.readStatus() != CompassReadStatus::NoData || !sampledHeading.valid)
    board.headingHistory.record(sampled ? sampledHeading.sampledAtMs : now, sampledHeading.valid, sampledHeading.degrees);
  board.logCompass(now, sampled);

  const int touch = board.touch();
  if ((touch < 0 || board.touchReadFailed) && now - board.lastTouchReadMs > 200) {
    board.touchDown = false;
    board.behavior.cancelTouch();
  }
  board.touchReadFailed = touch < 0;
  if (touch >= 0) {
    board.touchDown = touch == 1;
    board.lastTouchReadMs = now;
  }
  WearableInput input(now, board.touchDown, board.expression, board.battery(now), board.localIdentity);
  input.touchValid = touch >= 0;
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
  if (board.radioReady) {
    board.radio.update(board.now());
    RadioPacket packet;
    while (directionCount < 8 && board.radio.receive(packet)) {
      if (board.headingHistory.associate(packet, board.now(), directions[directionCount])) ++directionCount;
    }
  }
  input.elapsedMs = board.now();
  const auto heading = board.compass.heading(input.elapsedMs);
  input.headingValid = heading.valid;
  input.headingDegrees = heading.degrees;
  auto output = board.behavior.update(input, observations, count, directions, directionCount);
  if (output.sleepRequested) board.sleep();
  if (output.screen == ScreenName::SettingsBattery && !board.settingsVisible)
    output.battery.millivolts = board.refreshBattery(input.elapsedMs);
  board.settingsVisible = output.screen == ScreenName::SettingsBattery;
  if (output.peerCount && !board.radioReady &&
      (board.lastRadioAttemptMs == 0 || input.elapsedMs - board.lastRadioAttemptMs >= 3000)) {
    board.lastRadioAttemptMs = input.elapsedMs ? input.elapsedMs : 1;
    board.radioReady = board.radio.begin(board.localIdentity);
    if (!board.radioReady) board.radioFailure();
  } else if (!output.peerCount) {
    if (board.radioReady) board.radio.end();
    board.radioReady = false;
    board.lastRadioAttemptMs = 0;
  }
  board.logMovement(now, input, motion, output);
  board.latestOutput = output;
  board.latestHeadingValid = input.headingValid;
}
}
