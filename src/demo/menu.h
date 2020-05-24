#ifndef MENU_H
#define MENU_H

#include <memory>
#include <string>

#include "../engine/animator.h"
#include "../engine/image_quad.h"

namespace eng {
class Image;
class InputEvent;
class Font;
}  // namespace eng

class Menu {
 public:
  enum Option {
    kContinue,
    kNewGame,
    kCredits,
    kExit,
    kOption_Max,
  };

  using Callback = std::function<void(Option)>;

  Menu() = default;
  ~Menu() = default;

  bool Initialize(Callback item_selected_cb);

  void Update(float delta_time);

  void OnInputEvent(std::unique_ptr<eng::InputEvent> event);

  void Draw();

  void ContextLost();

 private:
  struct Item {
    eng::ImageQuad text;
    eng::Animator text_animator;
    base::Closure text_animator_cb_;
    bool enabled = true;
  };

  Item items_[kOption_Max];

  std::shared_ptr<const eng::Font> font_;
  int max_text_width_ = 0;

  Callback item_selected_cb_;

  std::shared_ptr<eng::Image> CreateImage();
};

#endif  // MENU_H
