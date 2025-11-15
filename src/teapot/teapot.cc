#include <memory>

#include "base/vecmath.h"
#include "engine/components.h"
#include "engine/ecs.h"
#include "engine/engine.h"
#include "engine/game.h"
#include "engine/game_factory.h"
#include "third_party/imgui/imgui.h"

using namespace base;
using namespace eng;

class Teapot final : public eng::Game {
 public:
  bool Initialize(World& world) final {
    render_context_ =
        &world.GetRegistry().GetSingletonComponent<RenderContext>();
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
