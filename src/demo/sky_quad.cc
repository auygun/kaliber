#include "demo/sky_quad.h"

#include "base/interpolation.h"
#include "base/log.h"
#include "base/random.h"
#include "engine/engine.h"
#include "engine/renderer/geometry.h"
#include "engine/renderer/shader.h"

using namespace base;
using namespace eng;

SkyQuad::SkyQuad()
    : sky_offset_{
          0, Lerp(0.0f, 10.0f, Engine::Get().GetRandomGenerator().Rand())} {
  last_sky_offset_ = sky_offset_;
}

SkyQuad::~SkyQuad() = default;

bool SkyQuad::Create(bool without_nebula) {
  without_nebula_ = without_nebula;
  scale_ = Engine::Get().GetScreenSize();
  shader_ =
      Engine::Get().GetShader(without_nebula ? "sky_without_nebula" : "sky");

  color_animator_.Attach(this);

  SetVisible(true);

  return true;
}

void SkyQuad::Update(float delta_time) {
  last_sky_offset_ = sky_offset_;
  sky_offset_ += {0, delta_time * speed_};
}

void SkyQuad::Draw(float frame_frac) {
  Vector2f sky_offset = Lerp(last_sky_offset_, sky_offset_, frame_frac);

  shader_->Activate();
  shader_->SetUniform("scale", scale_);
  shader_->SetUniform("projection", Engine::Get().GetProjectionMatrix());
  shader_->SetUniform("sky_offset", sky_offset);
  if (!without_nebula_)
    shader_->SetUniform("nebula_color",
                        {nebula_color_.x, nebula_color_.y, nebula_color_.z});
  shader_->UploadUniforms();

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