#include "demo/sky_quad.h"

#include "base/interpolation.h"
#include "base/log.h"
#include "base/random.h"
#include "engine/asset/image.h"
#include "engine/engine.h"
#include "engine/renderer/geometry.h"
#include "engine/renderer/shader.h"
#include "engine/renderer/texture.h"

using namespace base;
using namespace eng;

SkyQuad::SkyQuad()
    : sky_offset_{
          0, Lerp(0.0f, 10.0f, Engine::Get().GetRandomGenerator().Rand())} {
  last_sky_offset_ = sky_offset_;
}

SkyQuad::~SkyQuad() = default;

bool SkyQuad::Create() {
  Engine::Get().SetImageSource(
      "sky_tex",
      []() -> std::unique_ptr<Image> {
        auto image = std::make_unique<Image>();
        image->Create(Engine::Get().GetScreenWidth() * 0.6f,
                      Engine::Get().GetScreenHeight() * 0.6f);
        image->Clear({1, 1, 1, 0});
        return image;
      },
      true);

  shader_ = Engine::Get().GetShader("sky");
  texture_ = Engine::Get().AcquireTexture("sky_tex");

  color_animator_.Attach(this);

  SetVisible(true);

  return true;
}

void SkyQuad::Update(float delta_time) {
  last_sky_offset_ = sky_offset_;
  sky_offset_ += {0, delta_time * speed_};
}

void SkyQuad::Draw(float frame_frac) {
  texture_->SetAsRenderTarget();

  Vector2f sky_offset = Lerp(last_sky_offset_, sky_offset_, frame_frac);

  shader_->Activate();
  shader_->SetUniform("scale", Engine::Get().GetViewportSize());
  shader_->SetUniform("projection", Engine::Get().GetProjectionMatrix());
  shader_->SetUniform("sky_offset", -sky_offset);
  shader_->SetUniform("nebula_color",
                      {nebula_color_.x, nebula_color_.y, nebula_color_.z});
  Engine::Get().GetQuad().Draw();

  texture_->EndRenderTarget();

  texture_->Activate(0);

  Shader* shader = &Engine::Get().GetPassThroughShader();
  shader->Activate();
  shader->SetUniform("offset", position_);
  shader->SetUniform("scale", Engine::Get().GetViewportSize());
  shader->SetUniform("rotation", rotation_);
  shader->SetUniform("tex_offset", {0, 0});
  shader->SetUniform("tex_scale", {1, 1});
  shader->SetUniform("projection", Engine::Get().GetProjectionMatrix());
  shader->SetUniform("color", {1, 1, 1, 1});
  shader->SetUniform("texture_0", 0);
  Engine::Get().GetQuad().Draw();
}

void SkyQuad::SwitchColor(const Vector4f& color) {
  color_animator_.Pause(Animator::kBlending);
  color_animator_.SetTime(Animator::kBlending, 0);
  color_animator_.SetBlending(color, 5,
                              std::bind(SmoothStep, std::placeholders::_1));
  color_animator_.Play(Animator::kBlending, false);
}

void SkyQuad::SetSpeed(float speed) {
  speed_ = speed;
}