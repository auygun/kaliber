#ifndef RENDERER_H
#define RENDERER_H

#include "opengl.h"
#if defined(__linux__) && !defined(__ANDROID__)
#include <X11/Xlib.h>
#include "../../third_party/glew/glxew.h"
#endif

#include "../../base/vecmath.h"
#include "../../base/callback.h"
#include "types.h"
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <array>
#include <vector>

#define THREADED_RENDERING

#ifdef THREADED_RENDERING
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <deque>
#endif // THREADED_RENDERING

#if defined(__ANDROID__)
struct ANativeWindow;
#endif

namespace eng {

struct RenderCommand;
class Image;

class Renderer {
 public:
  Renderer();
  ~Renderer();

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  void SetContextLostCB(base::Callback cb);

#if defined(__ANDROID__)
  bool Init(ANativeWindow* window);
#elif defined(__linux__)
  bool Init();
#endif

  void Shutdown();

  void Present();

  void ContextLost();

  bool SupportsETC1() const { return texture_compression_.etc1; }
  bool SupportsDXT1() const { return texture_compression_.dxt1 ||
                                     texture_compression_.s3tc; }
  bool SupportsDXT5() const { return texture_compression_.s3tc; }
  bool SupportsATC() const { return texture_compression_.atc; }

  bool SupportsVAO() const { return vertex_array_objects_; }

  void EnqueueCommand(std::unique_ptr<RenderCommand> cmd);

  int screen_width() const { return screen_width_; }
  int screen_height() const { return screen_height_; }
  const base::Matrix4x4& projection() const { return projection_; }

  size_t num_frames_dropped() { return num_frames_dropped_; }
  size_t global_queue_size() { return global_queue_size_; }
  size_t render_queue_size() { return render_queue_size_; }

#if defined(__linux__) && !defined(__ANDROID__)
  Display* display() { return display_; }
  Window window() { return window_; }
#endif

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

  struct Geometry {
    struct Element {
      GLsizei num_elements;
      GLenum type;
      size_t vertex_offset;
    };

    int num_vertices;
    int num_indices;
    GLenum primitive;
    GLenum index_type;
    std::vector<Element> vertex_layout;
    GLuint vertex_size;
    GLuint vertex_array_id;
    GLuint vertex_buffer_id;
    GLuint index_buffer_id;
  };

  struct Shader {
    GLuint id;
    std::unordered_map<std::string, GLuint> uniforms;
  };

  base::Callback context_lost_cb_;

  TextureCompression texture_compression_;
  bool vertex_array_objects_ = false;

  int screen_width_ = 0;
  int screen_height_ = 0;
  base::Matrix4x4 projection_;

  std::unordered_map<int, GLuint> texture_map_;
  std::unordered_map<int, Geometry> geometry_map_;
  std::unordered_map<int, Shader> shader_map_;

#ifdef THREADED_RENDERING
  // Global commands are independent from frames and guaranteed to be processed.
  std::deque<std::unique_ptr<RenderCommand>> global_commands_;
  // Draw commands are fame specific and can be discarded if the renderer deems
  // frame drop.
  std::deque<std::unique_ptr<RenderCommand>> draw_commands_[2];

  std::condition_variable cv_;
  std::mutex mutex_;
  std::thread worker_thread_;
  bool terminate_worker_ = false;

  std::atomic<bool> context_lost_ = false;
#endif // THREADED_RENDERING

  // Stats.
  size_t num_frames_dropped_ = 0;
  size_t global_queue_size_ = 0;
  size_t render_queue_size_ = 0;

#if defined(__ANDROID__)
  ANativeWindow *window_;
#elif defined(__linux__)
  Display* display_ = NULL;
  Window window_ = 0;
  XVisualInfo* visual_info_;
  GLXContext glx_context_ = NULL;
#endif

  bool InitInternal();
  bool InitCommon();
  void ShutdownInternal();

  bool StartWorker();
  void TerminateWorker();

#ifdef THREADED_RENDERING
  void WorkerMain(std::promise<bool> promise);
#endif // THREADED_RENDERING

  void ProcessCommand(RenderCommand* cmd);

  void HandleCmdEnableBlend(RenderCommand* cmd);
  void HandleCmdClear(RenderCommand* cmd);
  void HandleCmdPresent(RenderCommand* cmd);
  void HandleCmdUpdateTexture(RenderCommand* cmd);
  void HandleCmdDestoryTexture(RenderCommand* cmd);
  void HandleCmdActivateTexture(RenderCommand* cmd);
  void HandleCmdCreateGeometry(RenderCommand* cmd);
  void HandleCmdDestroyGeometry(RenderCommand* cmd);
  void HandleCmdDrawGeometry(RenderCommand* cmd);
  void HandleCmdCreateShader(RenderCommand* cmd);
  void HandleCmdDestroyShader(RenderCommand* cmd);
  void HandleCmdActivateShader(RenderCommand* cmd);
  void HandleCmdSetUniformVec2(RenderCommand* cmd);
  void HandleCmdSetUniformVec3(RenderCommand* cmd);
  void HandleCmdSetUniformVec4(RenderCommand* cmd);
  void HandleCmdSetUniformMat4(RenderCommand* cmd);
  void HandleCmdSetUniformFloat(RenderCommand* cmd);
  void HandleCmdSetUniformInt(RenderCommand* cmd);

  bool SetupVertexLayout(const VertexDescripton &vd,
                         GLuint vertex_size,
                         bool use_vao,
                         std::vector<Geometry::Element> &vertex_layout);
  GLuint CreateShader(const char *source, GLenum type);
  bool BindAttributeLocation(GLuint id, const VertexDescripton &vd);
  GLint GetUniformLocation(GLuint id, const std::string &name, std::unordered_map<std::string, GLuint> &uniforms);
  std::unordered_set<std::string> SetupExtensions();

  void LogVersion();

#if defined(__linux__) && !defined(__ANDROID__)
  bool CreateWindow();
  void DestroyWindow();
#endif
};

}  // namespace eng

#endif  // RENDERER_H
