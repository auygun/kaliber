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

 protected:
  std::shared_ptr<Shader> GetCustomShader() { return custom_shader_; }

 private:
  bool visible_ = false;
  int z_order_ = 0;

  std::shared_ptr<Shader> custom_shader_;
};

}  // namespace eng

#endif  // ENGINE_DRAWABLE_H
