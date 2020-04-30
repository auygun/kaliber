#ifndef IMAGE_QUAD_H
#define IMAGE_QUAD_H

#include "../base/vecmath.h"
#include "renderer/texture.h"
#include "drawable.h"
#include "frame_controller.h"

#include <string>
#include <vector>
#include <memory>
#include <array>

namespace engine {

class Image;

class ImageQuad : public Drawable, public FrameController {
 public:
  ImageQuad() = default;
  ~ImageQuad() override = default;

  bool Create(std::shared_ptr<const Image> image,
              std::array<int, 2> num_frames = {1, 1});

  size_t GetNumFrames() override;
  size_t GetCurrentFrame() override;
  void SetCurrentFrame(size_t frame) override;

  void Draw() override;

  size_t current_frame() { return current_frame_; }

 private:
  Texture texture_;
  Vector2 tex_scale_ = {1, 1};
  size_t current_frame_ = 0;
  std::array<int, 2> num_frames_ = {1, 1}; // horizontal, vertical

  Vector2 GetUVOffset(int frame);
};

}  // namespace engine

#endif  // IMAGE_QUAD_H
