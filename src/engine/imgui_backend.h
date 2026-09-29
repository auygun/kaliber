#ifndef ENGINE_IMGUI_BACKEND_H
#define ENGINE_IMGUI_BACKEND_H

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/vecmath.h"
#include "engine/renderer/renderer_types.h"

struct ImTextureData;

namespace eng {

class Renderer;
class Platform;

class ImguiBackend {
 public:
  static constexpr float kBaseFontSize = 16.0f;

  ImguiBackend();
  ~ImguiBackend();

  void Initialize(Platform* platform,
                  const std::string& font_path = {},
                  const std::string& fallback_font_path = {});
  void Shutdown();

  void RebuildFont(const std::string& font_path,
                   const std::string& fallback_font_path = {});

  void CreateRenderResources(Renderer* renderer);

  std::pair<bool, bool> ProcessInput(Platform* platform);

  void NewFrame(float delta_time);
  // Finalizes the ImGui frame (ImGui::Render()). Must be called once per
  // frame after all widgets have been drawn, even when GPU rendering is
  // skipped (e.g. the window is minimized).
  void Render();
  // Records the ImGui draw commands to the renderer. Call after Render().
  void Draw();

  void SetGeometryChangedCallback(std::function<void()> cb) {
    on_geometry_changed_ = std::move(cb);
  }

 private:
  struct SceneData {
    base::Matrix4f projection;
  };

  VertexDescription vertex_description_;
  std::vector<ResourceId> geometries_;
  ResourceId shader_ = 0;
  Renderer* renderer_ = nullptr;
  Platform* platform_ = nullptr;
  size_t geometry_hash_ = 0;
  std::function<void()> on_geometry_changed_;

  SceneData scene_data_;
  ResourceId scene_data_ubo_ = 0;
  ResourceId scene_dset_ = 0;

  // ImGui stores the descriptor set in ImTextureData::TexID because that is
  // what Draw() has to bind. The underlying texture must be destroyed along
  // with it, so keep the mapping here.
  std::unordered_map<ResourceId, ResourceId> dset_to_texture_;

  // Track InputText selection state for primary selection updates.
  unsigned int prev_sel_input_id_ = 0;
  int prev_sel_start_ = 0;
  int prev_sel_end_ = 0;

  void LoadFont(const std::string& font_path);
  void MergeFallbackFont(const std::string& path);
  void UpdatePrimarySelection();
  ResourceId CreateTextureAndDescriptorSet(int width,
                                         int height,
                                         const uint8_t* pixels);
  void DestroyTextureAndDescriptorSet(ResourceId dset);
  void UpdateTexture(ImTextureData* tex);
  void UpdateGeometries();
};

}  // namespace eng

#endif  // ENGINE_IMGUI_BACKEND_H
