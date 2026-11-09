#pragma once

#include <lvgl.h>
#include <algorithm>
#include "ExpressionPreset.h"
#include "FaceGeometry.generated.h"
#include "FaceRenderer.h"

namespace nina {

class LvglFace {
 public:
  void create(lv_obj_t* parent, const ExpressionPreset& preset) {
    root_ = lv_obj_create(parent);
    lv_obj_remove_style_all(root_);
    lv_obj_clear_flag(root_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(root_, 240, 240);
    lv_obj_center(root_);
    lv_obj_add_event_cb(root_, drawFill, LV_EVENT_DRAW_MAIN_END, this);
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
    render(preset);
  }

  void render(const ExpressionPreset& preset) {
    if (!root_) return;
    faceColor_ = preset.colors.face;
    lv_obj_invalidate(root_);
    lv_obj_set_style_bg_color(root_, lv_color_hex(preset.colors.background.rgb), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
    lv_obj_set_style_line_color(outline_, lv_color_hex(preset.colors.face.rgb), 0);
    const auto& m = preset.metrics;
    for (int i = 0; i < 2; ++i) {
      const auto eye = faceEye(kFaceEyes[i], m);
      lv_obj_set_pos(eyes_[i], eye.x, eye.y);
      lv_obj_set_size(eyes_[i], eye.width, eye.height);
      lv_obj_set_style_radius(eyes_[i], kFaceEyeRadius, 0);
      lv_obj_set_style_bg_color(eyes_[i], lv_color_hex(preset.colors.eye.rgb), 0);
    }
    for (unsigned i = 0; i < sizeof(mouthPoints_) / sizeof(mouthPoints_[0]); ++i) {
      const auto point = faceMouthPoint(kFaceMouth[i], m);
      mouthPoints_[i].x = point.x;
      mouthPoints_[i].y = point.y;
    }
    lv_line_set_points(mouth_, mouthPoints_, sizeof(mouthPoints_) / sizeof(mouthPoints_[0]));
    lv_obj_set_style_line_color(mouth_, lv_color_hex(preset.colors.mouth.rgb), 0);
    lv_obj_invalidate(mouth_);
  }

  lv_obj_t* object() const { return root_; }

 private:
  class FillSurface : public DrawSurface {
   public:
    explicit FillSurface(lv_draw_ctx_t* context) : context_(context) {}
    void beginLayer(FaceLayer) override {}
    void fillRect(int x, int y, int width, int height, Color color) override {
      lv_draw_rect_dsc_t descriptor;
      lv_draw_rect_dsc_init(&descriptor);
      descriptor.bg_color = lv_color_hex(color.rgb);
      descriptor.bg_opa = LV_OPA_COVER;
      const lv_area_t area = {static_cast<lv_coord_t>(x), static_cast<lv_coord_t>(y),
                              static_cast<lv_coord_t>(x + width - 1), static_cast<lv_coord_t>(y + height - 1)};
      lv_draw_rect(context_, &descriptor, &area);
    }
   private:
    lv_draw_ctx_t* context_;
  };

  static void drawFill(lv_event_t* event) {
    auto* face = static_cast<LvglFace*>(lv_event_get_user_data(event));
    lv_area_t coords;
    lv_obj_get_coords(face->root_, &coords);
    FillSurface surface(lv_event_get_draw_ctx(event));
    FaceRenderer::fillFace(surface, coords.x1, coords.y1, face->faceColor_);
  }

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
  Color faceColor_ = {0};
  lv_obj_t* outline_ = nullptr;
  lv_obj_t* eyes_[2] = {};
  lv_obj_t* mouth_ = nullptr;
  lv_point_t outlinePoints_[sizeof(kFaceOutline) / sizeof(kFaceOutline[0])] = {};
  lv_point_t mouthPoints_[sizeof(kFaceMouth) / sizeof(kFaceMouth[0])] = {};
};

}  // namespace nina
