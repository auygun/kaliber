#ifndef RENDERER_H
#define RENDERER_H

#include "opengl.h"
#if defined(__linux__) && !defined(__ANDROID__)
#include <X11/Xlib.h>
#include "../../third_party/glew/glxew.h"
#endif

#include "texture.h"
#include "shader.h"
#include "geometry.h"
#include "render_command.h"
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <array>

#define THREADED_RENDERING

#ifdef THREADED_RENDERING
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <deque>
#endif // THREADED_RENDERING

namespace engine {

class Renderer {
 public:
  Renderer() = default;

  bool StartWorker();
  void TerminateWorker();

  bool Init();
  void Shutdown();

  void EnableBlend();
  void Clear(const std::array<float, 4>& rgba);
  void Present();

  void ContextLost();

  void TrimMemory();

  int GetScreenWidth() { return screen_width_; }
  int GetScreenHeight() { return screen_height_; }

  bool SupportsETC1() const { return texture_compression_.etc1; }
  bool SupportsDXT1() const {
    return texture_compression_.dxt1 || texture_compression_.s3tc;
  }
  bool SupportsDXT5() const { return texture_compression_.s3tc; }
  bool SupportsATC() const { return texture_compression_.atc; }

  bool SupportsVAO() const { return vertex_array_objects_; }

#if defined(__linux__) && !defined(__ANDROID__)
  bool CreateWindow();
  void DestroyWindow();

  Display* display() { return display_; }
  Window window() { return window_; }
#endif

  void EnqueueCommand(std::unique_ptr<RenderCommand> cmd);

 private:
  struct TextureCompression {
    unsigned etc1 : 1;
    unsigned dxt1 : 1;
    unsigned latc : 1;
    unsigned s3tc : 1;
    unsigned pvrtc : 1;
    unsigned atc : 1;

    TextureCompression()
        : etc1(false),
          dxt1(false),
          latc(false),
          s3tc(false),
          pvrtc(false),
          atc(false) {}
  };

  TextureCompression texture_compression_;
  bool vertex_array_objects_ = false;

  int screen_width_ = 0;
  int screen_height_ = 0;

  std::unordered_map<int, std::unique_ptr<Texture>> texture_map_;
  std::unordered_map<int, std::unique_ptr<Geometry>> geometry_map_;
  std::unordered_map<int, std::unique_ptr<Shader>> shader_map_;

#ifdef THREADED_RENDERING
  std::deque<std::unique_ptr<RenderCommand>> command_queue_;

  std::condition_variable cv_;
  std::mutex mutex_;
  std::thread worker_thread_;
  bool terminate_worker_ = false;
#endif // THREADED_RENDERING

#if defined(__linux__) && !defined(__ANDROID__)
  Display* display_ = NULL;
  Window window_ = 0;
  XVisualInfo* visual_info_;
  GLXContext glx_context_ = NULL;
#endif

#ifdef THREADED_RENDERING
  void WorkerMain(std::promise<bool> promise);
#else
  void WorkerMain(std::unique_ptr<RenderCommand> cmd);
#endif // THREADED_RENDERING

  void HandleCmdEnableBlend(std::unique_ptr<RenderCommand> cmd);
  void HandleCmdClear(std::unique_ptr<RenderCommand> cmd);
  void HandleCmdPresent(std::unique_ptr<RenderCommand> cmd);
  void HandleCmdCreateTexture(std::unique_ptr<RenderCommand> cmd);
  void HandleCmdDestoryTexture(std::unique_ptr<RenderCommand> cmd);
  void HandleCmdActivateTexture(std::unique_ptr<RenderCommand> cmd);
  void HandleCmdCreateGeometry(std::unique_ptr<RenderCommand> cmd);
  void HandleCmdDestroyGeometry(std::unique_ptr<RenderCommand> cmd);
  void HandleCmdDrawGeometry(std::unique_ptr<RenderCommand> cmd);
  void HandleCmdCreateShader(std::unique_ptr<RenderCommand> cmd);
  void HandleCmdDestroyShader(std::unique_ptr<RenderCommand> cmd);
  void HandleCmdActivateShader(std::unique_ptr<RenderCommand> cmd);
  void HandleCmdSetUniformVec2(std::unique_ptr<RenderCommand> cmd);
  void HandleCmdSetUniformVec3(std::unique_ptr<RenderCommand> cmd);
  void HandleCmdSetUniformInt(std::unique_ptr<RenderCommand> cmd);

  std::unordered_set<std::string> SetupExtensions();

  void LogVersion();
};

}  // namespace engine

#endif  // RENDERER_H
