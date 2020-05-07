#include "drawable.h"
#include "engine.h"

namespace engine {

Drawable::~Drawable() {
  Engine::Get().RemoveDrawable(this);
}

}  // namespace engine
