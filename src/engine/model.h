#ifndef ENGINE_ASSET_MODEL_H
#define ENGINE_ASSET_MODEL_H

#include <string>
#include <vector>

#include "engine/renderer/geometry.h"
#include "engine/renderer/renderer_types.h"
#include "engine/renderer/shader.h"
#include "engine/renderer/texture.h"

namespace eng {

class Renderer;

class Model {
 public:
  Model() = default;
  ~Model() = default;

  bool LoadObj(Renderer* renderer, const std::string& file_name);

  void Draw();

 private:
  VertexDescription vertex_description_;
  std::vector<Geometry> geometries_;
  Shader shader_;
  Texture texture_;
  uint32_t desc_set0_ = 0;
  Renderer* renderer_ = nullptr;
};

}  // namespace eng

#endif  // ENGINE_ASSET_MODEL_H
