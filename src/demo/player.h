#ifndef PLAYER_H
#define PLAYER_H

#include "../base/vecmath.h"
#include "game_object.h"
#include "../engine/image_quad.h"

namespace engine {
class InputEvent;
} //  namespace engine

class Player : public GameObject {
 public:
  Player() = default;
  ~Player() override = default;

  bool Initialize() override;

  void Update(float delta_time) override;

  void OnInputEvent(std::unique_ptr<engine::InputEvent> event);

 private:
  engine::ImageQuad beam_start_;
  engine::ImageQuad beam_mid_;
  engine::ImageQuad beam_end_;

  Vector2 start_pos_ = {0, 0};
  Vector2 end_pos_ = {0, 0};

  bool CreateBeam();
  void TranslateBeam(const Vector2& offset);
  void RotateBeam(float angle);
  void SetBeamVisible(bool visible);
};

#endif  // PLAYER_H
