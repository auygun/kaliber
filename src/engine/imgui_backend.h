#ifndef ENGINE_IMGUI_BACKEND_H
#define ENGINE_IMGUI_BACKEND_H

#include <memory>
#include <vector>

namespace eng {

class InputEvent;
class Shader;
class Renderer;

class ImguiBackend {
 public:
  ImguiBackend();
  ~ImguiBackend();

  void Initialize();
  void Shutdown();

  void CreateRenderResources(Renderer* renderer);

  std::unique_ptr<InputEvent> OnInputEvent(std::unique_ptr<InputEvent> event);

  void NewFrame(float delta_time);
  void Render();
  void Draw();

 private:
  std::vector<uint64_t> geometries_;
  std::unique_ptr<Shader> shader_;
  Renderer* renderer_ = nullptr;
};

}  // namespace eng

#endif  // ENGINE_IMGUI_BACKEND_H
