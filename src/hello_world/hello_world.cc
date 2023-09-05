#include "base/interpolation.h"
#include "base/vecmath.h"
#include "engine/animator.h"
#include "engine/asset/image.h"
#include "engine/engine.h"
#include "engine/game.h"
#include "engine/game_factory.h"
#include "engine/image_quad.h"

class HelloWorld final : public eng::Game {
 public:
  bool Initialize() final {
    eng::Engine::Get().SetImageSource(
        "hello_world_image",
        std::bind(&eng::Engine::Print, &eng::Engine::Get(), "Hello World",
                  /*bg_color*/ base::Vector4f(1, 1, 1, 0)));

    hello_world_.Create("hello_world_image").SetVisible(true);
    animator_.Attach(&hello_world_);
    animator_.SetRotation(base::PI2f, /*duration*/ 3,
                          std::bind(base::SmootherStep, std::placeholders::_1));
    animator_.Play(eng::Animator::kRotation, /*loop*/ true);
    return true;
  }

 private:
  eng::ImageQuad hello_world_;
  eng::Animator animator_;
};

GAME_FACTORIES{GAME_CLASS(HelloWorld)};
