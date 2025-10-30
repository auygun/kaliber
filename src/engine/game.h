#ifndef ENGINE_GAME_H
#define ENGINE_GAME_H

namespace eng {

class Game {
 public:
  Game() = default;
  virtual ~Game() = default;

  virtual bool Initialize() { return true; }

  virtual void Update(float delta_time) {}

  virtual void Render(float frame_frac) {}

  virtual void ContextLost() {}

  virtual void OnWindowResized(int width, int height) {}

  virtual void LostFocus() {}

  virtual void GainedFocus(bool from_interstitial_ad) {}

 private:
  Game(const Game&) = delete;
  Game& operator=(const Game&) = delete;
};

}  // namespace eng

#endif  // ENGINE_GAME_H
