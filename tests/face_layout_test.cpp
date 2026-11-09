#include "../firmware/src/face/FaceLayout.h"
#include <cstdio>

int main() {
  int width, open, offsetX, offsetY, mouthWidth, mouthHeight;
  while (scanf("%d %d %d %d %d %d", &width, &open, &offsetX, &offsetY, &mouthWidth, &mouthHeight) == 6) {
    const nina::FaceMetrics metrics = {int16_t(width), int16_t(open), int16_t(offsetX),
                                       int16_t(offsetY), int16_t(mouthWidth), int16_t(mouthHeight)};
    for (const auto& eye : nina::kFaceEyes) {
      const auto result = nina::faceEye(eye, metrics);
      printf("%d %d %d %d ", result.x, result.y, result.width, result.height);
    }
    for (const auto& point : nina::kFaceMouth) {
      const auto result = nina::faceMouthPoint(point, metrics);
      printf("%d %d ", result.x, result.y);
    }
    puts("");
  }
}
