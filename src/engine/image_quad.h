#ifndef IMAGE_QUAD_H
#define IMAGE_QUAD_H

#include "../base/vecmath.h"
#include "animatable.h"

#include <array>
#include <memory>

namespace eng {

class Texture;

class ImageQuad : public Animatable {
 public:
  ImageQuad() = default;
  ~ImageQuad() override = default;

  void Create(std::shared_ptr<Texture> texture,
              std::array<int, 2> num_frames = {1, 1},
              int frame_width = 0,
              int frame_height = 0);

  void Destory();

  void AutoScale();

  // Animatable interface.
  void SetFrame(size_t frame) override;
  size_t GetFrame() const override { return current_frame_; }
  size_t GetNumFrames() const override;
  void SetColor(const base::Vector4& color) override { color_ = color; }
  base::Vector4 GetColor() const override { return color_; }

  void Draw();

  std::shared_ptr<Texture> GetTexture() { return texture_; }

 private:
  std::shared_ptr<Texture> texture_;

  size_t current_frame_ = 0;
  std::array<int, 2> num_frames_ = {1, 1};  // horizontal, vertical
  int frame_width_ = 0;
  int frame_height_ = 0;

  base::Vector4 color_ = {1, 1, 1, 1};

  float GetFrameWidth() const;
  float GetFrameHeight() const;

  base::Vector2 GetUVOffset(int frame) const;
};

}  // namespace eng

#endif  // IMAGE_QUAD_H
