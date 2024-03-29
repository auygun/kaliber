#include "teapot/scene.h"

#include <memory>
#include <vector>

#include "engine/asset/mesh.h"
#include "engine/asset/shader_source.h"
#include "engine/engine.h"
#include "engine/renderer/renderer_types.h"
#include "third_party/imgui/imgui.h"

using namespace base;
using namespace eng;

namespace {

void CreateSphere(std::vector<float>& vertices,
                  std::vector<unsigned short>& indices,
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

      // Texture coordinates
      float u = s * S;
      float v = r * R;
      vertices.push_back(u);
      vertices.push_back(v);

      if (r < rings - 1) {
        size_t curRow = r * sectors;
        size_t nextRow = (r + 1) * sectors;
        size_t nextS = (s + 1) % sectors;

        indices.push_back((unsigned short)(curRow + s));
        indices.push_back((unsigned short)(nextRow + s));
        indices.push_back((unsigned short)(nextRow + nextS));

        indices.push_back((unsigned short)(curRow + s));
        indices.push_back((unsigned short)(nextRow + nextS));
        indices.push_back((unsigned short)(curRow + nextS));
      }
    }
  }
}

}  // namespace

Scene::Scene() {
  camera_.Create({0, 0, 0}, -0.06f, 0.1f, 25);
  teapot_model_.CreateXRotation(0.5f);
  sphere_model_.Unit();
  sphere_model_.Multiply3x3(0.05f);
}

Scene::~Scene() = default;

void Scene::Create() {
  SetVisible(true);

  // Create a sphere to draw at each light position
  {
    static const char vertex_description[] = "p3f;n3f;t2f";
    std::vector<float> vertices;
    std::vector<unsigned short> indices;
    CreateSphere(vertices, indices, 32, 32);

    auto sphere_mesh = std::make_unique<Mesh>();
    sphere_mesh->Create(kPrimitive_Triangles, vertex_description,
                        vertices.size() / 8, vertices.data(), kDataType_UShort,
                        indices.size(), indices.data());

    sphere_geometry_.SetRenderer(Engine::Get().GetRenderer());
    sphere_geometry_.Create(std::move(sphere_mesh));
  }

  auto teapot = std::make_unique<Mesh>();
  teapot->Load("teapot/teapot.mesh");
  teapot_geometry_.SetRenderer(Engine::Get().GetRenderer());
  teapot_geometry_.Create(std::move(teapot));

  shader_.SetRenderer(Engine::Get().GetRenderer());
  auto source = std::make_unique<ShaderSource>();
  CHECK(source->Load("teapot/pbr.glsl")) << "Could not create ShaderSource";
  shader_.Create(std::move(source), teapot_geometry_.vertex_description(),
                 kPrimitive_Triangles, true);

  CreateProjectionMatrix();
}

void Scene::Draw(float frame_frac) {
  Matrix4f view;
  camera_.GetMatrix().InverseOrthogonal(view);

  Matrix4f view_projection;
  view.Multiply(projection_, view_projection);

  shader_.Activate();
  shader_.SetUniform("model", teapot_model_);
  shader_.SetUniform("view_projection", view_projection);
  shader_.SetUniform("light_pos1", Vector3f(-15, -4, -15));
  shader_.SetUniform("light_pos2", Vector3f(15, -4, -15));
  shader_.SetUniform("light_pos3", Vector3f(-15, -4, 15));
  shader_.SetUniform("light_pos4", Vector3f(15, -4, 15));
  shader_.SetUniform("light_power1", light1_power_);
  shader_.SetUniform("light_power2", light2_power_);
  shader_.SetUniform("light_power3", light3_power_);
  shader_.SetUniform("light_power4", light4_power_);
  shader_.SetUniform("cam_pos", camera_.GetMatrix().Row(3));
  shader_.SetUniform("albedo", albedo_);
  shader_.SetUniform("metallic", metallic_);
  shader_.SetUniform("roughness", roughness_);
  shader_.SetUniform("ao", ao_);
  teapot_geometry_.Draw();

  shader_.SetUniform("albedo", Vector3f(1, 1, 1));
  shader_.SetUniform("metallic", 0.0f);
  shader_.SetUniform("roughness", 1.0f);
  shader_.SetUniform("ao", 1.0f);

  sphere_model_.Row(3) = Vector3f(-15, -4, -15);
  shader_.SetUniform("model", sphere_model_);
  sphere_geometry_.Draw();

  sphere_model_.Row(3) = Vector3f(15, -4, -15);
  shader_.SetUniform("model", sphere_model_);
  sphere_geometry_.Draw();

  sphere_model_.Row(3) = Vector3f(-15, -4, 15);
  shader_.SetUniform("model", sphere_model_);
  sphere_geometry_.Draw();

  sphere_model_.Row(3) = Vector3f(15, -4, 15);
  shader_.SetUniform("model", sphere_model_);
  sphere_geometry_.Draw();
}

void Scene::Update(const Vector2f& angles, float zoom) {
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
    ImGui::SliderFloat("light 1", &light1_power_, 0.0f, 500.0f, "%.f");
    ImGui::SliderFloat("light 2", &light2_power_, 0.0f, 500.0f, "%.f");
    ImGui::SliderFloat("light 3", &light3_power_, 0.0f, 500.0f, "%.f");
    ImGui::SliderFloat("light 4", &light4_power_, 0.0f, 500.0f, "%.f");
  }
  ImGui::End();

  RendererType selected_type = static_cast<RendererType>(renderer_type);
  if (selected_type != Engine::Get().GetRendererType())
    Engine::Get().CreateRenderer(selected_type);
}

void Scene::CreateProjectionMatrix() {
  projection_.CreatePerspectiveProjection(
      45, 4.0f / 3.0f, (float)Engine::Get().GetScreenWidth(),
      (float)Engine::Get().GetScreenHeight(), 4, 2048);
}
