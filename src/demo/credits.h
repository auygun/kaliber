#ifndef CREDITS_H
#define CREDITS_H

#include <memory>
#include <string>

#include "../engine/animator.h"
#include "../engine/image_quad.h"

namespace eng {
class Image;
class InputEvent;
}  // namespace eng

class Credits {
 public:
  static constexpr int kNumLines = 10;

  Credits();
  ~Credits();

  bool Initialize();

  void OnInputEvent(std::unique_ptr<eng::InputEvent> event);

  void Show();
  void Hide();

 private:
  eng::ImageQuad text_[kNumLines];
  eng::Animator text_animator_;

  int max_text_width_ = 0;

  std::unique_ptr<eng::Image> CreateImage();
};

#endif  // CREDITS_H
