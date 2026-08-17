#ifndef ENGINE_GAME_H
#define ENGINE_GAME_H

namespace eng {

class Game {
 public:
  Game() = default;
  virtual ~Game() = default;

  virtual bool Initialize(World& world) { return true; }

  virtual void FixedUpdate(World& world) {}

  virtual void Update(World& world, float delta_time) {}

  virtual void ContextLost() {}

  virtual void OnFramebufferResized(int width, int height) {}

  virtual void LostFocus() {}

  virtual void GainedFocus(bool from_interstitial_ad) {}

 private:
  Game(const Game&) = delete;
  Game& operator=(const Game&) = delete;
};

}  // namespace eng

#endif  // ENGINE_GAME_H
