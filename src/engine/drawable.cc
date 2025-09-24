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
}

}  // namespace eng
