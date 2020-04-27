#ifndef DEMO_H
#define DEMO_H

#include <vector>
#include <memory>
#include "background.h"
#include "enemy.h"
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
  Background bg_;
  Enemy enemy_;
  engine::ImageQuad ship_;
};

#endif  // DEMO_H
