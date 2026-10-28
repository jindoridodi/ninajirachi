#include "../firmware/src/compass/CompassCalibration.h"

#include <cmath>
#include <cstdio>

namespace {

bool expect(bool condition, const char* message) {
  if (condition) return true;
  std::fprintf(stderr, "failed: %s\n", message);
  return false;
}

bool near(float actual, float expected, float tolerance = 0.01f) {
  return std::fabs(actual - expected) <= tolerance;
}

}  // namespace

int main() {
  nina::CompassCalibration calibration;

  calibration.observe({10, 20, -30});
  calibration.observe({30, 60, 10});

  const nina::MagneticVector centered = calibration.apply({20, 40, -10});
  const nina::CompassCalibration restored(calibration.data());

  // The tilted field projects to equal forward/right components (45 degrees).
  nina::CompassCalibration unit({{-1000, -1000, -1000}, {1000, 1000, 1000}, 0});
  if (!expect(near(unit.headingDegrees({500, 707, -500}, {0.70710678f, 0, 0.70710678f}),
                        45, 0.02f), "pitch compensation preserves heading") ||
      !expect(std::isnan(unit.headingDegrees({1000, 0, 0}, {0, 0, 0})),
              "missing gravity invalidates heading") ||
      !expect(std::isnan(nina::CompassCalibration{}.headingDegrees({1, 2, 3})),
              "uncalibrated heading is invalid")) return 1;

  return !(expect(calibration.ready(), "calibration becomes usable after all axes vary") &&
           expect(near(centered.x, 0) && near(centered.y, 0) && near(centered.z, 0),
                  "calibration removes the measured hard-iron center") &&
           expect(near(calibration.headingDegrees({30, 40, -10}), 0),
                  "positive calibrated X is 0 degrees") &&
           expect(near(calibration.headingDegrees({20, 60, -10}), 90),
                  "positive calibrated Y is 90 degrees") &&
           expect(near(calibration.headingDegrees({10, 40, -10}), 180),
                  "heading is normalized into a full circle") &&
           expect(near(restored.headingDegrees({20, 60, -10}), 90),
                  "saved calibration data can be loaded for one wearable") &&
           expect(!nina::CompassCalibration{}.ready(), "an empty calibration cannot produce a heading"));
}
