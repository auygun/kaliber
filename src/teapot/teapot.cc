#include <memory>

#include "base/vecmath.h"
#include "engine/components.h"
#include "engine/ecs.h"
#include "engine/engine.h"
#include "engine/game.h"
#include "engine/game_factory.h"

using namespace base;
using namespace eng;

class Teapot final : public eng::Game {
 public:
  bool Initialize() final { return true; }

  void Update(float delta_time) final {}

  void Render(float frame_frac) final {}

  void ContextLost() final {}

  void OnWindowResized(int width, int height) final {
    // scene_.CreateProjectionMatrix(); TODO
  }
};

GAME_FACTORIES{GAME_CLASS(Teapot)};
