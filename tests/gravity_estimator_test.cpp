#include "../firmware/src/compass/GravityEstimator.h"
#include <cassert>
#include <cmath>
#include <limits>

int main() {
  // A smooth pitch follows the gyro and gravity together; it should not lose gravity.
  nina::GravityEstimator pitch;
  assert(pitch.update(1, {0, 0, 1000}, {0, 0, 0}, 0).z == 0);
  assert(pitch.update(2, {0, 0, 1000}, {0, 0, 0}, 250).z == 1);
  const auto pitched = pitch.update(3, {-52, 0, 999}, {0, 30, 0}, 350);
  assert(pitched.x < -0.04f && pitched.z > 0.99f);
  pitch.update(4, {-105, 0, 995}, {0, 30, 0}, 450);
  pitch.update(5, {-156, 0, 988}, {0, 30, 0}, 550);
  const auto sustained = pitch.update(6, {-208, 0, 978}, {0, 30, 0}, 650);
  assert(std::fabs(sustained.x + 0.208f) < 0.01f);

  nina::GravityEstimator fastYaw;
  fastYaw.update(1, {0, 0, 1000}, {0, 0, 0}, 0);
  fastYaw.update(2, {0, 0, 1000}, {0, 0, 0}, 250);
  assert(fastYaw.update(3, {0, 0, 1000}, {0, 0, 500}, 350).z > 0.99f);
  assert(fastYaw.update(4, {0, 0, 1000}, {500, 0, 0}, 450).z == 0);

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
  assert(estimator.update(7, level, {0, 11, 0}, 800).z > 0.99f);
  assert(estimator.update(8, level, quiet, 820).z > 0.99f);
  assert(estimator.update(9, level, quiet, 1070).z > 0.99f);
  assert(estimator.update(10, {0, 0, 1200}, quiet, 1100).z == 0);
  assert(estimator.update(11, level, quiet, 1120).z == 0);
  assert(estimator.update(12, level, quiet, 1370).z == 1);
  assert(estimator.update(13, {600, 0, 800}, quiet, 1400).z > 0.99f);
  assert(estimator.update(14, {600, 0, 800}, quiet, 1600).z > 0.99f);
  assert(estimator.update(15, {600, 0, 800}, quiet, 1621).z == 0); // Uncorrected estimate expires.

  const float nan = std::numeric_limits<float>::quiet_NaN();
  assert(estimator.update(16, {nan, 0, 1000}, quiet, 1640).z == 0);
  assert(estimator.update(17, level, {0, nan, 0}, 1660).z == 0);
  assert(estimator.update(18, {0, 0, 0}, quiet, 1680).z == 0);

  nina::GravityEstimator turning;
  const nina::MagneticVector tilted{600, 0, 800}, yaw{12, 0, 16};
  assert(turning.update(1, tilted, yaw, 0).z == 0);
  const auto down = turning.update(2, tilted, yaw, 250);
  assert(std::fabs(down.x - 0.6f) < 0.001f && std::fabs(down.z - 0.8f) < 0.001f);
  assert(turning.update(3, tilted, {16, 0, -12}, 280).z > 0.79f);
  assert(turning.update(4, tilted, yaw, 300).z > 0.79f);
  assert(turning.update(5, tilted, yaw, 550).z > 0.79f);

  nina::GravityEstimator wrapped;
  assert(wrapped.update(0xFFFFFF, level, quiet, UINT32_MAX - 100).z == 0);
  assert(wrapped.update(0, level, quiet, 149).z == 1);
  assert(wrapped.update(1, level, quiet, 400).z == 0);  // Gap over 250 ms.
}
