// Replay a captured IMU trace through GravityEstimator.
// Each line is: nowMs sensorTimestamp axMg ayMg azMg gxDps gyDps gzDps recordedFlag
// recordedFlag is kept from the capture and is not used.
// Exits 1 when more than one fifth of the lines have no gravity estimate.

#include "../firmware/src/compass/GravityEstimator.h"

#include <cstdio>

int main() {
  nina::GravityEstimator estimator;
  unsigned frames = 0;
  unsigned unavailable = 0;
  unsigned nowMs = 0, timestamp = 0, recordedFlag = 0;
  float ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0;
  while (std::scanf("%u %u %f %f %f %f %f %f %u", &nowMs, &timestamp, &ax, &ay, &az, &gx, &gy, &gz,
                    &recordedFlag) == 9) {
    const nina::MagneticVector gravity =
        estimator.update(timestamp, {ax, ay, az}, {gx, gy, gz}, nowMs);
    ++frames;
    if (gravity.x == 0 && gravity.y == 0 && gravity.z == 0) ++unavailable;
  }
  const double percent = frames == 0 ? 0 : 100.0 * unavailable / frames;
  std::printf("frames=%u gravity_unavailable=%u (%.1f%%)\n", frames, unavailable, percent);
  return unavailable * 5 > frames ? 1 : 0;
}
