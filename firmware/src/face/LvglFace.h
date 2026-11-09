#pragma once

// LVGL v8 face primitive for the Ninajirachi icon.  This is deliberately
// separate from the direct Waveshare surface used by the first bring-up.
#include <lvgl.h>

#include "ExpressionPreset.h"

namespace nina {

struct LvglFaceLayout {
  lv_coord_t width = 240;
  lv_coord_t height = 240;
  lv_coord_t bodySize = 170;
  lv_coord_t bodyRadius = 28;
  lv_coord_t bodyBorder = 4;
  lv_coord_t eyeWidth = 28;
  lv_coord_t eyeHeight = 86;
  lv_coord_t eyeRadius = 16;
  lv_coord_t eyeY = 43;
  lv_coord_t leftEyeX = 48;
  lv_coord_t rightEyeX = 152;
  lv_coord_t mouthSize = 64;
  lv_coord_t mouthStroke = 11;
  lv_coord_t leftMouthX = 62;
  lv_coord_t rightMouthX = 114;
  lv_coord_t mouthY = 104;
};

class LvglFace {
 public:
  explicit LvglFace(LvglFaceLayout layout = {}) : layout_(layout) {}

  void create(lv_obj_t* parent, const ExpressionPreset& preset) {
    root_ = lv_obj_create(parent);
    lv_obj_remove_style_all(root_);
    lv_obj_set_size(root_, layout_.width, layout_.height);
    lv_obj_center(root_);
    render(preset);
  }

  void render(const ExpressionPreset& preset) {
    if (root_ == nullptr) return;
    clear();
    lv_obj_set_style_bg_color(root_, color(preset.colors.background), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);

    body_ = lv_obj_create(root_);
    lv_obj_remove_style_all(body_);
    lv_obj_set_size(body_, layout_.bodySize, layout_.bodySize);
    lv_obj_center(body_);
    lv_obj_set_style_radius(body_, layout_.bodyRadius, 0);
    lv_obj_set_style_bg_opa(body_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(body_, layout_.bodyBorder, 0);
    lv_obj_set_style_border_color(body_, color(preset.colors.face), 0);
    lv_obj_set_style_border_opa(body_, LV_OPA_COVER, 0);
    lv_obj_set_style_transform_pivot_x(body_, layout_.bodySize / 2, 0);
    lv_obj_set_style_transform_pivot_y(body_, layout_.bodySize / 2, 0);
    lv_obj_set_style_transform_angle(body_, 450, 0);  // 45 degrees, 0.1 degree units.

    addEye(layout_.leftEyeX, layout_.eyeY, preset.colors.face);
    addEye(layout_.rightEyeX, layout_.eyeY, preset.colors.face);
    addMouth(layout_.leftMouthX, preset.colors.face);
    addMouth(layout_.rightMouthX, preset.colors.face);
  }

  lv_obj_t* object() const { return root_; }

 private:
  static lv_color_t color(Color value) { return lv_color_hex(value.rgb); }

  void addEye(lv_coord_t x, lv_coord_t y, Color value) {
    lv_obj_t* eye = lv_obj_create(root_);
    lv_obj_remove_style_all(eye);
    lv_obj_set_pos(eye, x, y);
    lv_obj_set_size(eye, layout_.eyeWidth, layout_.eyeHeight);
    lv_obj_set_style_radius(eye, layout_.eyeRadius, 0);
    lv_obj_set_style_bg_color(eye, color(value), 0);
    lv_obj_set_style_bg_opa(eye, LV_OPA_COVER, 0);
  }

  void addMouth(lv_coord_t x, Color value) {
    lv_obj_t* mouth = lv_arc_create(root_);
    lv_obj_remove_style_all(mouth);
    lv_obj_set_pos(mouth, x, layout_.mouthY);
    lv_obj_set_size(mouth, layout_.mouthSize, layout_.mouthSize);
    lv_arc_set_bg_angles(mouth, 0, 0);
    lv_arc_set_angles(mouth, 200, 340);
    lv_obj_set_style_arc_width(mouth, layout_.mouthStroke, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(mouth, color(value), LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(mouth, LV_OPA_COVER, LV_PART_INDICATOR);
  }

  void clear() {
    if (body_ != nullptr) {
      lv_obj_del(body_);
      body_ = nullptr;
    }
    // Eyes and mouth are children of root_; deleting the body does not clear them.
    while (lv_obj_get_child_cnt(root_) > 0)
      lv_obj_del(lv_obj_get_child(root_, 0));
  }

  LvglFaceLayout layout_;
  lv_obj_t* root_ = nullptr;
  lv_obj_t* body_ = nullptr;
};

}  // namespace nina
