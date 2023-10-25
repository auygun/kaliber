#ifndef ENGINE_IMGUI_BACKEND_H
#define ENGINE_IMGUI_BACKEND_H

#include <memory>

#include "base/timer.h"

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

  void NewFrame();
  void Render();

 private:
  std::unique_ptr<Shader> shader_;
  Renderer* renderer_ = nullptr;
  base::DeltaTimer timer_;
};

}  // namespace eng

#endif  // ENGINE_IMGUI_BACKEND_H
