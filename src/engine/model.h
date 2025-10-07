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
               uint64_t shader_id,
               VertexDescription vertex_description,
               const std::string& file_name,
               const std::string& mtl_file_name,
               const std::vector<std::string>& texture_file_names);

  void CreateMesh(Renderer* renderer,
                  uint64_t shader_id,
                  VertexDescription vertex_description,
                  std::vector<float> vertices,
                  std::vector<uint32_t> indices,
                  const std::vector<std::string>& texture_file_names);

  void Update(float metallic, float roughness, float ao);

  void Draw(unsigned int instance_index);

 private:
  struct Mesh {
    size_t num_indices = 0;
    size_t index_offset = 0;
    base::Vector3f color{};
  };

  struct MaterialData {
    base::Vector3f albedo;
    float metallic;
    float roughness;
    float ao;
    float _pad0;
    float _pad1;
  };

  std::vector<Mesh> meshes_;
  Geometry geometry_;
  Texture texture_[4];
  Renderer* renderer_ = nullptr;

  std::vector<MaterialData> materials_;

  uint64_t materials_ubo_ = 0;
  uint64_t materials_dset_ = 0;

  void LoadTexture(const std::string& file_name,
                   size_t index,
                   bool is_srgb,
                   bool normalize);
};

}  // namespace eng

#endif  // ENGINE_ASSET_MODEL_H
