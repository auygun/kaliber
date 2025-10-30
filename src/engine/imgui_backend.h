#ifndef ENGINE_IMGUI_BACKEND_H
#define ENGINE_IMGUI_BACKEND_H

#include <memory>
#include <vector>

#include "base/vecmath.h"
#include "engine/renderer/renderer_types.h"

namespace eng {

class InputEvent;
class Renderer;

class ImguiBackend {
 public:
  ImguiBackend();
  ~ImguiBackend();

  void Initialize(bool is_mobile, std::string root_path);
  void Shutdown();

  void CreateRenderResources(Renderer* renderer);

  std::unique_ptr<InputEvent> OnInputEvent(std::unique_ptr<InputEvent> event);

  void NewFrame(float delta_time);
  void EndFrame();
  void Draw();

 private:
  struct SceneData {
    base::Matrix4f projection;
  };

  VertexDescription vertex_description_;
  std::vector<uint64_t> geometries_;
  uint64_t shader_;
  uint64_t font_atlas_;
  uint32_t texture_dset_ = 0;
  Renderer* renderer_ = nullptr;
  bool needs_update_ = false;

  SceneData scene_data_;
  uint64_t scene_data_ubo_ = 0;
  uint64_t scene_dset_ = 0;

  void UpdateGeometries();
};

}  // namespace eng

#endif  // ENGINE_IMGUI_BACKEND_H
