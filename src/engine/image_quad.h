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

  void ResetScale();

  // FrameController interface.
  size_t GetNumFrames() override;
  size_t GetCurrentFrame() override;
  void SetCurrentFrame(size_t frame) override;

  // Drawable interface.
  void Draw() override;

  void PlaceToLeftOf(const ImageQuad& d) {
    Translate({d.scale().x / -2.0f + scale().x / -2.0f, 0});
  }

  void PlaceToRightOf(const ImageQuad& d) {
    Translate({d.scale().x / 2.0f + scale().x / 2.0f, 0});
  }

 private:
  Texture texture_;
  Vector2 tex_scale_ = {1, 1};
  size_t current_frame_ = 0;
  std::array<int, 2> num_frames_ = {1, 1}; // horizontal, vertical
  int frame_width_ = 0;
  int frame_height_ = 0;

  Vector2 GetUVOffset(int frame);
};

}  // namespace engine

#endif  // IMAGE_QUAD_H
