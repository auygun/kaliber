#include "drawable.h"
#include "engine.h"

namespace eng {

Drawable::Drawable() {
  Engine::Get().AddDrawable(this);
}

Drawable::~Drawable() {
  Engine::Get().RemoveDrawable(this);
}

}  // namespace eng
