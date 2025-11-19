#ifndef ENGINE_MODEL_H
#define ENGINE_MODEL_H

#include <memory>
#include <string>
#include <vector>

#include "base/vecmath.h"

namespace eng {

class Mesh;
class Renderer;

class Model {
 public:
  struct Vertex {
    base::Vector3f position{0};
    base::Vector3f normal{0};
    base::Vector4f tangent{0};
    float uv[2]{0, 0};
  };

  Model() = default;
  ~Model();

  bool LoadObj(Renderer* renderer,
               uint64_t shader_id,
               const std::string& file_name,
               const std::string& mtl_file_name,
               const std::vector<std::string>& texture_file_names);

  bool LoadGLTF(Renderer* renderer,
                uint64_t shader_id,
                const std::string& file_name);

  void CreateMesh(Renderer* renderer,
                  uint64_t shader_id,
                  std::vector<Vertex> vertices,
                  std::vector<uint32_t> indices,
                  const std::vector<std::string>& texture_file_names);

  void Draw(unsigned int instance_index, unsigned int fist_instance);

  const base::Vector3f& GetExtents() const { return extents_; }

 private:
  struct DrawCmd {
    size_t num_indices = 0;
    size_t index_offset = 0;
  };

  struct MaterialData {
    base::Vector4f albedo;
    float metallic;
    float roughness;
    float ao;
    float _pad0;
  };

  base::Vector3f extents_{0};

  std::vector<DrawCmd> draw_list_;
  uint64_t geometry_id_ = 0;
  uint64_t texture_ids_[4] = {0, 0, 0, 0};
  Renderer* renderer_ = nullptr;

  bool is_material_ = true;
  std::vector<MaterialData> materials_;

  uint64_t materials_ubo_ = 0;
  uint64_t materials_dset_ = 0;

  std::unique_ptr<Mesh> ProcessMesh(
      std::vector<Vertex> raw_vertices,
      std::vector<uint32_t> aggregated_indices,
      const std::vector<size_t>& material_indices_counts,
      bool generate_tangents);

  void GenerateTangents(const std::vector<uint32_t>& indices,
                        std::vector<Vertex>& vertices);

  void CreateRenderResources(
      uint64_t shader_id,
      std::unique_ptr<Mesh> mesh,
      const std::vector<std::string>& texture_file_names);

  void LoadTexture(const std::string& file_name,
                   size_t index,
                   bool is_srgb,
                   bool normalize);
};

}  // namespace eng

#endif  // ENGINE_MODEL_H
