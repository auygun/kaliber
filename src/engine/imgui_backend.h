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
                  bool use_freetype = true,
                  const std::string& fallback_font_path = {});
  void Shutdown();

  void RebuildFont(const std::string& font_path,
                   bool use_freetype,
                   const std::string& fallback_font_path = {});
  void SetFontLoader(bool use_freetype);

  void CreateRenderResources(Renderer* renderer);

  std::pair<bool, bool> ProcessInput(Platform* platform);

  void NewFrame(float delta_time);
  void Draw();

  void SetGeometryChangedCallback(std::function<void()> cb) {
    on_geometry_changed_ = std::move(cb);
  }

 private:
  struct SceneData {
    base::Matrix4f projection;
  };

  VertexDescription vertex_description_;
  std::vector<uint64_t> geometries_;
  uint64_t shader_ = 0;
  Renderer* renderer_ = nullptr;
  Platform* platform_ = nullptr;
  size_t geometry_hash_ = 0;
  std::function<void()> on_geometry_changed_;

  SceneData scene_data_;
  uint64_t scene_data_ubo_ = 0;
  uint64_t scene_dset_ = 0;

  // ImGui stores the descriptor set in ImTextureData::TexID because that is
  // what Draw() has to bind. The underlying texture must be destroyed along
  // with it, so keep the mapping here.
  std::unordered_map<uint64_t, uint64_t> dset_to_texture_;

  // Track InputText selection state for primary selection updates.
#if defined(OS_LINUX)
  unsigned int prev_sel_input_id_ = 0;
  int prev_sel_start_ = 0;
  int prev_sel_end_ = 0;
#endif

  void LoadFont(const std::string& font_path);
  void MergeFallbackFont(const std::string& path);
#if defined(OS_LINUX)
  void UpdatePrimarySelection();
#endif
  uint64_t CreateTextureAndDescriptorSet(int width,
                                         int height,
                                         const uint8_t* pixels);
  void DestroyTextureAndDescriptorSet(uint64_t dset);
  void UpdateTexture(ImTextureData* tex);
  void UpdateGeometries();
};

}  // namespace eng

#endif  // ENGINE_IMGUI_BACKEND_H
