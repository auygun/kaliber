#include "teapot/scene.h"

#include <memory>

#include "engine/asset/shader_source.h"
#include "engine/engine.h"
#include "engine/renderer/renderer.h"
#include "third_party/imgui/imgui.h"

using namespace base;
using namespace eng;

namespace {

const char vertex_description[] = "p3f;n3f;a3f;t2f";

[[maybe_unused]] void CreateSphere(std::vector<float>& vertices,
                                   std::vector<uint32_t>& indices,
                                   size_t rings,
                                   size_t sectors) {
  float const R = 1. / (float)(rings - 1);
  float const S = 1. / (float)(sectors - 1);

  for (size_t r = 0; r < rings; ++r) {
    for (size_t s = 0; s < sectors; ++s) {
      float y = sin(-PIHALFf + PIf * r * R);
      float x = cos(2 * PIf * s * S) * sin(PIf * r * R);
      float z = sin(2 * PIf * s * S) * sin(PIf * r * R);

      // Position
      vertices.push_back(x);
      vertices.push_back(y);
      vertices.push_back(z);

      // Normal
      vertices.push_back(x);
      vertices.push_back(y);
      vertices.push_back(z);

      // Tangent
      vertices.push_back(0);
      vertices.push_back(0);
      vertices.push_back(0);

      // Texture coordinates
      float u = s * S;
      float v = r * R;
      vertices.push_back(u);
      vertices.push_back(v);

      if (r < rings - 1) {
        size_t curRow = r * sectors;
        size_t nextRow = (r + 1) * sectors;
        size_t nextS = (s + 1) % sectors;

        indices.push_back((uint32_t)(curRow + s));
        indices.push_back((uint32_t)(nextRow + s));
        indices.push_back((uint32_t)(nextRow + nextS));

        indices.push_back((uint32_t)(curRow + s));
        indices.push_back((uint32_t)(nextRow + nextS));
        indices.push_back((uint32_t)(curRow + nextS));
      }
    }
  }
}

}  // namespace

Scene::Scene() {
  camera_.Create({0, 0, 0}, -0.06f, 0.1f, 5);
}

Scene::~Scene() = default;

void Scene::Create() {
  // TestBVH();

  SetVisible(true);

  debug_layer_.CreateRenderResources(Engine::Get().GetRenderer());

  if (!ParseVertexDescription(vertex_description, vertex_description_)) {
    LOG(0) << "Failed to parse vertex description.";
    return;
  }

  shader_.SetRenderer(Engine::Get().GetRenderer());
  auto source = std::make_unique<ShaderSource>();
  CHECK(source->Load("teapot/pbr.glsl")) << "Could not create ShaderSource";
  shader_.Create(std::move(source), vertex_description_, kPrimitive_Triangles,
                 true, false, CullMode::kBack);

#if 0

  // model_.LoadObj(Engine::Get().GetRenderer(), shader_.resource_id(),
  //                "teapot/viking_room.obj", "", {"teapot/viking_room.png"});
  model_.LoadObj(Engine::Get().GetRenderer(), shader_.resource_id(),
                 "teapot/buddha.obj", "", {});
  // model_.LoadObj(Engine::Get().GetRenderer(), shader_.resource_id(),
  //                "teapot/sportsCar.obj", "teapot/sportsCar.mtl", {});
  // model_.LoadObj(Engine::Get().GetRenderer(), shader_.resource_id(),
  //                "teapot/Cerberus_LP.obj", "teapot/Cerberus_LP.mtl",
  //                {"teapot/Cerberus_A.tga", "teapot/Cerberus_N.tga",
  //                 "teapot/Cerberus_M.tga", "teapot/Cerberus_R.tga"});

  for (size_t i = 0; i < 1; ++i) {
    instances_.emplace_back().model.CreateXRotation(0.5f);
    instances_.back().model.Row(3) = {2.2f * i, 0, 0};
  }

#else

  std::vector<float> vertices;
  std::vector<uint32_t> indices;
  CreateSphere(vertices, indices, 32, 32);
  model_.CreateMesh(
      Engine::Get().GetRenderer(), shader_.resource_id(), vertices, indices,
      // {"teapot/iron-rusted4-basecolor.png", "teapot/iron-rusted4-normal.png",
      //  "teapot/iron-rusted4-metalness.png",
      //  "teapot/iron-rusted4-roughness.png"});
      // {"teapot/greasy-pan-2-albedo.png", "teapot/greasy-pan-2-normal.png",
      //  "teapot/greasy-pan-2-metal.png",
      //  "teapot/greasy-pan-2-roughness.png"});
      // {"teapot/grimy-metal-albedo.png", "teapot/grimy-metal-normal-dx.png",
      //  "teapot/grimy-metal-metalness.png",
      //  "teapot/grimy-metal-roughness.png"});
      // {"teapot/steelplate1_albedo.png", "teapot/steelplate1_normal.png",
      //  "teapot/steelplate1_metallic.png",
      //  "teapot/steelplate1_roughness.png"});
      {"teapot/alien-slime1-albedo.png", "teapot/alien-slime1-normal-dx.png",
       "teapot/alien-slime1-metallic.png",
       "teapot/alien-slime1-roughness.png"});

  std::vector<eng::MeshObject> bvh_mesh_objects;
  for (size_t i = 0; i < 10; ++i) {
    instances_.emplace_back().model.CreateFromAngles({0.1f, 0.1f, 0.1f}, 0);
    auto& model_mat = instances_.back().model;
    model_mat.Row(3) = {2.2f * i, 0, 0};

    OBBf obb{model_mat, model_.GetExtents()};
    bvh_mesh_objects.emplace_back(i, obb, model_mat);
  }
  bvh_tree_ = BuildBVHTree(std::move(bvh_mesh_objects));
  DumpBVHTree(bvh_tree_, 0, "");

#endif

  scene_data_ubo_ = Engine::Get().GetRenderer()->CreateBuffer(
      shader_.resource_id(), 1, 0, sizeof(SceneData));
  lights_ubo_ = Engine::Get().GetRenderer()->CreateBuffer(
      shader_.resource_id(), 1, 1, sizeof(lights_));
  instances_ubo_ = Engine::Get().GetRenderer()->CreateBuffer(
      shader_.resource_id(), 1, 2, sizeof(InstanceData) * instances_.size());
  scene_dset_ = Engine::Get().GetRenderer()->CreateDescriptorSet(
      shader_.resource_id(), 1, {},
      {scene_data_ubo_, lights_ubo_, instances_ubo_});

  Engine::Get().GetRenderer()->UpdateBuffer(
      instances_ubo_, instances_.data(),
      sizeof(InstanceData) * instances_.size());

  CreateProjectionMatrix();

  lights_[0].pos = {-15, -4, -15};
  lights_[1].pos = {15, -4, -15};
  lights_[2].pos = {-15, -4, 15};
  lights_[3].pos = {15, -4, 15};
  lights_[0].power = 400.0f;
  lights_[1].power = 400.0f;
  lights_[2].power = 400.0f;
  lights_[3].power = 400.0f;
}

void Scene::Draw(float frame_frac) {
  shader_.Activate();
  Engine::Get().GetRenderer()->ActivateDescriptorSet(scene_dset_);
  model_.Draw(instances_.size());

  Matrix4f view, view_projection;
  camera_.GetMatrix().InverseOrthogonal(view);
  view.Multiply(projection_, view_projection);
  debug_layer_.Draw(view_projection);
}

void Scene::Update(float delta_time, const Vector2f& angles, float zoom) {
  camera_.Orbit(-angles.y, angles.x, zoom);

  int renderer_type = static_cast<int>(Engine::Get().GetRendererType());

  float label_width = ImGui::CalcTextSize("roughness").x;
  ImGui::SetNextWindowSize(ImVec2(label_width * 3.0f, -1.0f), ImGuiCond_Once);
  if (ImGui::Begin("Teapot", nullptr,
                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav |
                       ImGuiWindowFlags_NoSavedSettings |
                       ImGuiWindowFlags_NoResize)) {
    ImGui::PushItemWidth(-label_width);
    ImGui::RadioButton("Vulkan", &renderer_type, 1);
    ImGui::SameLine();
    ImGui::RadioButton("OpenGL", &renderer_type, 2);
    ImGui::ColorEdit4("albedo", albedo_.k, ImGuiColorEditFlags_NoAlpha);
    ImGui::SliderFloat("metallic", &metallic_, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("roughness", &roughness_, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("ambient", &ao_, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("light 1", &lights_[0].power, 0.0f, 2000.0f, "%.f");
    ImGui::SliderFloat("light 2", &lights_[1].power, 0.0f, 2000.0f, "%.f");
    ImGui::SliderFloat("light 3", &lights_[2].power, 0.0f, 2000.0f, "%.f");
    ImGui::SliderFloat("light 4", &lights_[3].power, 0.0f, 2000.0f, "%.f");
    ImGui::SliderFloat("white", &scene_data_.white, 0.0f, 30.0f, "%.1f");
    ImGui::SliderFloat("exposure", &scene_data_.exposure, 0.0f, 20.0f, "%.1f");
  }
  ImGui::End();

  RendererType selected_type = static_cast<RendererType>(renderer_type);
  if (selected_type != Engine::Get().GetRendererType())
    Engine::Get().CreateRenderer(selected_type);

  debug_layer_.Update(delta_time);

  for (auto& i : instances_)
    debug_layer_.DrawMatrix(i.model);
  DrawBVHTree(bvh_tree_, 0, debug_layer_);

  Matrix4f view;
  camera_.GetMatrix().InverseOrthogonal(view);
  view.Multiply(projection_, scene_data_.view_projection);
  scene_data_.cam_pos = camera_.GetMatrix().Row(3);
  scene_data_.light_dir = {1, 1, 1};
  scene_data_.light_radiance = {1, 1, 1};
  Engine::Get().GetRenderer()->UpdateBuffer(scene_data_ubo_, &scene_data_,
                                            sizeof(scene_data_));

  Engine::Get().GetRenderer()->UpdateBuffer(lights_ubo_, &lights_,
                                            sizeof(lights_));

  model_.UpdateMaterial(metallic_, roughness_, ao_);

  Frustumf f;
  Matrix4f f_view, f_view_projection;
  camera_.GetMatrixMainCam().InverseOrthogonal(f_view);
  f_view.Multiply(projection_, f_view_projection);
  f.CreateFromMatrix(f_view_projection);
  debug_layer_.DrawFrustum(f.planes);
  debug_layer_.DrawMatrix(camera_.GetMatrixMainCam());
  auto objects = FrustumCull(bvh_tree_, f);
  DLOG(0) << "FrustumCull: " << objects.size();
}

void Scene::CreateProjectionMatrix() {
  projection_.CreatePerspectiveProjection(
      45, (float)Engine::Get().GetScreenWidth(),
      (float)Engine::Get().GetScreenHeight(), 1.0f, 1000);
}
