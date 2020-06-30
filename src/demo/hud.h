#ifndef HUD_H
#define HUD_H

#include <memory>
#include <string>

#include "../base/closure.h"
#include "../engine/animator.h"
#include "../engine/image_quad.h"
#include "../engine/solid_quad.h"

namespace eng {
class Image;
}  // namespace eng

class Hud {
 public:
  Hud();
  ~Hud();

  bool Initialize();

  void Update(float delta_time);

  void Show();

  void SetScore(int score, bool flash);
  void SetWave(int wave, bool flash);
  void SetProgress(float progress);

 private:
  eng::SolidQuad progress_bar_[2];
  eng::ImageQuad text_[2];

  eng::Animator progress_bar_animator_[2];
  eng::Animator text_animator_[2];
  base::Closure text_animator_cb_[2];

  int max_text_width_ = 0;

  int last_score_ = 0;
  int last_wave_ = 0;
  float last_progress_ = 0;

  std::unique_ptr<eng::Image> CreateScoreImage();
  std::unique_ptr<eng::Image> CreateWaveImage();

  std::unique_ptr<eng::Image> Print(int i, const std::string& text);

  std::unique_ptr<eng::Image> CreateImage();
};

#endif  // HUD_H
