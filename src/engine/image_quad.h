#ifndef ENGINE_IMAGE_QUAD_H
#define ENGINE_IMAGE_QUAD_H

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

#include "base/vecmath.h"
#include "engine/animatable.h"

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
  void SetColor(const base::Vector4f& color) override { color_ = color; }
  base::Vector4f GetColor() const override { return color_; }

  // Drawable interface.
  void Draw(float frame_frac) override;

 private:
  using UniformValue = std::variant<base::Vector2f,
                                    base::Vector3f,
                                    base::Vector4f,
                                    base::Matrix4f,
                                    float,
                                    int>;

  std::shared_ptr<Texture> texture_;

  std::shared_ptr<Shader> custom_shader_;
  std::unordered_map<std::string, UniformValue> custom_uniforms_;

  size_t current_frame_ = 0;
  std::array<int, 2> num_frames_ = {1, 1};  // horizontal, vertical
  int frame_width_ = 0;
  int frame_height_ = 0;

  base::Vector4f color_ = {1, 1, 1, 1};

  std::string asset_name_;

  float GetFrameWidth() const;
  float GetFrameHeight() const;

  base::Vector2f GetUVOffset(int frame) const;
};

}  // namespace eng

#endif  // ENGINE_IMAGE_QUAD_H
