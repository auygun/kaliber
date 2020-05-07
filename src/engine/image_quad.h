#ifndef IMAGE_QUAD_H
#define IMAGE_QUAD_H

#include "../base/vecmath.h"
#include "renderer/texture.h"
#include "drawable.h"

#include <string>
#include <vector>
#include <memory>
#include <array>

namespace eng {

class Image;

class ImageQuad : public Drawable {
 public:
  ImageQuad() = default;
  ~ImageQuad() override = default;

  ImageQuad(const ImageQuad&) = delete;
  ImageQuad& operator=(const ImageQuad&) = delete;

  void Create(std::shared_ptr<const Image> image,
              std::array<int, 2> num_frames = {1, 1});

  void AutoScale();

  void Translate(const Vector2& offset);
  void Scale(const Vector2& scale);
  void Scale(float scale);
  void Rotate(float angle);

  void SetOffset(const Vector2& offset) { offset_ = offset; }
  void SetScale(const Vector2& scale) { scale_ = scale; }
  void SetPivot(const Vector2& pivot) { pivot_ = pivot; }
  void SetRotation(const Vector2& rotation) { rotation_ = rotation; }
  void SetColor(const Vector4& color) { color_ = color; }
  void SetFrame(size_t frame);

  Vector2 GetOffset() const { return offset_; }
  Vector2 GetScale() const { return scale_; }
  Vector2 GetPivot() const { return pivot_; }
  Vector2 GetRotation() const { return rotation_; }
  Vector4 GetColor() const { return color_; }
  size_t GetFrame() { return current_frame_; }
  size_t GetNumFrames();

  // Drawable interface.
  void Draw() override;

  void PlaceToLeftOf(const ImageQuad& d) {
    Translate({d.GetScale().x / -2.0f + GetScale().x / -2.0f, 0});
  }

  void PlaceToRightOf(const ImageQuad& d) {
    Translate({d.GetScale().x / 2.0f + GetScale().x / 2.0f, 0});
  }

 private:
  Vector2 offset_ = {0, 0};
  Vector2 scale_ = {1, 1};
  Vector2 pivot_ = {0, 0};
  Vector2 rotation_ = {0, 1};

  Texture texture_;
  Vector4 color_ = {1, 1, 1, 1};
  Vector2 tex_scale_ = {1, 1};

  size_t current_frame_ = 0;
  std::array<int, 2> num_frames_ = {1, 1}; // horizontal, vertical
  int frame_width_ = 0;
  int frame_height_ = 0;

  Vector2 GetUVOffset(int frame);
};

}  // namespace eng

#endif  // IMAGE_QUAD_H
