#ifndef MOVIE_QUAD_H
#define MOVIE_QUAD_H

#include "../base/vecmath.h"
#include "renderer/texture.h"
#include "drawable.h"

#include <string>
#include <vector>
#include <memory>

namespace engine {

class Image;

class MovieQuad : public Drawable {
 public:
  MovieQuad() = default;
  ~MovieQuad() override = default;

  bool Create(std::vector<std::shared_ptr<const Image>> images);

  size_t GetNumTextures() { return textures_.size(); }
  void SetActiveTexture(int i) { active_texture_ = i; }

  void Draw() override;

  void Translate(const Vector2& offset) { offset_ = offset; }
  void Scale(const Vector2& scale) { scale_ = scale; }

  Vector2 offset() { return offset_; }
  Vector2 scale() { return scale_; }

 private:
  std::vector<std::unique_ptr<Texture>> textures_;
  size_t active_texture_ = 0;
  Vector2 offset_ = {0, 0};
  Vector2 scale_ = {1, 1};
};

}  // namespace engine

#endif  // MOVIE_QUAD_H
