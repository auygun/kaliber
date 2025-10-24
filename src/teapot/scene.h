#ifndef TEAPOT_SCENE_H
#define TEAPOT_SCENE_H

#include <vector>

#include "base/vecmath.h"
#include "engine/bvh.h"
#include "engine/drawable.h"
#include "engine/model.h"
#include "engine/debug_layer.h"
#include "engine/scene_graph.h"
#include "engine/renderer/renderer_types.h"
#include "engine/renderer/shader.h"
#include "teapot/camera.h"

class Scene : public eng::Drawable {
 public:
  Scene();
  ~Scene();

  void Create();

  void Draw(float frame_frac) override;

  void Update(float delta_time, const base::Vector2f& angles, float zoom);

  void CreateProjectionMatrix();

  Camera& GetCamera() { return camera_; }

 private:
  struct SceneData {
    base::Matrix4f view_projection;
    base::Vector3f cam_pos;
    float white = 5.0f;
    base::Vector3f light_dir;
    float exposure = 1.0f;
    base::Vector3f light_radiance;
    float _pad0;
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

  eng::DebugLayer debug_layer_;

  eng::SceneG scene_;
  std::vector<eng::BVHNode> bvh_tree_;

  base::Vector3f albedo_{0.8f, 0.4f, 0.2f};
  float metallic_ = 1.0f;
  float roughness_ = 0.3f;
  float ao_ = 0.5f;

  SceneData scene_data_;
  LightData lights_[4];

  uint64_t scene_data_ubo_ = 0;
  uint64_t lights_ubo_ = 0;
  uint64_t instances_ubo_ = 0;
  uint64_t scene_dset_ = 0;
};

#endif  // TEAPOT_SCENE_H
