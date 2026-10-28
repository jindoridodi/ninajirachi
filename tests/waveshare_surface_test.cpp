#include "../firmware/src/board/WaveshareSurface.h"
#include <cassert>

LCD_1IN28_ATTRIBUTES LCD_1IN28{240, 240, 0};
void LCD_1IN28_Init(UBYTE) {}
void LCD_1IN28_Display(UWORD*) {}

static UBYTE pixels[240 * 240 * 2];
static UWORD pixel(int x, int y) {
  const int offset = (y * 240 + x) * 2;
  return (UWORD(pixels[offset]) << 8) | pixels[offset + 1];
}

int main() {
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

  nina::FaceRenderer renderer;
  auto preset = nina::idlePreset();
  preset.colors.background = preset.colors.face = preset.colors.eyelid = {0};
  preset.colors.eye = {0xFFFFFF};
  preset.colors.pupil = {0xFF0000};
  preset.colors.mouth = {0x00FF00};
  renderer.render(surface, preset, 240, 240);
  assert(pixel(72, 92) == RED && pixel(168, 92) == RED);
  assert(pixel(56, 92) == WHITE && pixel(152, 92) == WHITE);
  assert(pixel(120, 174) == GREEN);
}
