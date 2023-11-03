#ifndef ENGINE_IMGUI_BACKEND_H
#define ENGINE_IMGUI_BACKEND_H

#include <memory>
#include <vector>

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
  void Render();
  void Draw();

 private:
  VertexDescription vertex_description_;
  std::vector<uint64_t> geometries_;
  uint64_t shader_ = 0;
  uint64_t font_atlas_ = 0;
  Renderer* renderer_ = nullptr;
};

}  // namespace eng

#endif  // ENGINE_IMGUI_BACKEND_H
