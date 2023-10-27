#ifndef ENGINE_IMGUI_BACKEND_H
#define ENGINE_IMGUI_BACKEND_H

#include <memory>

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
  void Draw();

 private:
  std::unique_ptr<Shader> shader_;
  Renderer* renderer_ = nullptr;
  bool is_new_frame_ = false;
};

}  // namespace eng

#endif  // ENGINE_IMGUI_BACKEND_H
