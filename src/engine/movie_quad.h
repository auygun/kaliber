#ifndef MOVIE_QUAD_H
#define MOVIE_QUAD_H

#include "../base/vecmath.h"
#include "renderer/texture.h"
#include "drawable.h"
#include "frame_controller.h"

#include <string>
#include <vector>
#include <memory>

namespace engine {

class Image;

class MovieQuad : public Drawable, public FrameController {
 public:
  MovieQuad() = default;
  ~MovieQuad() override = default;

  bool Create(std::vector<std::shared_ptr<const Image>> images);

  size_t GetNumFrames() override;
  size_t GetCurrentFrame() override;
  void SetCurrentFrame(size_t frame) override;

  void Draw() override;

  size_t active_texture() { return active_texture_; }

 private:
  std::vector<std::unique_ptr<Texture>> textures_;
  size_t active_texture_ = 0;
  Vector2 uv_scale_ = {1, 1};
};

}  // namespace engine

#endif  // MOVIE_QUAD_H
