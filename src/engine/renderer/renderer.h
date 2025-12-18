#ifndef ENGINE_RENDERER_RENDERER_H
#define ENGINE_RENDERER_RENDERER_H

#include <memory>
#include <string>
#include <vector>

#include "base/closure.h"
#include "base/vecmath.h"
#include "engine/renderer/renderer_types.h"

namespace eng {

class Image;
class ShaderSource;
class Mesh;
class Platform;

enum class RendererType { kUnknown, kVulkan };

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

  virtual uint64_t CreateGeometry(std::unique_ptr<Mesh> mesh) = 0;
  virtual uint64_t CreateGeometry(
      VertexDescription vertex_description,
      DataType index_description = kDataType_Invalid) = 0;
  virtual void UpdateGeometry(uint64_t resource_id,
                              size_t num_vertices,
                              const void* vertices,
                              size_t num_indices,
                              const void* indices) = 0;
  virtual void DestroyGeometry(uint64_t resource_id) = 0;
  virtual void ActivateGeometry(uint64_t resource_id) = 0;
  virtual void Draw(size_t num_indices = 0,
                    size_t start_offset = 0,
                    size_t instance_count = 1,
                    size_t first_instance = 0) = 0;

  virtual uint64_t CreateTexture() = 0;
  virtual void UpdateTexture(uint64_t resource_id,
                             std::unique_ptr<Image> image) = 0;
  virtual void UpdateTexture(uint64_t resource_id,
                             std::vector<std::unique_ptr<Image>> images) = 0;
  virtual void UpdateTexture(uint64_t resource_id,
                             int width,
                             int height,
                             int num_mip_levels,
                             int mip_level,
                             ImageFormat format,
                             size_t data_size,
                             uint8_t* image_data) = 0;
  virtual void DestroyTexture(uint64_t resource_id) = 0;

  virtual uint64_t CreateShader(std::unique_ptr<ShaderSource> source,
                                const VertexDescription& vertex_description,
                                Primitive primitive,
                                bool enable_depth_test,
                                bool wireframe,
                                CullMode cull_mode) = 0;
  virtual void DestroyShader(uint64_t resource_id) = 0;
  virtual void ActivateShader(uint64_t resource_id) = 0;

  virtual void UpdatePushConstants(size_t size, const void* data) = 0;

  virtual uint64_t CreateBuffer(uint64_t shader_id,
                                size_t set,
                                size_t binding,
                                uint32_t buffer_size) = 0;
  virtual void UpdateBuffer(uint64_t resource_id,
                            const void* data,
                            size_t size) = 0;
  virtual void DestroyBuffer(uint64_t resource_id) = 0;

  virtual uint64_t CreateDescriptorSet(
      uint64_t shader_id,
      size_t set,
      const std::vector<std::vector<uint64_t>>& textures,
      const std::vector<uint64_t>& buffers) = 0;
  virtual void ActivateDescriptorSet(uint64_t resource_id) = 0;
  virtual void DestroyDescriptorSet(uint64_t resource_id) = 0;

  virtual void PrepareForDrawing() = 0;
  virtual void Present() = 0;

  virtual void BeginRenderToTexture(uint64_t texture_id) = 0;
  virtual void EndRenderToTexture(uint64_t texture_id) = 0;

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
