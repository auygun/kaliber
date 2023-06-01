#ifndef ENGINE_RENDERER_OPENGL_RENDERER_OPENGL_H
#define ENGINE_RENDERER_OPENGL_RENDERER_OPENGL_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#define THREADED_RENDERING

#ifdef THREADED_RENDERING
#include <condition_variable>
#include <deque>
#include <future>
#include <mutex>
#include <semaphore>
#include <thread>
#endif  // THREADED_RENDERING

#include "engine/renderer/opengl/opengl.h"

#include "engine/renderer/renderer.h"

#if defined(__ANDROID__)
struct ANativeWindow;
#endif

#ifdef THREADED_RENDERING
namespace base {
class TaskRunner;
}
#endif  // THREADED_RENDERING

namespace eng {

struct RenderCommand;

class RendererOpenGL final : public Renderer {
 public:
  RendererOpenGL(base::Closure context_lost_cb);
  ~RendererOpenGL() final;

  bool Initialize(Platform* platform) final;
  void Shutdown() final;

  bool IsInitialzed() const final { return is_initialized_; }

  uint64_t CreateGeometry(std::unique_ptr<Mesh> mesh) final;
  void DestroyGeometry(uint64_t resource_id) final;
  void Draw(uint64_t resource_id) final;

  uint64_t CreateTexture() final;
  void UpdateTexture(uint64_t resource_id, std::unique_ptr<Image> image) final;
  void DestroyTexture(uint64_t resource_id) final;
  void ActivateTexture(uint64_t resource_id) final;

  uint64_t CreateShader(std::unique_ptr<ShaderSource> source,
                        const VertexDescripton& vertex_description,
                        Primitive primitive,
                        bool enable_depth_test) final;
  void DestroyShader(uint64_t resource_id) final;
  void ActivateShader(uint64_t resource_id) final;

  void SetUniform(uint64_t resource_id,
                  const std::string& name,
                  const base::Vector2f& val) final;
  void SetUniform(uint64_t resource_id,
                  const std::string& name,
                  const base::Vector3f& val) final;
  void SetUniform(uint64_t resource_id,
                  const std::string& name,
                  const base::Vector4f& val) final;
  void SetUniform(uint64_t resource_id,
                  const std::string& name,
                  const base::Matrix4f& val) final;
  void SetUniform(uint64_t resource_id,
                  const std::string& name,
                  float val) final;
  void SetUniform(uint64_t resource_id, const std::string& name, int val) final;
  void UploadUniforms(uint64_t resource_id) final {}

  void PrepareForDrawing() final {}
  void Present() final;

  size_t GetAndResetFPS() final;

  const char* GetDebugName() final { return "OpenGL"; }

  RendererType GetRendererType() final { return RendererType::kOpenGL; }

 private:
  struct GeometryOpenGL {
    struct Element {
      GLsizei num_elements;
      GLenum type;
      size_t vertex_offset;
    };

    GLsizei num_vertices = 0;
    GLsizei num_indices = 0;
    GLenum primitive = 0;
    GLenum index_type = 0;
    std::vector<Element> vertex_layout;
    GLuint vertex_size = 0;
    GLuint vertex_array_id = 0;
    GLuint vertex_buffer_id = 0;
    GLuint index_buffer_id = 0;
  };

  struct ShaderOpenGL {
    GLuint id = 0;
    std::unordered_map<std::string, GLuint> uniforms;
    bool enable_depth_test = false;
  };

  struct TextureOpenGL {
    GLuint id = 0;
  };

  std::unordered_map<uint64_t, GeometryOpenGL> geometries_;
  std::unordered_map<uint64_t, ShaderOpenGL> shaders_;
  std::unordered_map<uint64_t, TextureOpenGL> textures_;
  uint64_t last_resource_id_ = 0;

  GLuint active_shader_id_ = 0;
  GLuint active_texture_id_ = 0;

  bool vertex_array_objects_ = false;
  bool npot_ = false;

  bool is_initialized_ = false;

#ifdef THREADED_RENDERING
  // Global commands are independent from frames and guaranteed to be processed.
  std::deque<std::unique_ptr<RenderCommand>> global_commands_;
  // Draw commands are fame specific and can be discarded if the rendering is
  // not throttled.
  std::deque<std::unique_ptr<RenderCommand>> draw_commands_[2];

  std::condition_variable cv_;
  std::mutex mutex_;
  std::thread render_thread_;
  bool terminate_render_thread_ = false;

  std::counting_semaphore<> draw_complete_semaphore_{0};

  base::TaskRunner* main_thread_task_runner_;
#endif  // THREADED_RENDERING

  // Stats.
  size_t fps_ = 0;

#if defined(__ANDROID__)
  ANativeWindow* window_;
#elif defined(__linux__)
  Display* display_ = NULL;
  Window window_ = 0;
  GLXContext glx_context_ = NULL;
#endif

  bool InitInternal();
  bool InitCommon();
  void ShutdownInternal();

  void OnDestroy();

  void ContextLost();

  void DestroyAllResources();

  bool StartRenderThread();

#ifdef THREADED_RENDERING
  void RenderThreadMain(std::promise<bool> promise);
#endif  // THREADED_RENDERING

  void EnqueueCommand(std::unique_ptr<RenderCommand> cmd);
  void ProcessCommand(RenderCommand* cmd);

  void HandleCmdPresent(RenderCommand* cmd);
  void HandleCmdCreateTexture(RenderCommand* cmd);
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

  void BindTexture(GLuint id);
  bool SetupVertexLayout(const VertexDescripton& vd,
                         GLuint vertex_size,
                         bool use_vao,
                         std::vector<GeometryOpenGL::Element>& vertex_layout);
  GLuint CreateShader(const char* source, GLenum type);
  bool BindAttributeLocation(GLuint id, const VertexDescripton& vd);
  GLint GetUniformLocation(GLuint id,
                           const std::string& name,
                           std::unordered_map<std::string, GLuint>& uniforms);
};

}  // namespace eng

#endif  // ENGINE_RENDERER_OPENGL_RENDERER_OPENGL_H
