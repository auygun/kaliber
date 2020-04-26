#ifndef DEMO_H
#define DEMO_H

#include <vector>
#include <memory>
#include "background.h"
#include "../engine/game.h"
#include "../engine/image_quad.h"

class Demo : public engine::Game {
 public:
  Demo() = default;
  ~Demo() override = default;

  bool Initialize() override;

  void Update(float delta_time) override;

  // void Draw(float frame_frac) override;

 private:
  float seconds_accumulated_ = 0.0f;
  Background bg_;
  engine::ImageQuad ship_;
  engine::ImageQuad enemy_;
};

#endif  // DEMO_H
