#pragma once

#include <math.h>

#include "../face/FaceRenderer.h"
#include "../waveshare/DEV_Config.h"
#include "../waveshare/GUI_Paint.h"
#include "../waveshare/LCD_1in28.h"

namespace nina {

class WaveshareSurface final : public DrawSurface {
 public:
  bool begin() {
    if (DEV_Module_Init() != 0) return false;

    image_ = static_cast<UWORD*>(ps_malloc(LCD_1IN28_HEIGHT * LCD_1IN28_WIDTH * sizeof(UWORD)));
    if (image_ == nullptr) return false;

    LCD_1IN28_Init(HORIZONTAL);
    Paint_NewImage(reinterpret_cast<UBYTE*>(image_), LCD_1IN28.WIDTH, LCD_1IN28.HEIGHT, 0, 0);
    Paint_SetScale(65);
    Paint_SetRotate(ROTATE_0);
    return true;
  }

  void present() { LCD_1IN28_Display(image_); }

  void beginLayer(FaceLayer) override {}

  void fillCircle(int x, int y, int radius, Color color) override {
    Paint_DrawCircle(x, y, radius, rgb565(color), DOT_PIXEL_1X1, DRAW_FILL_FULL);
  }

  void fillEllipse(int x, int y, int width, int height, Color color) override {
    const float radiusX = width / 2.0f;
    const float radiusY = height / 2.0f;
    for (int row = -height / 2; row <= height / 2; ++row) {
      const float ratio = row / radiusY;
      const int span = static_cast<int>(radiusX * sqrtf(1.0f - ratio * ratio));
      Paint_DrawRectangle(x - span, y + row, x + span, y + row, rgb565(color), DOT_PIXEL_1X1,
                          DRAW_FILL_FULL);
    }
  }

  void fillRect(int x, int y, int width, int height, Color color) override {
    Paint_DrawRectangle(x, y, x + width - 1, y + height - 1, rgb565(color), DOT_PIXEL_1X1,
                        DRAW_FILL_FULL);
  }

 private:
  static UWORD rgb565(Color color) {
    const uint8_t red = (color.rgb >> 16) & 0xFF;
    const uint8_t green = (color.rgb >> 8) & 0xFF;
    const uint8_t blue = color.rgb & 0xFF;
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
  }

  UWORD* image_ = nullptr;
};

}  // namespace nina
