#ifndef ENGINE_IMGUI_BACKEND_H
#define ENGINE_IMGUI_BACKEND_H

#include <vector>

#include "engine/renderer/geometry.h"
#include "engine/renderer/renderer_types.h"
#include "engine/renderer/shader.h"
#include "engine/renderer/texture.h"

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
  std::vector<Geometry> geometries_;
  Shader shader_;
  Texture font_atlas_;
  Renderer* renderer_ = nullptr;
};

}  // namespace eng

#endif  // ENGINE_IMGUI_BACKEND_H
