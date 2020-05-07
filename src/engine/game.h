#ifndef GAME_H
#define GAME_H

namespace eng {

class Game {
 public:
  virtual ~Game() = default;

  virtual bool Initialize() = 0;

  virtual void Update(float delta_time) = 0;
};

}  // namespace eng

#endif  // GAME_H
