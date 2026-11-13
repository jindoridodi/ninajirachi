#include "../firmware/src/board/WaveshareSurface.h"
#include <cassert>

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
  failPsram = true;
  assert(!failed.begin() && !backlight);
  failPsram = false; failAllocation = true;
  assert(!failed.begin() && !backlight);
  failAllocation = false;
  Paint_NewImage(pixels, 240, 240, ROTATE_0, 0);
  Paint_SetScale(65);
  nina::WaveshareSurface surface;
  surface.fillRect(10, 20, 3, 1, {0xFFFFFF});
  assert(pixel(10, 20) == WHITE && pixel(12, 20) == WHITE);
  assert(pixel(9, 20) == BLACK && pixel(13, 20) == BLACK && pixel(10, 21) == BLACK);
  surface.fillRect(239, 239, 2, 2, {0xFFFFFF});
  assert(pixel(239, 239) == WHITE);
  surface.fillRect(-1, -1, 2, 2, {0xFFFFFF});
  assert(pixel(0, 0) == WHITE && pixel(1, 0) == BLACK && pixel(0, 1) == BLACK);

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
