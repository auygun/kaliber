#ifndef RENDERER_OPENGL_H
#define RENDERER_OPENGL_H

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
#include <thread>

#include "../../../base/semaphore.h"
#endif  // THREADED_RENDERING

#include "opengl.h"

#include "../render_resource.h"
#include "../renderer.h"

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

class RendererOpenGL : public Renderer {
 public:
  RendererOpenGL();
  ~RendererOpenGL() override;

#if defined(__ANDROID__)
  bool Initialize(ANativeWindow* window) override;
#elif defined(__linux__)
  bool Initialize(Display* display, Window window) override;
#endif

  void Shutdown() override;

  void CreateGeometry(std::shared_ptr<void> impl_data,
                      std::unique_ptr<Mesh> mesh) override;
  void DestroyGeometry(std::shared_ptr<void> impl_data) override;
  void Draw(std::shared_ptr<void> impl_data) override;

  void UpdateTexture(std::shared_ptr<void> impl_data,
                     std::unique_ptr<Image> image) override;
  void DestroyTexture(std::shared_ptr<void> impl_data) override;
  void ActivateTexture(std::shared_ptr<void> impl_data) override;

  void CreateShader(std::shared_ptr<void> impl_data,
                    std::unique_ptr<ShaderSource> source,
                    const VertexDescripton& vertex_description,
                    Primitive primitive,
                    bool enable_depth_test) override;
  void DestroyShader(std::shared_ptr<void> impl_data) override;
  void ActivateShader(std::shared_ptr<void> impl_data) override;

  void SetUniform(std::shared_ptr<void> impl_data,
                  const std::string& name,
                  const base::Vector2f& val) override;
  void SetUniform(std::shared_ptr<void> impl_data,
                  const std::string& name,
                  const base::Vector3f& val) override;
  void SetUniform(std::shared_ptr<void> impl_data,
                  const std::string& name,
                  const base::Vector4f& val) override;
  void SetUniform(std::shared_ptr<void> impl_data,
                  const std::string& name,
                  const base::Matrix4f& val) override;
  void SetUniform(std::shared_ptr<void> impl_data,
                  const std::string& name,
                  float val) override;
  void SetUniform(std::shared_ptr<void> impl_data,
                  const std::string& name,
                  int val) override;
  void UploadUniforms(std::shared_ptr<void> impl_data) override {}

  void PrepareForDrawing() override {}
  void Present() override;

  std::unique_ptr<RenderResource> CreateResource(
      RenderResourceFactoryBase& factory) override;
  void ReleaseResource(unsigned resource_id) override;

  size_t GetAndResetFPS() override;

#if defined(__linux__) && !defined(__ANDROID__)
  XVisualInfo* GetXVisualInfo(Display* display) override;
#endif

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

  GLuint active_shader_id_ = 0;
  GLuint active_texture_id_ = 0;

  bool vertex_array_objects_ = false;
  bool npot_ = false;

  std::unordered_map<unsigned, RenderResource*> resources_;

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

  base::Semaphore draw_complete_semaphore_;

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

  void ContextLost();

  void InvalidateAllResources();

  bool StartRenderThread();
  void TerminateRenderThread();

#ifdef THREADED_RENDERING
  void RenderThreadMain(std::promise<bool> promise);
#endif  // THREADED_RENDERING

  void EnqueueCommand(std::unique_ptr<RenderCommand> cmd);
  void ProcessCommand(RenderCommand* cmd);

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

#endif  // RENDERER_OPENGL_H
