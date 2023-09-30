#include "engine/drawable.h"

#include "engine/engine.h"
#include "engine/renderer/shader.h"

namespace eng {

Drawable::Drawable() {
  Engine::Get().AddDrawable(this);
}

Drawable::~Drawable() {
  Engine::Get().RemoveDrawable(this);
}

void Drawable::SetCustomShader(const std::string& asset_name) {
  custom_shader_ = Engine::Get().GetShader(asset_name);
  custom_uniforms_.clear();
}

void Drawable::DoSetCustomUniforms() {
  if (custom_shader_) {
    for (auto& cu : custom_uniforms_)
      std::visit([&](auto&& arg) { custom_shader_->SetUniform(cu.first, arg); },
                 cu.second);
  }
}

}  // namespace eng
