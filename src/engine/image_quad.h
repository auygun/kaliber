#ifndef ENGINE_IMAGE_QUAD_H
#define ENGINE_IMAGE_QUAD_H

#include <array>
#include <memory>
#include <string>

#include "base/vecmath.h"
#include "engine/animatable.h"

namespace eng {

class Texture;

class ImageQuad final : public Animatable {
 public:
  ImageQuad();
  ~ImageQuad() final;

  ImageQuad& Create(const std::string& asset_name,
                    std::array<int, 2> num_frames = {1, 1},
                    int frame_width = 0,
                    int frame_height = 0);

  void Destroy();

  // Animatable interface.
  void SetFrame(size_t frame) final;
  size_t GetFrame() const final { return current_frame_; }
  size_t GetNumFrames() const final;
  void SetColor(const base::Vector4f& color) final { color_ = color; }
  base::Vector4f GetColor() const final { return color_; }

  // Drawable interface.
  void Draw(float frame_frac) final;

 private:
  std::shared_ptr<Texture> texture_;

  size_t current_frame_ = 0;
  std::array<int, 2> num_frames_ = {1, 1};  // horizontal, vertical
  int frame_width_ = 0;
  int frame_height_ = 0;

  base::Vector4f color_ = {1, 1, 1, 1};

  std::string asset_name_;

  float GetFrameWidth() const;
  float GetFrameHeight() const;

  base::Vector2f GetUVOffset(size_t frame) const;
};

}  // namespace eng

#endif  // ENGINE_IMAGE_QUAD_H
