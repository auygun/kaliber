#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <vector>
#include <memory>
#include "../engine/image_quad.h"

class Background {
 public:
  Background() = default;
  ~Background() = default;

  bool Initialize();

  void Update(float delta_time);

 private:
  float seconds_accumulated_ = 0.0f;
  std::vector<std::unique_ptr<engine::ImageQuad>> bg_tiles_;
};

#endif  // BACKGROUND_H
