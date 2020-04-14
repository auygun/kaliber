#ifndef DEMO_H
#define DEMO_H

#include "../engine/game.h"
#include "../engine/quad.h"
#include "../engine/renderer/texture.h"

class Demo : public engine::Game {
 public:
  Demo() = default;
  ~Demo() override  = default;

  bool Initialize() override;

  void Shutdown() override;

  void Update(float delta_time) override;

  void Draw(float frame_frac) override;

 private:
  engine::Quad quad_;
  engine::Texture texture_;
  Vector2 scale_ = {1 ,1};
};

#endif // DEMO_H
