#include <memory>

#include "base/vecmath.h"
#include "engine/engine.h"
#include "engine/game.h"
#include "engine/game_factory.h"
#include "teapot/components.h"
#include "teapot/ecs.h"
#include "teapot/scene.h"

using namespace base;
using namespace eng;

class Teapot final : public eng::Game {
 public:
  bool Initialize() final {
    scene_.Create(Engine::Get().GetRenderer());
    return true;
  }

  void Update(float delta_time) final {
    scene_.Update(delta_time);
  }

  void Render(float frame_frac) final { scene_.Render(frame_frac); }

  void ContextLost() final { scene_.Create(Engine::Get().GetRenderer()); }

  void OnWindowResized(int width, int height) final {
    // scene_.CreateProjectionMatrix(); TODO
  }

 private:
  Scene scene_;
  Vector2f last_pos_{0};
  float last_dist_ = 0;
  Vector2f positions_[2] = {Vector2f{0}, Vector2f{0}};
  bool is_active_[2] = {};
};

GAME_FACTORIES{GAME_CLASS(Teapot)};
