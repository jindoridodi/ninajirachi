#pragma once

#include <lvgl.h>
#include <algorithm>
#include "ExpressionPreset.h"
#include "FaceGeometry.generated.h"

namespace nina {

class LvglFace {
 public:
  void create(lv_obj_t* parent, const ExpressionPreset& preset) {
    root_ = lv_obj_create(parent);
    lv_obj_remove_style_all(root_);
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(root_, 240, 240);
    lv_obj_center(root_);
    for (unsigned i = 0; i < sizeof(outlinePoints_) / sizeof(outlinePoints_[0]); ++i)
      outlinePoints_[i] = {kFaceOutline[i].x, kFaceOutline[i].y};
    outline_ = line(outlinePoints_, sizeof(outlinePoints_) / sizeof(outlinePoints_[0]),
                    kFaceOutlineWidth);
    for (int i = 0; i < 2; ++i) {
      eyes_[i] = lv_obj_create(root_);
      lv_obj_remove_style_all(eyes_[i]);
      lv_obj_clear_flag(eyes_[i], LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_set_style_bg_opa(eyes_[i], LV_OPA_COVER, 0);
    }
    mouth_ = line(mouthPoints_, sizeof(mouthPoints_) / sizeof(mouthPoints_[0]),
                  kFaceMouthWidth);
    effect_ = lv_obj_create(root_);
    lv_obj_remove_style_all(effect_);
    lv_obj_set_style_radius(effect_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(effect_, LV_OPA_COVER, 0);
    render(preset);
  }

  void render(const ExpressionPreset& preset) {
    if (!root_) return;
    lv_obj_set_style_bg_color(root_, lv_color_hex(preset.colors.background.rgb), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
    lv_obj_set_style_line_color(outline_, lv_color_hex(preset.colors.face.rgb), 0);
    const auto& m = preset.metrics;
    for (int i = 0; i < 2; ++i) {
      const auto& eye = kFaceEyes[i];
      const int width = std::max(1, eye[2] * m.eyeWidth / 36);
      const int height = std::max(1, eye[3] * m.eyeOpen / 42);
      lv_obj_set_pos(eyes_[i], eye[0] + (eye[2]-width)/2 + m.pupilOffsetX,
                     eye[1] + (eye[3]-height)/2 + m.pupilOffsetY);
      lv_obj_set_size(eyes_[i], width, height);
      lv_obj_set_style_radius(eyes_[i], kFaceEyeRadius, 0);
      lv_obj_set_style_bg_color(eyes_[i], lv_color_hex(preset.colors.eye.rgb), 0);
    }
    for (unsigned i = 0; i < sizeof(mouthPoints_) / sizeof(mouthPoints_[0]); ++i) {
      mouthPoints_[i].x = 120 + (kFaceMouth[i].x - 120) * m.mouthWidth / 42;
      mouthPoints_[i].y = kFaceMouth[0].y +
          (kFaceMouth[i].y - kFaceMouth[0].y) * m.mouthHeight / 14;
    }
    lv_line_set_points(mouth_, mouthPoints_, sizeof(mouthPoints_) / sizeof(mouthPoints_[0]));
    lv_obj_set_style_line_color(mouth_, lv_color_hex(preset.colors.mouth.rgb), 0);
    lv_obj_invalidate(mouth_);
    if (m.effectRadius > 0) {
      lv_obj_clear_flag(effect_, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_pos(effect_, 120-m.effectRadius, 32-m.effectRadius);
      lv_obj_set_size(effect_, m.effectRadius*2, m.effectRadius*2);
      lv_obj_set_style_bg_color(effect_, lv_color_hex(preset.colors.effect.rgb), 0);
    } else {
      lv_obj_add_flag(effect_, LV_OBJ_FLAG_HIDDEN);
    }
  }

  lv_obj_t* object() const { return root_; }

 private:
  lv_obj_t* line(const lv_point_t* points, uint16_t count, int width) {
    lv_obj_t* obj = lv_line_create(root_);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, 0, 0);
    lv_line_set_points(obj, points, count);
    lv_obj_set_style_line_width(obj, width, 0);
    lv_obj_set_style_line_rounded(obj, true, 0);
    lv_obj_set_style_line_opa(obj, LV_OPA_COVER, 0);
    return obj;
  }

  lv_obj_t* root_ = nullptr;
  lv_obj_t* outline_ = nullptr;
  lv_obj_t* eyes_[2] = {};
  lv_obj_t* mouth_ = nullptr;
  lv_obj_t* effect_ = nullptr;
  lv_point_t outlinePoints_[sizeof(kFaceOutline) / sizeof(kFaceOutline[0])] = {};
  lv_point_t mouthPoints_[sizeof(kFaceMouth) / sizeof(kFaceMouth[0])] = {};
};

}  // namespace nina
