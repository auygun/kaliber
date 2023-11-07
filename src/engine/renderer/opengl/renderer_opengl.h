#ifndef ENGINE_RENDERER_OPENGL_RENDERER_OPENGL_H
#define ENGINE_RENDERER_OPENGL_RENDERER_OPENGL_H

#include <array>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "engine/renderer/opengl/opengl.h"
#include "engine/renderer/renderer.h"

#if defined(__ANDROID__)
struct ANativeWindow;
#endif

namespace eng {

class RendererOpenGL final : public Renderer {
 public:
  RendererOpenGL(base::Closure context_lost_cb);
  ~RendererOpenGL() final;

  bool Initialize(Platform* platform) final;
  void Shutdown() final;

  bool IsInitialzed() const final { return is_initialized_; }

  void OnWindowResized(int width, int height) final;

  int GetScreenWidth() const final;
  int GetScreenHeight() const final;

  void SetViewport(int x, int y, int width, int height) final;
  void ResetViewport() final;

  void SetScissor(int x, int y, int width, int height) final;
  void ResetScissor() final;

  std::shared_ptr<void> CreateGeometry(std::unique_ptr<Mesh> mesh) final;
  std::shared_ptr<void> CreateGeometry(
      Primitive primitive,
      VertexDescription vertex_description,
      DataType index_description = kDataType_Invalid) final;
  void UpdateGeometry(std::shared_ptr<void> resource,
                      size_t num_vertices,
                      const void* vertices,
                      size_t num_indices,
                      const void* indices) final;

  void DestroyGeometry(std::shared_ptr<void> resource) final;
  void Draw(std::shared_ptr<void> resource,
            size_t num_indices = 0,
            size_t start_offset = 0) final;

  std::shared_ptr<void> CreateTexture() final;
  void UpdateTexture(std::shared_ptr<void> resource,
                     std::unique_ptr<Image> image) final;
  void UpdateTexture(std::shared_ptr<void> resource,
                     int width,
                     int height,
                     ImageFormat format,
                     size_t data_size,
                     uint8_t* image_data) final;
  void DestroyTexture(std::shared_ptr<void> resource) final;
  void ActivateTexture(std::shared_ptr<void> resource,
                       size_t texture_unit) final;

  std::shared_ptr<void> CreateShader(
      std::unique_ptr<ShaderSource> source,
      const VertexDescription& vertex_description,
      Primitive primitive,
      bool enable_depth_test) final;
  void DestroyShader(std::shared_ptr<void> resource) final;
  void ActivateShader(std::shared_ptr<void> resource) final;

  void SetUniform(std::shared_ptr<void> resource,
                  const std::string& name,
                  const base::Vector2f& val) final;
  void SetUniform(std::shared_ptr<void> resource,
                  const std::string& name,
                  const base::Vector3f& val) final;
  void SetUniform(std::shared_ptr<void> resource,
                  const std::string& name,
                  const base::Vector4f& val) final;
  void SetUniform(std::shared_ptr<void> resource,
                  const std::string& name,
                  const base::Matrix4f& val) final;
  void SetUniform(std::shared_ptr<void> resource,
                  const std::string& name,
                  float val) final;
  void SetUniform(std::shared_ptr<void> resource,
                  const std::string& name,
                  int val) final;
  void UploadUniforms(std::shared_ptr<void> resource) final {}

  void PrepareForDrawing() final;
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

    std::list<std::shared_ptr<GeometryOpenGL>>::iterator it;
    GLsizei num_vertices = 0;
    GLsizei num_indices = 0;
    GLenum primitive = 0;
    GLenum index_type = 0;
    std::vector<Element> vertex_layout;
    GLuint vertex_size = 0;
    GLuint vertex_array_id = 0;
    GLuint vertex_buffer_id = 0;
    GLuint index_size = 0;
    GLuint index_buffer_id = 0;
  };

  struct ShaderOpenGL {
    std::list<std::shared_ptr<ShaderOpenGL>>::iterator it;
    GLuint id = 0;
    std::vector<std::pair<size_t,  // Uniform name hash
                          GLuint   // Uniform index
                          >>
        uniforms;
    bool enable_depth_test = false;
  };

  struct TextureOpenGL {
    std::list<std::shared_ptr<TextureOpenGL>>::iterator it;
    GLuint id = 0;
  };

  std::list<std::shared_ptr<GeometryOpenGL>> geometries_;
  std::list<std::shared_ptr<ShaderOpenGL>> shaders_;
  std::list<std::shared_ptr<TextureOpenGL>> textures_;

  GLuint active_shader_id_ = 0;
  std::array<GLuint, kMaxTextureUnits> active_texture_id_ = {};

  bool vertex_array_objects_ = false;
  bool npot_ = false;

  bool is_initialized_ = false;

  // Stats.
  size_t fps_ = 0;

  int screen_width_ = 0;
  int screen_height_ = 0;

#if defined(__ANDROID__)
  ANativeWindow* window_ = nullptr;
#elif defined(__linux__)
  Display* display_ = NULL;
  Window window_ = 0;
  GLXContext glx_context_ = NULL;
#elif defined(_WIN32)
  HWND wnd_ = nullptr;
  HDC dc_ = nullptr;
  HGLRC glrc_ = nullptr;
#endif

  bool InitCommon();
  void ShutdownInternal();
  void OnDestroy();
  void ContextLost();
  void DestroyAllResources();

  bool SetupVertexLayout(const VertexDescription& vd,
                         GLuint vertex_size,
                         bool use_vao,
                         std::vector<GeometryOpenGL::Element>& vertex_layout);
  GLuint CreateShader(const char* source, GLenum type);
  bool BindAttributeLocation(GLuint id, const VertexDescription& vd);
  GLint GetUniformLocation(GLuint id,
                           const std::string& name,
                           std::vector<std::pair<size_t, GLuint>>& uniforms);
};

}  // namespace eng

#endif  // ENGINE_RENDERER_OPENGL_RENDERER_OPENGL_H
