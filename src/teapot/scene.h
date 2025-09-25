#ifndef TEAPOT_SCENE_H
#define TEAPOT_SCENE_H

#include "base/vecmath.h"
#include "engine/drawable.h"
#include "engine/model.h"
#include "engine/renderer/geometry.h"
#include "engine/renderer/shader.h"
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
  struct LightData {
    base::Vector3f pos{};
    float power = 0;
  };

  struct Ubo1Data {
    LightData lights[4];
    base::Vector3f cam_pos;
    float _pad0;
  };

  eng::Shader shader_;

  eng::Geometry teapot_geometry_;
  base::Matrix4f teapot_model_;

  eng::Geometry sphere_geometry_;
  base::Matrix4f sphere_model_;

  eng::Model model_;

  Camera camera_;
  base::Matrix4f projection_;
  base::Matrix4f view_projection_;

  base::Vector3f albedo_{0.8f, 0.4f, 0.2f};
  float metallic_ = 1.0f;
  float roughness_ = 0.3f;
  float ao_ = 0.5f;
  Ubo1Data ubo1_data_;

  uint64_t ubo0_ = 0;
  uint64_t ubo1_ = 0;
  uint32_t desc_set1_ = 0;
};

#endif  // TEAPOT_SCENE_H
