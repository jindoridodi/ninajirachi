#pragma once
#include <stdint.h>

namespace nina {
// Wake causes are supplied by the board adapter: 1 timer, 2 touch, 0 otherwise.
template <typename Board>
void enterSleep(Board& board, bool& waitingForRelease, bool waitForRelease) {
  board.darken();
  board.holdBacklight();
  board.disableWake();
  waitingForRelease = waitForRelease;
  // The IRQ can pulse during a hold; check release before arming touch wake.
  if (waitForRelease) board.timerWake();
  else board.touchWake();
  board.deepSleep();
}

template <typename Board>
bool wakeRuntime(Board& board, bool& waitingForRelease, uint32_t holdMs) {
  board.prepareWake();
  const int cause = board.wakeCause();
  if (cause == 1 && waitingForRelease) {
    board.beginTouch();
    enterSleep(board, waitingForRelease, board.touchReading() != 0 || board.touchIrqLow());
    return false;
  }
  waitingForRelease = false;
  if (cause == 2) {
    board.darken();
    board.beginTouch();
    const uint32_t started = board.now();
    uint32_t lastValid = started;
    while (true) {
      const int touch = board.touchReading();
      if (touch == 1) lastValid = board.now();
      if (touch == 0 || uint32_t(board.now() - lastValid) > 200) {
        enterSleep(board, waitingForRelease, true);
        return false;
      }
      if (touch == 1 && uint32_t(board.now() - started) >= holdMs) break;
      board.wait();
    }
    board.suppressTouch();
    board.restoreTouchPower();
  }
  return true;
}
}
