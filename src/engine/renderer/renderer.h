#ifndef RENDERER_H
#define RENDERER_H

#include <memory>
#include <string>

#if defined(__linux__) && !defined(__ANDROID__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#include "../../base/closure.h"
#include "../../base/vecmath.h"
#include "renderer_types.h"

#if defined(__ANDROID__)
struct ANativeWindow;
#endif

namespace eng {

class Image;
class ShaderSource;
class Mesh;

class Renderer {
 public:
  const unsigned kInvalidId = 0;

  Renderer() = default;
  virtual ~Renderer() = default;

  void SetContextLostCB(base::Closure cb) { context_lost_cb_ = std::move(cb); }

#if defined(__ANDROID__)
  virtual bool Initialize(ANativeWindow* window) = 0;
#elif defined(__linux__)
  virtual bool Initialize(Display* display, Window window) = 0;
#endif

  virtual void Shutdown() = 0;

  virtual void CreateGeometry(std::shared_ptr<void> impl_data,
                              std::unique_ptr<Mesh> mesh) = 0;
  virtual void DestroyGeometry(std::shared_ptr<void> impl_data) = 0;
  virtual void Draw(std::shared_ptr<void> impl_data) = 0;

  virtual void UpdateTexture(std::shared_ptr<void> impl_data,
                             std::unique_ptr<Image> image) = 0;
  virtual void DestroyTexture(std::shared_ptr<void> impl_data) = 0;
  virtual void ActivateTexture(std::shared_ptr<void> impl_data) = 0;

  virtual void CreateShader(std::shared_ptr<void> impl_data,
                            std::unique_ptr<ShaderSource> source,
                            const VertexDescripton& vertex_description,
                            Primitive primitive,
                            bool enable_depth_test) = 0;
  virtual void DestroyShader(std::shared_ptr<void> impl_data) = 0;
  virtual void ActivateShader(std::shared_ptr<void> impl_data) = 0;

  virtual void SetUniform(std::shared_ptr<void> impl_data,
                          const std::string& name,
                          const base::Vector2f& val) = 0;
  virtual void SetUniform(std::shared_ptr<void> impl_data,
                          const std::string& name,
                          const base::Vector3f& val) = 0;
  virtual void SetUniform(std::shared_ptr<void> impl_data,
                          const std::string& name,
                          const base::Vector4f& val) = 0;
  virtual void SetUniform(std::shared_ptr<void> impl_data,
                          const std::string& name,
                          const base::Matrix4f& val) = 0;
  virtual void SetUniform(std::shared_ptr<void> impl_data,
                          const std::string& name,
                          float val) = 0;
  virtual void SetUniform(std::shared_ptr<void> impl_data,
                          const std::string& name,
                          int val) = 0;
  virtual void UploadUniforms(std::shared_ptr<void> impl_data) = 0;

  virtual void PrepareForDrawing() = 0;
  virtual void Present() = 0;

  virtual std::unique_ptr<RenderResource> CreateResource(
      RenderResourceFactoryBase& factory) = 0;
  virtual void ReleaseResource(unsigned resource_id) = 0;

  bool SupportsETC1() const { return texture_compression_.etc1; }
  bool SupportsDXT1() const {
    return texture_compression_.dxt1 || texture_compression_.s3tc;
  }
  bool SupportsDXT5() const { return texture_compression_.s3tc; }
  bool SupportsATC() const { return texture_compression_.atc; }

  int screen_width() const { return screen_width_; }
  int screen_height() const { return screen_height_; }

  virtual size_t GetAndResetFPS() = 0;

#if defined(__linux__) && !defined(__ANDROID__)
  virtual XVisualInfo* GetXVisualInfo(Display* display) = 0;
#endif

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

#endif  // RENDERER_H
