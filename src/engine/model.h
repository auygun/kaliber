#ifndef ENGINE_ASSET_MODEL_H
#define ENGINE_ASSET_MODEL_H

#include <string>
#include <vector>

#include "base/vecmath.h"
#include "engine/renderer/geometry.h"
#include "engine/renderer/renderer_types.h"
#include "engine/renderer/texture.h"

namespace eng {

class Renderer;

class Model {
 public:
  Model() = default;
  ~Model() = default;

  bool LoadObj(Renderer* renderer,
               const std::string& file_name,
               const std::string& mtl_file_name,
               const std::string& tex_file_name,
               uint64_t shader_id);

  void Draw(const base::Matrix4f& model,
            const base::Matrix4f& view_projection,
            const base::Vector3f& cam_pos,
            float metallic,
            float roughness,
            float ao);

 private:
  struct Mesh {
    size_t num_indices = 0;
    size_t index_offset = 0;
    base::Vector3f color{};
  };

  std::vector<Mesh> meshes_;
  VertexDescription vertex_description_;
  Geometry geometry_;
  Texture texture_;
  uint32_t desc_set0_ = 0;
  Renderer* renderer_ = nullptr;
};

}  // namespace eng

#endif  // ENGINE_ASSET_MODEL_H
