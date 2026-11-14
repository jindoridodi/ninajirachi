#pragma once
#include <stdint.h>

namespace nina {
constexpr uint32_t kInputPeriodMs = 20;
constexpr uint32_t kFramePeriodMs = 33;
class RuntimeSchedule {
 public:
  template <typename Runtime>
  void run(Runtime& runtime) {
    uint32_t now = runtime.now();
    if (now - lastInputMs_ >= kInputPeriodMs) {
      lastInputMs_ = now;
      runtime.update(now);
    }
    now = runtime.now();
    if (now - lastFrameMs_ >= kFramePeriodMs) {
      lastFrameMs_ = now;
      runtime.render();
    }
  }
 private:
  uint32_t lastInputMs_ = 0, lastFrameMs_ = 0;
};
}
