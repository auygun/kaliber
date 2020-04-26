#ifndef TEXT_BO_H
#define TEXT_BO_H

#include "../base/vecmath.h"
#include "renderer/texture.h"
#include "drawable.h"

#include <string>
#include <vector>

class Fontx;

namespace engine {

class TextBox : public Drawable {
 public:
  TextBox() = default;
  ~TextBox() override = default;

  bool Print(Fontx& font, const std::string& text);
  bool Print(Fontx& font, const std::vector<std::string> lines, int width);

  void Draw() override;

  void Translate(const Vector2& offset) { offset_ = offset; }
  void Scale(const Vector2& scale) { scale_ = scale; }

  Vector2 offset() { return offset_; }
  Vector2 scale() { return scale_; }

 private:
  Texture texture_;
  Vector2 offset_ = {0, 0};
  Vector2 scale_ = {1, 1};
};

}  // namespace engine

#endif  // TEXT_BO_H
