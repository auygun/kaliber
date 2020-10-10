#include "sky_quad.h"

#include "../base/interpolation.h"
#include "../base/log.h"
#include "../base/random.h"
#include "../engine/engine.h"
#include "../engine/renderer/geometry.h"
#include "../engine/renderer/shader.h"
#include "../engine/shader_source.h"

using namespace base;
using namespace eng;

SkyQuad::SkyQuad()
    : shader_(Engine::Get().CreateRenderResource<Shader>()),
      sky_offset_{
          0, Lerp(0.0f, 10.0f, Engine::Get().GetRandomGenerator().GetFloat())} {
}

SkyQuad::~SkyQuad() = default;

bool SkyQuad::Create(bool without_nebula) {
  without_nebula_ = without_nebula;

  Engine& engine = Engine::Get();

  auto source = std::make_unique<ShaderSource>();
  if (!source->Load(without_nebula_ ? "sky_without_nebula.glsl" : "sky.glsl"))
    return false;
  shader_->Create(std::move(source), engine.GetQuad()->vertex_description());

  scale_ = engine.GetScreenSize();

  color_animator_.Attach(this);

  SetVisible(true);

  return true;
}

void SkyQuad::Update(float delta_time) {
  sky_offset_ += {0, delta_time * 0.04f};
  color_animator_.Update(delta_time);
}

void SkyQuad::Draw(float frame_frac) {
  Vector2 sky_offset = Lerp(last_sky_offset_, sky_offset_, frame_frac);

  shader_->Activate();
  shader_->SetUniform("scale", scale_);
  shader_->SetUniform("projection", Engine::Get().GetProjectionMatrix());
  shader_->SetUniform("sky_offset", sky_offset);
  if (!without_nebula_)
    shader_->SetUniform("nebula_color",
                        {nebula_color_.x, nebula_color_.y, nebula_color_.z});

  Engine::Get().GetQuad()->Draw();
  last_sky_offset_ = sky_offset_;
}

void SkyQuad::ContextLost() {
  Create(without_nebula_);
}

void SkyQuad::SwitchColor(const Vector4& color) {
  color_animator_.Pause(Animator::kBlending);
  color_animator_.SetTime(Animator::kBlending, 0);
  color_animator_.SetBlending(color, 5,
                              std::bind(SmoothStep, std::placeholders::_1));
  color_animator_.Play(Animator::kBlending, false);
}
