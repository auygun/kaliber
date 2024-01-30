#include <memory>

#include "base/vecmath.h"
#include "engine/engine.h"
#include "engine/game.h"
#include "engine/game_factory.h"
#include "engine/input_event.h"
#include "teapot/scene.h"

using namespace base;
using namespace eng;

class Teapot final : public eng::Game {
 public:
  bool Initialize() final {
    scene_.Create();
    return true;
  }

  void Update(float delta_time) final {
    Vector2f pos = last_pos_;
    float dist = last_dist_;
    while (std::unique_ptr<InputEvent> event =
               Engine::Get().GetNextInputEvent()) {
      if (event->GetType() == InputEvent::kDragStart) {
        is_active_[event->GetPointerId()] = true;
        positions_[event->GetPointerId()] = event->GetVector();
        if (is_active_[0] && is_active_[1]) {
          dist = last_dist_ = (positions_[0] - positions_[1]).Length();
        } else if (event->GetPointerId() == 0) {
          pos = last_pos_ = event->GetVector();
        }
      } else if (event->GetType() == InputEvent::kDragEnd) {
        is_active_[event->GetPointerId()] = false;
      } else if (event->GetType() == InputEvent::kDrag) {
        positions_[event->GetPointerId()] = event->GetVector();
        if (is_active_[0] && is_active_[1]) {
          dist = (positions_[0] - positions_[1]).Length();
          pos = last_pos_ = positions_[0];
        } else if (event->GetPointerId() == 0) {
          pos = event->GetVector();
        }
      }
    }
    auto angles = last_pos_ - pos;
    last_pos_ = pos;
    auto zoom = last_dist_ - dist;
    last_dist_ = dist;
    scene_.Update(angles, zoom * 80.0f);
  }

  void ContextLost() final { scene_.Create(); }

  void OnWindowResized(int width, int height) final {
    scene_.CreateProjectionMatrix();
  }

 private:
  Scene scene_;
  Vector2f last_pos_{0};
  float last_dist_ = 0;
  Vector2f positions_[2] = {Vector2f{0}, Vector2f{0}};
  bool is_active_[2] = {};
};

GAME_FACTORIES{GAME_CLASS(Teapot)};
