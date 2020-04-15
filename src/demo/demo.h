#ifndef DEMO_H
#define DEMO_H

#include "../engine/game.h"
#include "../engine/sprite.h"

class Demo : public engine::Game {
 public:
  Demo() = default;
  ~Demo() override  = default;

  bool Initialize() override;

  void Shutdown() override;

  void Update(float delta_time) override;

  void Draw(float frame_frac) override;

 private:
  engine::Sprite sprite_;
};

#endif // DEMO_H
