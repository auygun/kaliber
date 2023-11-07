#ifndef ENGINE_RENDERER_RENDERER_H
#define ENGINE_RENDERER_RENDERER_H

#include <memory>
#include <string>

#include "base/closure.h"
#include "base/vecmath.h"
#include "engine/renderer/renderer_types.h"

namespace eng {

class Image;
class ShaderSource;
class Mesh;
class Platform;

enum class RendererType { kUnknown, kVulkan, kOpenGL };

class Renderer {
 public:
  static const unsigned kInvalidId = 0;
  static const unsigned kMaxTextureUnits = 8;

  static std::unique_ptr<Renderer> Create(RendererType type,
                                          base::Closure context_lost_cb);

  Renderer(base::Closure context_lost_cb)
      : context_lost_cb_{std::move(context_lost_cb)} {}
  virtual ~Renderer() = default;

  virtual bool Initialize(Platform* platform) = 0;
  virtual void Shutdown() = 0;

  virtual bool IsInitialzed() const = 0;

  virtual void OnWindowResized(int width, int height) = 0;

  virtual int GetScreenWidth() const = 0;
  virtual int GetScreenHeight() const = 0;

  virtual void SetViewport(int x, int y, int width, int height) = 0;
  virtual void ResetViewport() = 0;

  virtual void SetScissor(int x, int y, int width, int height) = 0;
  virtual void ResetScissor() = 0;

  virtual std::shared_ptr<void> CreateGeometry(std::unique_ptr<Mesh> mesh) = 0;
  virtual std::shared_ptr<void> CreateGeometry(
      Primitive primitive,
      VertexDescription vertex_description,
      DataType index_description = kDataType_Invalid) = 0;
  virtual void UpdateGeometry(std::shared_ptr<void> resource,
                              size_t num_vertices,
                              const void* vertices,
                              size_t num_indices,
                              const void* indices) = 0;
  virtual void DestroyGeometry(std::shared_ptr<void> resource) = 0;
  virtual void Draw(std::shared_ptr<void> resource,
                    size_t num_indices = 0,
                    size_t start_offset = 0) = 0;

  virtual std::shared_ptr<void> CreateTexture() = 0;
  virtual void UpdateTexture(std::shared_ptr<void> resource,
                             std::unique_ptr<Image> image) = 0;
  virtual void UpdateTexture(std::shared_ptr<void> resource,
                             int width,
                             int height,
                             ImageFormat format,
                             size_t data_size,
                             uint8_t* image_data) = 0;
  virtual void DestroyTexture(std::shared_ptr<void> resource) = 0;
  virtual void ActivateTexture(std::shared_ptr<void> resource,
                               size_t texture_unit) = 0;

  virtual std::shared_ptr<void> CreateShader(
      std::unique_ptr<ShaderSource> source,
      const VertexDescription& vertex_description,
      Primitive primitive,
      bool enable_depth_test) = 0;
  virtual void DestroyShader(std::shared_ptr<void> resource) = 0;
  virtual void ActivateShader(std::shared_ptr<void> resource) = 0;

  virtual void SetUniform(std::shared_ptr<void> resource,
                          const std::string& name,
                          const base::Vector2f& val) = 0;
  virtual void SetUniform(std::shared_ptr<void> resource,
                          const std::string& name,
                          const base::Vector3f& val) = 0;
  virtual void SetUniform(std::shared_ptr<void> resource,
                          const std::string& name,
                          const base::Vector4f& val) = 0;
  virtual void SetUniform(std::shared_ptr<void> resource,
                          const std::string& name,
                          const base::Matrix4f& val) = 0;
  virtual void SetUniform(std::shared_ptr<void> resource,
                          const std::string& name,
                          float val) = 0;
  virtual void SetUniform(std::shared_ptr<void> resource,
                          const std::string& name,
                          int val) = 0;
  virtual void UploadUniforms(std::shared_ptr<void> resource) = 0;

  virtual void PrepareForDrawing() = 0;
  virtual void Present() = 0;

  bool SupportsETC1() const { return texture_compression_.etc1; }
  bool SupportsDXT1() const {
    return texture_compression_.dxt1 || texture_compression_.s3tc;
  }
  bool SupportsDXT5() const { return texture_compression_.s3tc; }
  bool SupportsATC() const { return texture_compression_.atc; }

  virtual size_t GetAndResetFPS() = 0;

  virtual const char* GetDebugName() = 0;

  virtual RendererType GetRendererType() { return RendererType::kUnknown; }

 protected:
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

  base::Closure context_lost_cb_;

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;
};

}  // namespace eng

#endif  // ENGINE_RENDERER_RENDERER_H
