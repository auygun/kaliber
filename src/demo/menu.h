#ifndef DEMO_MENU_H
#define DEMO_MENU_H

#include <memory>
#include <string>

#include "base/closure.h"
#include "base/vecmath.h"
#include "engine/animator.h"
#include "engine/image_quad.h"
#include "engine/sound_player.h"

namespace eng {
class Image;
class InputEvent;
class Sound;
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

  void OnInputEvent(std::unique_ptr<eng::InputEvent> event);

  void SetOptionEnabled(Option o, bool enable);

  void Show();
  void Hide(base::Closure cb = nullptr);

  Option selected_option() const { return selected_option_; }

  int start_from_wave() { return start_from_wave_; }

 private:
  class Button {
   public:
    Button() = default;
    ~Button() = default;

    void Create(const std::string& asset_name,
                std::array<int, 2> num_frames,
                int frame1,
                int frame2,
                base::Closure pressed_cb,
                bool switch_control,
                bool enabled);

    bool OnInputEvent(eng::InputEvent* event);

    void Show();
    void Hide();

    eng::ImageQuad& image() { return image_; };

    bool enabled() const { return enabled_; }

   private:
    eng::ImageQuad image_;
    eng::Animator animator_;
    int frame1_ = 0;
    int frame2_ = 0;
    base::Closure pressed_cb_;

    bool switch_control_ = false;
    bool enabled_ = false;
    base::Vector2f tap_pos_[2] = {{0, 0}, {0, 0}};

    void SetEnabled(bool enable);
  };

  class Radio {
   public:
    Radio() = default;
    ~Radio() = default;

    void Create(const std::string& asset_name);

    bool OnInputEvent(eng::InputEvent* event);

    void Show();
    void Hide();

    eng::ImageQuad& image() { return options_; };

   private:
    eng::ImageQuad options_;
    eng::Animator animator_;

    std::unique_ptr<eng::Image> CreateImage();
  };

  struct Item {
    eng::ImageQuad text;
    eng::Animator text_animator;
    base::Closure select_item_cb_;
    bool hide = false;
  };

  eng::ImageQuad logo_[2];
  eng::Animator logo_animator_[2];

  std::shared_ptr<eng::Sound> click_sound_;

  eng::SoundPlayer click_;

  Item items_[kOption_Max];

  int max_text_width_ = 0;

  Option selected_option_ = kOption_Invalid;

  base::Vector2f tap_pos_[2] = {{0, 0}, {0, 0}};

  Button toggle_audio_;
  Button toggle_music_;
  Button toggle_vibration_;

  size_t high_score_value_ = 0;

  eng::ImageQuad high_score_;
  eng::Animator high_score_animator_;

  eng::ImageQuad version_;
  eng::Animator version_animator_;

  int start_from_wave_ = 1;

  Radio starting_wave_;
  Button wave_up_;
  Button wave_down_;

  bool CreateRenderResources();

  std::unique_ptr<eng::Image> CreateMenuImage();
  std::unique_ptr<eng::Image> CreateHighScoreImage();

  bool IsAnimating();
};

#endif  // DEMO_MENU_H
