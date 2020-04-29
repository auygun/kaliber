#ifndef BEAM_H
#define BEAM_H

#include "../base/vecmath.h"
#include "game_object.h"
#include "../engine/image_quad.h"

namespace engine {
class InputEvent;
} //  namespace engine

class Beam : public GameObject {
 public:
  Beam() = default;
  ~Beam() override = default;

  bool Initialize() override;

  void Update(float delta_time) override;

  void OnInputEvent(std::unique_ptr<engine::InputEvent> event);

 private:
  engine::ImageQuad start_;
  engine::ImageQuad mid_;
  engine::ImageQuad end_;

  Vector2 start_pos_ = {0, 0};
  Vector2 end_pos_ = {0, 0};

  void Translate(const Vector2& offset);
  void Rotate(float angle);

  void SetVisible(bool visible);
};

#endif  // BEAM_H
