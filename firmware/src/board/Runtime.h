#pragma once

namespace nina {
class Runtime {
 public:
  template <typename Surface, typename Initialize>
  bool start(Surface& surface, Initialize initialize) {
    ready_ = surface.begin();
    if (ready_) initialize();
    return ready_;
  }

  template <typename Update, typename Idle>
  void update(Update update, Idle idle) {
    if (ready_) update();
    else idle();
  }

 private:
  bool ready_ = false;
};
}
