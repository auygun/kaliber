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

 private:
  Texture texture_;
};

}  // namespace engine

#endif  // TEXT_BO_H
