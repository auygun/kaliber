#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <vector>
#include <memory>
#include "../engine/image_quad.h"
#include "../engine/draw_animator.h"

class Background {
 public:
  Background() = default;
  ~Background() = default;

  bool Initialize();

  void Update(float delta_time);

 private:
  std::vector<std::unique_ptr<engine::ImageQuad>> bg_tiles_;
  engine::DrawAnimator draw_animator_;
};

#endif  // BACKGROUND_H
