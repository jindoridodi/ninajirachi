#include "../firmware/src/board/WaveshareSurface.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>

LCD_1IN28_ATTRIBUTES LCD_1IN28{240, 240, 0};
void LCD_1IN28_Init(UBYTE) {}
void LCD_1IN28_Display(UWORD*) {}

class VendorSurface : public nina::DrawSurface {
 public:
  nina::Color pixel(int x, int y) const override { return readback_.pixel(x, y); }
  void fillRect(int x, int y, int width, int height, nina::Color color) override {
    if (width <= 0 || height <= 0 || x >= 240 || y >= 240) return;
    const int endX = min(239, x + width - 1), endY = min(239, y + height - 1);
    x = max(0, x); y = max(0, y);
    if (endX < x || endY < y) return;
    const unsigned rgb = color.rgb;
    const UWORD pixel = ((rgb >> 8) & 0xf800) | ((rgb >> 5) & 0x07e0) | ((rgb >> 3) & 0x001f);
    Paint_ClearWindows(x, y, endX + 1, endY + 1, pixel);
  }
 private:
  nina::WaveshareSurface readback_;
};

static UBYTE vendorPixels[240 * 240 * 2];
static UBYTE directPixels[240 * 240 * 2];

int main() {
  Paint_NewImage(vendorPixels, 240, 240, ROTATE_0, 0);
  Paint_SetScale(65);
  VendorSurface vendor;
  nina::WaveshareSurface direct;
  nina::FaceRenderer renderer;
  const auto preset = nina::idlePreset();
  Paint_SelectImage(vendorPixels);
  renderer.render(vendor, preset, 240, 240);
  Paint_SelectImage(directPixels);
  renderer.render(direct, preset, 240, 240);
  assert(std::memcmp(vendorPixels, directPixels, sizeof(vendorPixels)) == 0);

  constexpr int frames = 200;
  Paint_SelectImage(vendorPixels);
  auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < frames; ++i) renderer.render(vendor, preset, 240, 240);
  auto middle = std::chrono::steady_clock::now();
  Paint_SelectImage(directPixels);
  for (int i = 0; i < frames; ++i) renderer.render(direct, preset, 240, 240);
  auto end = std::chrono::steady_clock::now();
  assert(std::memcmp(vendorPixels, directPixels, sizeof(vendorPixels)) == 0);
  std::printf("%d identical idle frames: vendor %lld us, direct %lld us\n", frames,
      static_cast<long long>(std::chrono::duration_cast<std::chrono::microseconds>(middle - start).count()),
      static_cast<long long>(std::chrono::duration_cast<std::chrono::microseconds>(end - middle).count()));
}
