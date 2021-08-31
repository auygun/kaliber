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

  void Pause(bool pause);

  void Show();
  void Hide();
  void HideProgress();

  void SetScore(size_t score, bool flash);
  void SetWave(int wave, bool flash);
  void SetProgress(float progress);

  void ShowMessage(const std::string& text, float duration);

  void ShowBonus(size_t bonus);

 private:
  eng::SolidQuad progress_bar_[2];
  eng::ImageQuad text_[2];
  eng::ImageQuad message_;
  eng::ImageQuad bonus_;

  eng::Animator progress_bar_animator_[2];
  eng::Animator text_animator_[2];
  eng::Animator message_animator_;
  base::Closure text_animator_cb_[2];
  eng::Animator bonus_animator_;

  int max_text_width_ = 0;

  size_t last_score_ = 0;
  int last_wave_ = 0;
  float last_progress_ = 0;

  std::string message_text_;

  size_t bonus_score_ = 0;

  std::unique_ptr<eng::Image> CreateScoreImage();
  std::unique_ptr<eng::Image> CreateWaveImage();
  std::unique_ptr<eng::Image> CreateMessageImage();
  std::unique_ptr<eng::Image> CreateBonusImage();

  std::unique_ptr<eng::Image> Print(int i, const std::string& text);

  std::unique_ptr<eng::Image> CreateImage();
};

#endif  // HUD_H
