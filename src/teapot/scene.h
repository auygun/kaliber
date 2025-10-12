#ifndef TEAPOT_SCENE_H
#define TEAPOT_SCENE_H

#include <memory>

#include "base/vecmath.h"
#include "engine/drawable.h"
#include "engine/model.h"
#include "engine/renderer/renderer_types.h"
#include "engine/renderer/shader.h"
#include "teapot/bvh.h"
#include "teapot/camera.h"

class Scene : public eng::Drawable {
 public:
  Scene();
  ~Scene();

  void Create();

  void Draw(float frame_frac) override;

  void Update(const base::Vector2f& angles, float zoom);

  void CreateProjectionMatrix();

 private:
  struct SceneData {
    base::Matrix4f view_projection;
    base::Vector3f cam_pos;
    float white = 5.0f;
    float exposure = 1.0f;
    float _pad0;
    float _pad1;
    float _pad2;
  };

  struct LightData {
    base::Vector3f pos;
    float power = 0;
  };

  struct InstanceData {
    base::Matrix4f model;
  };

  eng::VertexDescription vertex_description_;
  eng::Shader shader_;
  eng::Model model_;

  Camera camera_;
  base::Matrix4f projection_;

  std::unique_ptr<BVHNode> bvh_root_;
  std::vector<MeshObject> bvh_mesh_objects_;
  std::vector<const MeshObject*> bvh_mesh_object_ptrs_;

  base::Vector3f albedo_{0.8f, 0.4f, 0.2f};
  float metallic_ = 1.0f;
  float roughness_ = 0.3f;
  float ao_ = 0.5f;

  SceneData scene_data_;
  LightData lights_[4];
  std::vector<InstanceData> instances_;

  uint64_t scene_data_ubo_ = 0;
  uint64_t lights_ubo_ = 0;
  uint64_t instances_ubo_ = 0;
  uint64_t scene_dset_ = 0;
};

#endif  // TEAPOT_SCENE_H
