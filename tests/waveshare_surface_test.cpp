#include "../firmware/src/board/WaveshareSurface.h"
#include "../firmware/src/board/Runtime.h"
#include <cassert>
#include <climits>

bool failPsram = false, failAllocation = false, backlight = false;
LCD_1IN28_ATTRIBUTES LCD_1IN28{240, 240, 0};
void LCD_1IN28_Init(UBYTE) {}
void LCD_1IN28_Display(UWORD*) {}

static UBYTE pixels[240 * 240 * 2];
static UWORD pixel(int x, int y) {
  const int offset = (y * 240 + x) * 2;
  return (UWORD(pixels[offset]) << 8) | pixels[offset + 1];
}

int main() {
  nina::WaveshareSurface failed;
  nina::Runtime runtime;
  int initialized = 0, updated = 0, idled = 0;
  auto initialize = [&]() { ++initialized; };
  auto update = [&]() { ++updated; };
  auto idle = [&]() { ++idled; };
  failPsram = true;
  assert(!runtime.start(failed, initialize) && !backlight);
  runtime.update(update, idle);
  assert(initialized == 0 && updated == 0 && idled == 1);
  failPsram = false; failAllocation = true;
  assert(!runtime.start(failed, initialize) && !backlight);
  runtime.update(update, idle);
  assert(initialized == 0 && updated == 0 && idled == 2);
  failAllocation = false;
  assert(runtime.start(failed, initialize));
  runtime.update(update, idle);
  assert(initialized == 1 && updated == 1 && idled == 2);
  Paint_NewImage(pixels, 240, 240, ROTATE_0, 0);
  Paint_SetScale(65);
  nina::WaveshareSurface surface;
  surface.fillRect(8, 20, 1, 1, {0xF80000});
  assert(surface.pixel(8, 20).rgb == 0xff0000);
  surface.fillRect(10, 20, 3, 1, {0xFFFFFF});
  assert(pixel(10, 20) == WHITE && pixel(12, 20) == WHITE);
  assert(pixel(9, 20) == BLACK && pixel(13, 20) == BLACK && pixel(10, 21) == BLACK);
  surface.fillRect(239, 239, 2, 2, {0xFFFFFF});
  assert(pixel(239, 239) == WHITE);
  surface.fillRect(-1, -1, 2, 2, {0xFFFFFF});
  assert(pixel(0, 0) == WHITE && pixel(1, 0) == BLACK && pixel(0, 1) == BLACK);

  surface.fillRect(0, 0, 240, 240, {0});
  surface.fillRect(2, 1, INT_MAX, 1, {0xFF0000});
  assert(pixel(1, 1) == BLACK && pixel(2, 1) == 0xf800 && pixel(239, 1) == 0xf800);
  surface.fillRect(0, 2, 1, 1, {0x00FF00});
  surface.fillRect(1, 2, 1, 1, {0x0000FF});
  surface.fillRect(2, 2, 1, 1, {0x123456});
  assert(pixel(0, 2) == 0x07e0 && pixel(1, 2) == 0x001f && pixel(2, 2) == 0x11aa);
  surface.fillRect(-3, 3, 4, 1, {0xffffff});
  surface.fillRect(239, 3, 8, 1, {0xffffff});
  surface.fillRect(-5, 4, 2, 1, {0xffffff});
  surface.fillRect(0, 4, 0, 1, {0xffffff});
  surface.fillRect(0, 4, 1, 0, {0xffffff});
  assert(pixel(0, 3) == WHITE && pixel(1, 3) == BLACK && pixel(239, 3) == WHITE);
  assert(pixel(0, 4) == BLACK);
  surface.fillRect(4, -2, 1, 3, {0x0000ff});
  surface.fillRect(4, 239, 1, INT_MAX, {0x00ff00});
  surface.fillRect(240, 0, 5, 5, {0xffffff});
  surface.fillRect(0, 240, 5, 5, {0xffffff});
  surface.fillRect(0, -5, 5, 2, {0xffffff});
  assert(pixel(4, 0) == 0x001f && pixel(4, 1) == 0xf800 && pixel(4, 239) == 0x07e0);
  assert(pixels[(4 * 2)] == 0x00 && pixels[(4 * 2) + 1] == 0x1f);

  surface.fillRect(0, 0, 240, 240, {0});
  Paint_DrawString_EN(32, 50, "Battery / settings", &Font12, BLACK, WHITE);
  bool textVisible = false;
  for (int y = 50; y < 62 && !textVisible; ++y)
    for (int x = 32; x < 152; ++x)
      textVisible |= pixel(x, y) != BLACK;
  assert(textVisible);

  nina::FaceRenderer renderer;
  auto preset = nina::idlePreset();
  preset.colors.background = preset.colors.face = {0};
  preset.colors.eye = {0xFFFFFF};
  preset.colors.mouth = {0x00FF00};
  renderer.render(surface, preset, 240, 240);
  assert(pixel(69, 111) == WHITE && pixel(171, 111) == WHITE);
  assert(pixel(120, 136) == BLACK);
  assert(pixel(104, 158) == GREEN);
}
