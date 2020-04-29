#ifndef BEAM_H
#define BEAM_H

#include "../base/vecmath.h"
#include "game_object.h"
#include "../engine/image_quad.h"

class Beam : public GameObject {
 public:
  Beam() = default;
  ~Beam() override = default;

  bool Initialize() override;

  void Update(float delta_time) override;

 private:
  engine::ImageQuad start_;
  engine::ImageQuad mid_;
  engine::ImageQuad end_;

  void Translate(const Vector2& offset);
  void Rotate(float angle);
};

#endif  // BEAM_H
