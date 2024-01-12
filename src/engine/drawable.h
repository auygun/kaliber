#ifndef ENGINE_DRAWABLE_H
#define ENGINE_DRAWABLE_H

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

#include "base/vecmath.h"

namespace eng {

class Shader;

class Drawable {
 public:
  Drawable();
  virtual ~Drawable();

  Drawable(const Drawable&) = delete;
  Drawable& operator=(const Drawable&) = delete;

  virtual void Draw(float frame_frac) = 0;

  void SetZOrder(int z) { z_order_ = z; }
  void SetVisible(bool visible) { visible_ = visible; }

  int GetZOrder() const { return z_order_; }
  bool IsVisible() const { return visible_; }

  void SetCustomShader(const std::string& asset_name);

  template <typename T>
  void SetCustomUniform(const std::string& name, T value) {
    custom_uniforms_[name] = UniformValue(value);
  }

 protected:
  std::shared_ptr<Shader> GetCustomShader() { return custom_shader_; }
  void DoSetCustomUniforms();

 private:
  using UniformValue = std::variant<base::Vector2f,
                                    base::Vector3f,
                                    base::Vector4f,
                                    base::Matrix4f,
                                    float,
                                    int>;

  bool visible_ = false;
  int z_order_ = 0;

  std::shared_ptr<Shader> custom_shader_;
  std::unordered_map<std::string, UniformValue> custom_uniforms_;
};

}  // namespace eng

#endif  // ENGINE_DRAWABLE_H
