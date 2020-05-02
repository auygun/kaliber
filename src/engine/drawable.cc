#include "drawable.h"
#include <math.h>

namespace engine {

void Drawable::Translate(const Vector2& offset) {
  offset_ += offset;
}

void Drawable::Scale(const Vector2& scale) {
  scale_ *= scale;
}

void Drawable::Scale(float scale) {
  scale_ *= scale;
}

void Drawable::Rotate(float angle) {
  rotation_.x = sin(angle);
  rotation_.y = cos(angle);

}  // namespace engine
}
