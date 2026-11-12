#include "../firmware/src/compass/GravityEstimator.h"
#include <cassert>
#include <cmath>
#include <limits>

int main() {
  nina::GravityEstimator estimator;
  const nina::MagneticVector level{0, 0, 1000}, quiet{0, 0, 0};
  assert(estimator.update(1, level, quiet, 0).z == 0);
  assert(estimator.update(2, level, quiet, 100).z == 0);
  assert(estimator.update(3, level, quiet, 250).z == 1);
  const auto filtered = estimator.update(4, {50, 0, 999}, quiet, 280);
  assert(filtered.x > 0 && filtered.x < 0.05f && filtered.z > 0.99f);

  assert(estimator.update(4, level, quiet, 281).z == 0);  // Duplicate.
  assert(estimator.update(4, level, quiet, 531).z == 0);  // Stale.
  assert(estimator.update(5, level, quiet, 532).z == 0);  // Must settle again.
  assert(estimator.update(6, level, quiet, 782).z == 1);
  assert(estimator.update(7, level, {0, 11, 0}, 800).z == 0);  // Rotation at 1 g.
  assert(estimator.update(8, level, quiet, 820).z == 0);
  assert(estimator.update(9, level, quiet, 1070).z == 1);
  assert(estimator.update(10, {0, 0, 1200}, quiet, 1100).z == 0);
  assert(estimator.update(11, level, quiet, 1120).z == 0);
  assert(estimator.update(12, level, quiet, 1370).z == 1);
  assert(estimator.update(13, {600, 0, 800}, quiet, 1400).z == 0);  // Direction jump at 1 g.

  const float nan = std::numeric_limits<float>::quiet_NaN();
  assert(estimator.update(14, {nan, 0, 1000}, quiet, 1420).z == 0);
  assert(estimator.update(15, level, {0, nan, 0}, 1440).z == 0);
  assert(estimator.update(16, {0, 0, 0}, quiet, 1460).z == 0);

  nina::GravityEstimator turning;
  const nina::MagneticVector tilted{600, 0, 800}, yaw{12, 0, 16};
  assert(turning.update(1, tilted, yaw, 0).z == 0);
  const auto down = turning.update(2, tilted, yaw, 250);
  assert(std::fabs(down.x - 0.6f) < 0.001f && std::fabs(down.z - 0.8f) < 0.001f);
  assert(turning.update(3, tilted, {16, 0, -12}, 280).z == 0);  // Rotation across gravity.
  assert(turning.update(4, tilted, yaw, 300).z == 0);
  assert(turning.update(5, tilted, yaw, 550).z > 0.79f);  // Must settle again.

  nina::GravityEstimator wrapped;
  assert(wrapped.update(0xFFFFFF, level, quiet, UINT32_MAX - 100).z == 0);
  assert(wrapped.update(0, level, quiet, 149).z == 1);
  assert(wrapped.update(1, level, quiet, 400).z == 0);  // Gap over 250 ms.
}
