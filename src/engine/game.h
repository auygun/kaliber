#ifndef GAME_H
#define GAME_H

namespace eng {

class Game {
 public:
  Game() = default;
  virtual ~Game() = default;

  virtual bool Initialize() = 0;

  virtual void Update(float delta_time) = 0;

  virtual void ContextLost() = 0;

  virtual void LostFocus() = 0;

  virtual void GainedFocus() = 0;

 private:
  Game(const Game&) = delete;
  Game& operator=(const Game&) = delete;
};

}  // namespace eng

#endif  // GAME_H
