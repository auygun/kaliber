#ifndef GAME_H
#define GAME_H

namespace engine {

class Game {
 public:
  virtual ~Game() = default;

  virtual bool Initialize() = 0;

  virtual void Shutdown() = 0;

  virtual void Update(float delta_time) = 0;

  virtual void Draw(float frame_frac) = 0;
};

} // namespace engine

#endif // GAME_H
