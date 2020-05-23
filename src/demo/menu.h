#ifndef MENU_H
#define MENU_H

#include <memory>
#include <string>

#include "../engine/animator.h"
#include "../engine/image_quad.h"

namespace eng {
class Image;
class Font;
}  // namespace eng

class Menu {
 public:
  enum Options {
    kContinue,
    kNewGame,
    kCredits,
    kExit,
    kOptions_Max,
  };

  Menu() = default;
  ~Menu() = default;

  bool Initialize();

  void Update(float delta_time);

  void Draw();

  void ContextLost();

 private:
  struct Item {
    eng::ImageQuad text;
    eng::Animator text_animator[3];
    bool enabled = true;
  };

  Item items_[kOptions_Max];

  std::shared_ptr<const eng::Font> font_;
  int max_text_width_ = 0;

  std::shared_ptr<eng::Image> CreateImage();
};

#endif  // MENU_H
