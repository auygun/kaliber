#ifndef TEAPOT_SCENE_H
#define TEAPOT_SCENE_H

#include "base/vecmath.h"
#include "engine/drawable.h"
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
  eng::Shader shader_;

  eng::Geometry teapot_geometry_;
  base::Matrix4f teapot_model_;

  eng::Geometry sphere_geometry_;
  base::Matrix4f sphere_model_;

  Camera camera_;
  base::Matrix4f projection_;

  base::Vector3f albedo_{0.8f, 0.4f, 0.2f};
  float metallic_ = 1.0f;
  float roughness_ = 0.3f;
  float ao_ = 0.5f;
  float light1_power_ = 400.0f;
  float light2_power_ = 400.0f;
  float light3_power_ = 400.0f;
  float light4_power_ = 400.0f;
};

#endif  // TEAPOT_SCENE_H
