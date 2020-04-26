#ifndef DEMO_H
#define DEMO_H

#include <vector>
#include "../engine/game.h"
#include "../engine/image_quad.h"

class Demo : public engine::Game {
 public:
  Demo() = default;
  ~Demo() override = default;

  bool Initialize() override;

  void Update(float delta_time) override;

  void Draw(float frame_frac) override;

 private:
  float seconds_accumulated_ = 0.0f;
  engine::ImageQuad bg_;
  engine::ImageQuad ship_;
  engine::ImageQuad enemy_;

  Vector2 ToScale(int width, int height);
};

#endif  // DEMO_H
