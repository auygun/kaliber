#ifndef IMAGE_QUAD_H
#define IMAGE_QUAD_H

#include "../base/vecmath.h"
#include "animatable.h"

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

namespace eng {

class Shader;
class Texture;

class ImageQuad : public Animatable {
 public:
  ImageQuad() = default;
  ~ImageQuad() override = default;

  void Create(const std::string& asset_name,
              std::array<int, 2> num_frames = {1, 1},
              int frame_width = 0,
              int frame_height = 0);

  void Destory();

  void AutoScale();

  void SetCustomShader(std::shared_ptr<Shader> shader);

  template <typename T>
  void SetCustomUniform(const std::string& name, T value) {
    custom_uniforms_[name] = UniformValue(value);
  }

  // Animatable interface.
  void SetFrame(size_t frame) override;
  size_t GetFrame() const override { return current_frame_; }
  size_t GetNumFrames() const override;
  void SetColor(const base::Vector4& color) override { color_ = color; }
  base::Vector4 GetColor() const override { return color_; }

  // Drawable interface.
  void Draw(float frame_frac) override;

 private:
  using UniformValue = std::variant<base::Vector2,
                                    base::Vector3,
                                    base::Vector4,
                                    base::Matrix4x4,
                                    float,
                                    int>;

  std::shared_ptr<Texture> texture_;

  std::shared_ptr<Shader> custom_shader_;
  std::unordered_map<std::string, UniformValue> custom_uniforms_;

  size_t current_frame_ = 0;
  std::array<int, 2> num_frames_ = {1, 1};  // horizontal, vertical
  int frame_width_ = 0;
  int frame_height_ = 0;

  base::Vector4 color_ = {1, 1, 1, 1};

  std::string asset_name_;

  float GetFrameWidth() const;
  float GetFrameHeight() const;

  base::Vector2 GetUVOffset(int frame) const;
};

}  // namespace eng

#endif  // IMAGE_QUAD_H
