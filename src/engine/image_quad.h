#ifndef IMAGE_QUAD_H
#define IMAGE_QUAD_H

#include "../base/vecmath.h"
#include "renderer/texture.h"
#include "shape.h"

#include <string>
#include <vector>
#include <memory>
#include <array>

namespace eng {

class Image;

class ImageQuad : public Shape {
 public:
  ImageQuad() = default;
  ~ImageQuad() override = default;

  ImageQuad(const ImageQuad&) = delete;
  ImageQuad& operator=(const ImageQuad&) = delete;

  void Create(std::shared_ptr<const Image> image,
              std::array<int, 2> num_frames = {1, 1},
              int frame_width = 0,
              int frame_height = 0);

  void AutoScale();

  // Shape interface.
  void SetFrame(size_t frame) override;
  size_t GetFrame() override { return current_frame_; }
  size_t GetNumFrames() override;

  // Drawable interface.
  void Draw() override;
  void ContextLost() override;

 private:
  Texture texture_;
  Vector2 tex_scale_ = {1, 1};

  size_t current_frame_ = 0;
  std::array<int, 2> num_frames_ = {1, 1}; // horizontal, vertical
  int frame_width_ = 0;
  int frame_height_ = 0;

  Vector2 GetUVOffset(int frame);
};

}  // namespace eng

#endif  // IMAGE_QUAD_H
