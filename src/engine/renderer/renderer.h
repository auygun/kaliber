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

class Renderer {
 public:
  const unsigned kInvalidId = 0;

  Renderer(base::Closure context_lost_cb)
      : context_lost_cb_{std::move(context_lost_cb)} {}
  virtual ~Renderer() = default;

  virtual bool Initialize(Platform* platform) = 0;
  virtual void Shutdown() = 0;

  virtual uint64_t CreateGeometry(std::unique_ptr<Mesh> mesh) = 0;
  virtual void DestroyGeometry(uint64_t resource_id) = 0;
  virtual void Draw(uint64_t resource_id) = 0;

  virtual uint64_t CreateTexture() = 0;
  virtual void UpdateTexture(uint64_t resource_id,
                             std::unique_ptr<Image> image) = 0;
  virtual void DestroyTexture(uint64_t resource_id) = 0;
  virtual void ActivateTexture(uint64_t resource_id) = 0;

  virtual uint64_t CreateShader(std::unique_ptr<ShaderSource> source,
                                const VertexDescripton& vertex_description,
                                Primitive primitive,
                                bool enable_depth_test) = 0;
  virtual void DestroyShader(uint64_t resource_id) = 0;
  virtual void ActivateShader(uint64_t resource_id) = 0;

  virtual void SetUniform(uint64_t resource_id,
                          const std::string& name,
                          const base::Vector2f& val) = 0;
  virtual void SetUniform(uint64_t resource_id,
                          const std::string& name,
                          const base::Vector3f& val) = 0;
  virtual void SetUniform(uint64_t resource_id,
                          const std::string& name,
                          const base::Vector4f& val) = 0;
  virtual void SetUniform(uint64_t resource_id,
                          const std::string& name,
                          const base::Matrix4f& val) = 0;
  virtual void SetUniform(uint64_t resource_id,
                          const std::string& name,
                          float val) = 0;
  virtual void SetUniform(uint64_t resource_id,
                          const std::string& name,
                          int val) = 0;
  virtual void UploadUniforms(uint64_t resource_id) = 0;

  virtual void PrepareForDrawing() = 0;
  virtual void Present() = 0;

  bool SupportsETC1() const { return texture_compression_.etc1; }
  bool SupportsDXT1() const {
    return texture_compression_.dxt1 || texture_compression_.s3tc;
  }
  bool SupportsDXT5() const { return texture_compression_.s3tc; }
  bool SupportsATC() const { return texture_compression_.atc; }

  int screen_width() const { return screen_width_; }
  int screen_height() const { return screen_height_; }

  virtual size_t GetAndResetFPS() = 0;

  virtual const char* GetDebugName() = 0;

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

  int screen_width_ = 0;
  int screen_height_ = 0;

  base::Closure context_lost_cb_;

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;
};

}  // namespace eng

#endif  // ENGINE_RENDERER_RENDERER_H
