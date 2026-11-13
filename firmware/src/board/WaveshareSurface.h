#pragma once

#include "../face/FaceRenderer.h"
#include "../waveshare/DEV_Config.h"
#include "../waveshare/GUI_Paint.h"
#include "../waveshare/LCD_1in28.h"

namespace nina {

class WaveshareSurface final : public DrawSurface {
 public:
  bool begin() {
    if (DEV_Module_Init() != 0) return fail();

    if (!psramInit()) return fail();

    image_ = static_cast<UWORD*>(ps_malloc(LCD_1IN28_HEIGHT * LCD_1IN28_WIDTH * sizeof(UWORD)));
    if (image_ == nullptr) return fail();

    LCD_1IN28_Init(HORIZONTAL);
    Paint_NewImage(reinterpret_cast<UBYTE*>(image_), LCD_1IN28.WIDTH, LCD_1IN28.HEIGHT, 0, 0);
    Paint_SetScale(65);
    Paint_SetRotate(ROTATE_0);
    return true;
  }

  void present() { LCD_1IN28_Display(image_); }

  Color pixel(int x, int y) const override {
    const int offset = (y * 240 + x) * 2;
    const uint16_t value = uint16_t(Paint.Image[offset]) << 8 | Paint.Image[offset + 1];
    const uint8_t r = (value >> 11) & 31, g = (value >> 5) & 63, b = value & 31;
    return {uint32_t((r << 3) | (r >> 2)) << 16 | uint32_t((g << 2) | (g >> 4)) << 8 |
            uint32_t((b << 3) | (b >> 2))};
  }

  void fillRect(int x, int y, int width, int height, Color color) override {
    if (width <= 0 || height <= 0 || x >= 240 || y >= 240) return;
    const int endX = min(239, x + width - 1);
    const int endY = min(239, y + height - 1);
    x = max(0, x);
    y = max(0, y);
    if (endX < x || endY < y) return;
    // ClearWindows uses exclusive endpoints and avoids DrawPoint's one-pixel offset.
    Paint_ClearWindows(x, y, endX + 1, endY + 1, rgb565(color));
  }

 private:
  static bool fail() {
    digitalWrite(2, LOW);
    pinMode(2, OUTPUT);
    return false;
  }

  static UWORD rgb565(Color color) {
    const uint8_t red = (color.rgb >> 16) & 0xFF;
    const uint8_t green = (color.rgb >> 8) & 0xFF;
    const uint8_t blue = color.rgb & 0xFF;
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
  }

  UWORD* image_ = nullptr;
};

}  // namespace nina
