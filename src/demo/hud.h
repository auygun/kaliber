#ifndef HUD_H
#define HUD_H

#include "../engine/image_quad.h"
#include "../engine/solid_quad.h"
#include "../engine/animator.h"
#include <string>
#include <memory>

namespace eng {
class Image;
class Font;
} // namespace eng

class Hud {
 public:
  Hud() = default;
  ~Hud() = default;

  bool Initialize();

  void Update(float delta_time);

  void Draw();

  void ContextLost();

  void PrintScore(int score, bool flash);
  void PrintWave(int wave);

  void SetProgress(float progress);

 private:
  eng::SolidQuad progress_bar_[2];
  eng::ImageQuad text_[2];

  eng::Animator hud_animator_;
  Callback hud_animator_cb_;

  std::shared_ptr<eng::Font> font_;
  int max_text_width_ = 0;

  int last_score_ = 0;
  int last_wave_ = 0;
  float last_progress_ = 0;

  void Print(int i, const std::string& text);

  std::shared_ptr<eng::Image> CreateImage();
};

#endif  // HUD_H
