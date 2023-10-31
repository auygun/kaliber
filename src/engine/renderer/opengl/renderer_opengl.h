#ifndef ENGINE_RENDERER_OPENGL_RENDERER_OPENGL_H
#define ENGINE_RENDERER_OPENGL_RENDERER_OPENGL_H

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
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

  uint64_t CreateGeometry(std::unique_ptr<Mesh> mesh) final;
  uint64_t CreateGeometry(Primitive primitive,
                          VertexDescription vertex_description,
                          DataType index_description = kDataType_Invalid) final;
  void UpdateGeometry(uint64_t resource_id,
                      size_t num_vertices,
                      const void* vertices,
                      size_t num_indices,
                      const void* indices) final;

  void DestroyGeometry(uint64_t resource_id) final;
  void Draw(uint64_t resource_id,
            uint64_t num_indices = 0,
            uint64_t start_offset = 0) final;

  uint64_t CreateTexture() final;
  void UpdateTexture(uint64_t resource_id, std::unique_ptr<Image> image) final;
  void DestroyTexture(uint64_t resource_id) final;
  void ActivateTexture(uint64_t resource_id, uint64_t texture_unit) final;

  uint64_t CreateShader(std::unique_ptr<ShaderSource> source,
                        const VertexDescription& vertex_description,
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
    GLuint id = 0;
    std::vector<std::pair<size_t,  // Uniform name hash
                          GLuint   // Uniform index
                          >>
        uniforms;
    bool enable_depth_test = false;
  };

  std::unordered_map<uint64_t, GeometryOpenGL> geometries_;
  std::unordered_map<uint64_t, ShaderOpenGL> shaders_;
  std::unordered_map<uint64_t, GLuint> textures_;
  uint64_t last_resource_id_ = 0;

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
