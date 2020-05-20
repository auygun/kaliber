#ifndef IMAGE_QUAD_H
#define IMAGE_QUAD_H

#include "../base/vecmath.h"
#include "animatable.h"
#include "renderer/texture.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace eng {

class Image;

class ImageQuad : public Animatable {
 public:
  ImageQuad() = default;
  ~ImageQuad() override = default;

  void Create(std::shared_ptr<const Image> image,
              std::array<int, 2> num_frames = {1, 1},
              int frame_width = 0,
              int frame_height = 0);

  void AutoScale();

  // Shape interface.
  void SetFrame(size_t frame) override;
  size_t GetFrame() override { return current_frame_; }
  size_t GetNumFrames() override;

  void Draw();
  void ContextLost();

  void SetVisible(bool visible) { visible_ = visible; }
  bool IsVisible() const { return visible_; }

  int frame_width() const { return frame_width_; }
  int frame_height() const { return frame_height_; }

  bool IsValid() const { return texture_.IsValid(); }

 private:
  Texture texture_;
  base::Vector2 tex_scale_ = {1, 1};

  size_t current_frame_ = 0;
  std::array<int, 2> num_frames_ = {1, 1};  // horizontal, vertical
  int frame_width_ = 0;
  int frame_height_ = 0;

  bool visible_ = false;

  base::Vector2 GetUVOffset(int frame);
};

}  // namespace eng

#endif  // IMAGE_QUAD_H
