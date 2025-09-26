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

  void Update(float metallic, float roughness, float ao);

  void Draw();

 private:
  struct Mesh {
    size_t num_indices = 0;
    size_t index_offset = 0;
    base::Vector3f color{};
  };

  struct InstanceData {
    base::Matrix4f model;
    base::Vector3f albedo;
    float metallic;
    float roughness;
    float ao;
    float _pad0;
    float _pad1;
  };

  base::Matrix4f model_;

  std::vector<Mesh> meshes_;
  VertexDescription vertex_description_;
  Geometry geometry_;
  Texture texture_;
  Renderer* renderer_ = nullptr;

  std::vector<InstanceData> instances_;

  uint64_t instances_ubo_ = 0;
  uint64_t instances_dset_ = 0;
  uint64_t albedo_tex_dset_ = 0;
};

}  // namespace eng

#endif  // ENGINE_ASSET_MODEL_H
