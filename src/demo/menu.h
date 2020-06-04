#ifndef MENU_H
#define MENU_H

#include <memory>
#include <string>

#include "../base/closure.h"
#include "../base/vecmath.h"
#include "../engine/animator.h"
#include "../engine/image_quad.h"

namespace eng {
class Image;
class InputEvent;
class Font;
class Texture;
}  // namespace eng

class Menu {
 public:
  enum Option {
    kOption_Invalid = -1,
    kContinue,
    kNewGame,
    kCredits,
    kExit,
    kOption_Max,
  };

  Menu();
  ~Menu();

  bool Initialize();

  void Update(float delta_time);

  void OnInputEvent(std::unique_ptr<eng::InputEvent> event);

  void Draw();

  void ContextLost();

  void SetOptionEnabled(Option o, bool enable);

  void Show();
  void Hide();

  Option selected_option() const { return selected_option_; }

 private:
  struct Item {
    eng::ImageQuad text;
    eng::Animator text_animator;
    base::Closure select_item_cb_;
    bool hide = false;
  };

  std::shared_ptr<eng::Texture> tex_;

  Item items_[kOption_Max];

  std::shared_ptr<const eng::Font> font_;
  int max_text_width_ = 0;

  Option selected_option_ = kOption_Invalid;

  base::Vector2 tap_pos_[2] = {{0, 0}, {0, 0}};

  std::shared_ptr<eng::Image> CreateImage();

  bool IsAnimating();
};

#endif  // MENU_H
