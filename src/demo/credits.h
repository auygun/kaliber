#ifndef CREDITS_H
#define CREDITS_H

#include <memory>
#include <string>

#include "../engine/animator.h"
#include "../engine/image_quad.h"

namespace eng {
class Image;
class InputEvent;
class Texture;
}  // namespace eng

class Credits {
 public:
  static constexpr int kNumLines = 5;

  Credits();
  ~Credits();

  bool Initialize();

  void Update(float delta_time);

  void OnInputEvent(std::unique_ptr<eng::InputEvent> event);

  void Draw();

  void ContextLost();

  void Show();
  void Hide();

 private:
  std::shared_ptr<eng::Texture> tex_;

  eng::ImageQuad text_[kNumLines];
  eng::Animator text_animator_;

  int max_text_width_ = 0;

  std::unique_ptr<eng::Image> CreateImage();
};

#endif  // CREDITS_H
