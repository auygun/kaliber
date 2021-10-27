#include "engine/drawable.h"

#include "engine/engine.h"

namespace eng {

Drawable::Drawable() {
  Engine::Get().AddDrawable(this);
}

Drawable::~Drawable() {
  Engine::Get().RemoveDrawable(this);
}

}  // namespace eng
