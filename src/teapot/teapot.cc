#include <memory>

#include "base/vecmath.h"
#include "engine/asset_manager.h"
#include "engine/components.h"
#include "engine/ecs.h"
#include "engine/engine.h"
#include "engine/game.h"
#include "engine/game_factory.h"
#include "imgui.h"

using namespace base;
using namespace eng;

class Teapot final : public eng::Game {
 public:
  bool Initialize(World& world) final {
    render_context_ =
        &world.GetRegistry().GetSingletonComponent<RenderContext>();

    // Load Assets via AssetManager
    auto& asset_manager = Engine::Get().GetAssetManager();
    uint64_t shader = world.GetShaderId();

    // 0: Cube
    uint32_t cube_id = asset_manager.LoadGLTF("teapot/Cube.gltf", shader);

    // 1: Sports Car
    uint32_t car_id = asset_manager.LoadObj("teapot/sportsCar.obj", shader,
                                            "teapot/sportsCar.mtl");

    // 2: Cerberus
    uint32_t gun_id = asset_manager.LoadObj(
        "teapot/Cerberus_LP.obj", shader, "teapot/Cerberus_LP.mtl",
        {"teapot/Cerberus_A.tga", "teapot/Cerberus_N.tga",
         "teapot/Cerberus_M.tga", "teapot/Cerberus_R.tga"});

    // 3: WaterBottle
    uint32_t bottle_id =
        asset_manager.LoadGLTF("teapot/WaterBottle.glb", shader);

    // 4: Avocado
    uint32_t avocado_id = asset_manager.LoadGLTF("teapot/Avocado.glb", shader);

    // 5: BarramundiFish
    uint32_t fish_id =
        asset_manager.LoadGLTF("teapot/BarramundiFish.glb", shader);

    // 6: Sphere
    uint32_t sphere_id = asset_manager.CreateSphere(
        shader, 32, 32,
        {"teapot/alien-slime1-albedo.png", "teapot/alien-slime1-normal-dx.png",
         "teapot/alien-slime1-metallic.png",
         "teapot/alien-slime1-roughness.png"});

    // Instantiate Entities
    Entity root =
        std::get<0>(*world.GetRegistry().View<SceneNodeComponent>().begin());

    // Instantiate Cubes
    Entity parent = root;
    for (size_t i = 0; i < 10; ++i) {
      Matrix4f transform;
      transform.Create(Quatf({0.0f, 0.1f, 0.0f}), {2.2f, 0, 0});
      Entity entity = world.CreateSceneNode("cube", parent, transform);
      world.AddModelComponent(entity, cube_id);
      parent = entity;
    }

    // Instantiate Cars
    parent = root;
    for (size_t i = 0; i < 3; ++i) {
      Matrix4f transform;
      transform.Create(Quatf({0.0f, 0.1f, 0.0f}), {2.2f, -2.0f, 0});
      Entity entity = world.CreateSceneNode("car", parent, transform);
      world.AddModelComponent(entity, car_id);
      parent = entity;
    }

    // Instantiate Gun
    {
      Matrix4f transform;
      transform.Create(Quatf({0.0f, 0.0f, 0.0f}),
                       Vector3f{200.0f, -100.0f, 0.0f});
      transform.Multiply(0.05f);
      Entity entity = world.CreateSceneNode("gun", root, transform);
      world.AddModelComponent(entity, gun_id);
    }

    // Instantiate Bottle
    {
      Matrix4f transform;
      transform.Create(Quatf({0.0f, 0.0f, 0.0f}), Vector3f{0.0f, -0.5f, 0.0f});
      transform.Multiply(10.0f);
      Entity entity = world.CreateSceneNode("bottle", root, transform);
      world.AddModelComponent(entity, bottle_id);
    }

    // Instantiate Avocado
    {
      Matrix4f transform;
      transform.Create(Quatf({0.0f, 0.0f, 0.0f}), Vector3f{0});
      transform.Multiply(30.0f);
      transform.Row(3) = Vector3f{0.0f, -3.0f, 3.0f};
      Entity entity = world.CreateSceneNode("avcd", root, transform);
      world.AddModelComponent(entity, avocado_id);
    }

    // Instantiate Fish
    {
      Matrix4f transform;
      transform.Create(Quatf({0.0f, 0.0f, 0.0f}), Vector3f{0.0f, -0.5f, 0.7f});
      transform.Multiply(10.0f);
      Entity entity = world.CreateSceneNode("fish", root, transform);
      world.AddModelComponent(entity, fish_id);
    }

    // Instantiate Spheres
    // parent = root;
    for (size_t i = 0; i < 10; ++i) {
      Matrix4f transform;
      transform.Create(Quatf({0.0f, 0.0f, 0.1f}), {2.2f, 0, 0});
      Entity entity = world.CreateSceneNode("sphere", parent, transform);
      world.AddModelComponent(entity, sphere_id);
      parent = entity;
    }

    return true;
  }

  void FixedUpdate(World& world) final {}

  void Update(World& world, float delta_time) final {
    float label_width = ImGui::CalcTextSize("roughness").x;
    ImGui::SetNextWindowSize(ImVec2(label_width * 3.0f, -1.0f), ImGuiCond_Once);
    if (ImGui::Begin("Teapot", nullptr,
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav |
                         ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoResize)) {
      ImGui::PushItemWidth(-label_width);
      // ImGui::RadioButton("Vulkan", &renderer_type, 1);
      // ImGui::SameLine();
      // ImGui::RadioButton("OpenGL", &renderer_type, 2);
      ImGui::Checkbox("Volumes", &render_context_->show_bounding_volumes);
      // ImGui::SliderFloat("light 1", &lights_[0].power, 0.0f, 2000.0f, "%.f");
      // ImGui::SliderFloat("light 2", &lights_[1].power, 0.0f, 2000.0f, "%.f");
      // ImGui::SliderFloat("light 3", &lights_[2].power, 0.0f, 2000.0f, "%.f");
      // ImGui::SliderFloat("light 4", &lights_[3].power, 0.0f, 2000.0f, "%.f");
      ImGui::SliderFloat("white", &render_context_->white, 0.0f, 30.0f, "%.1f");
      ImGui::SliderFloat("exposure", &render_context_->exposure, 0.0f, 20.0f,
                         "%.1f");
    }
    ImGui::End();
  }

  void ContextLost() final {}

  void OnWindowResized(int width, int height) final {
    // scene_.CreateProjectionMatrix(); TODO
  }

 private:
  RenderContext* render_context_;
};

GAME_FACTORIES{GAME_CLASS(Teapot)};
