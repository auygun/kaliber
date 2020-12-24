#ifndef RENDERER_VULKAN_H
#define RENDERER_VULKAN_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "vulkan_context.h"

#include "../../../third_party/vma/vk_mem_alloc.h"
#include "../render_resource.h"
#include "../renderer.h"

namespace eng {

class Image;

class RendererVulkan : public Renderer {
 public:
  RendererVulkan();
  ~RendererVulkan() override;

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
                    Primitive primitive) override;
  void DestroyShader(std::shared_ptr<void> impl_data) override;
  void ActivateShader(std::shared_ptr<void> impl_data) override;

  void SetUniform(std::shared_ptr<void> impl_data,
                  const std::string& name,
                  const base::Vector2& val) override;
  void SetUniform(std::shared_ptr<void> impl_data,
                  const std::string& name,
                  const base::Vector3& val) override;
  void SetUniform(std::shared_ptr<void> impl_data,
                  const std::string& name,
                  const base::Vector4& val) override;
  void SetUniform(std::shared_ptr<void> impl_data,
                  const std::string& name,
                  const base::Matrix4x4& val) override;
  void SetUniform(std::shared_ptr<void> impl_data,
                  const std::string& name,
                  float val) override;
  void SetUniform(std::shared_ptr<void> impl_data,
                  const std::string& name,
                  int val) override;
  void UploadUniforms(std::shared_ptr<void> impl_data) override;

  void PrepareForDrawing() override;
  void Present() override;

  std::unique_ptr<RenderResource> CreateResource(
      RenderResourceFactoryBase& factory) override;
  void ReleaseResource(unsigned resource_id) override;

  size_t GetAndResetFPS() override;

#if defined(__linux__) && !defined(__ANDROID__)
  XVisualInfo* GetXVisualInfo(Display* display) override;
#endif

 private:
  // VkBuffer or VkImage with allocator.
  template <typename T>
  using Buffer = std::tuple<T, VmaAllocation>;

  // VkDescriptorPool with usage count.
  using DescPool = std::tuple<VkDescriptorPool, size_t>;

  // VkDescriptorSet with the pool which it was allocated from.
  using DescSet = std::tuple<VkDescriptorSet, DescPool*>;

  // Containers to keep information of resources to be destroyed.
  using BufferDeathRow = std::vector<Buffer<VkBuffer>>;
  using ImageDeathRow = std::vector<std::tuple<Buffer<VkImage>, VkImageView>>;
  using DescSetDeathRow = std::vector<DescSet>;
  using PipelineDeathRow =
      std::vector<std::tuple<VkPipeline, VkPipelineLayout>>;

  struct GeometryVulkan {
    Buffer<VkBuffer> buffer;
    size_t num_vertices = 0;
  };

  struct ShaderVulkan {
    std::unordered_map<std::string, std::array<size_t, 2>> variables;
    std::unique_ptr<char[]> push_constants;
    size_t push_constants_size = 0;
    std::string sampler_uniform_name;
    bool use_desc_set = false;
    std::vector<uint8_t> spirv_vertex;
    std::vector<uint8_t> spirv_fragment;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
  };

  struct TextureVulkan {
    Buffer<VkImage> image;
    VkImageView view = VK_NULL_HANDLE;
    DescSet desc_set = {};
    int width = 0;
    int height = 0;
  };

  // Each frame contains 2 command buffers with separate synchronization scopes.
  // One for creating resources (recorded outside a render pass) and another for
  // drawing (recorded inside a render pass). Also contains list of resources to
  // be destroyed when the frame is cycled. There are 2 or 3 frames (double or
  // tripple buffering) that are cycled constantly.
  struct Frame {
    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkCommandBuffer setup_command_buffer = VK_NULL_HANDLE;
    VkCommandBuffer draw_command_buffer = VK_NULL_HANDLE;

    BufferDeathRow buffers_to_destroy;
    ImageDeathRow images_to_destroy;
    DescSetDeathRow desc_sets_to_destroy;
    PipelineDeathRow pipelines_to_destroy;
  };

  struct StagingBuffer {
    Buffer<VkBuffer> buffer{VK_NULL_HANDLE, nullptr};
    uint64_t frame_used = 0;
    uint32_t fill_amount = 0;
    VmaAllocationInfo alloc_info;
  };

  VulkanContext context_;

  VmaAllocator allocator_ = nullptr;

  VkDevice device_ = VK_NULL_HANDLE;
  size_t frames_drawn_ = 0;
  std::vector<Frame> frames_;
  int current_frame_ = 0;

  std::vector<StagingBuffer> staging_buffers_;
  int current_staging_buffer_ = 0;
  uint32_t staging_buffer_size_ = 256 * 1024;
  uint64_t max_staging_buffer_size_ = 16 * 1024 * 1024;
  bool staging_buffer_used_ = false;

  VkPipeline active_pipeline_ = VK_NULL_HANDLE;

  std::vector<std::unique_ptr<DescPool>> desc_pools_;
  VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
  VkDescriptorSet active_descriptor_set_ = VK_NULL_HANDLE;
  VkDescriptorSet penging_descriptor_set_ = VK_NULL_HANDLE;

  VkSampler sampler_ = VK_NULL_HANDLE;

  std::unordered_map<unsigned, RenderResource*> resources_;

#if defined(__ANDROID__)
  ANativeWindow* window_;
#elif defined(__linux__)
  Display* display_ = NULL;
  Window window_ = 0;
#endif

  bool InitializeInternal();

  void BeginFrame();

  void Flush();

  void FreePendingResources(int frame);

  void MemoryBarrier(VkPipelineStageFlags src_stage_mask,
                     VkPipelineStageFlags dst_stage_mask,
                     VkAccessFlags src_access,
                     VkAccessFlags dst_sccess);
  void FullBarrier();

  bool AllocateStagingBuffer(uint32_t amount,
                             uint32_t segment,
                             uint32_t& alloc_offset,
                             uint32_t& alloc_size);
  bool InsertStagingBuffer();

  DescPool* AllocateDescriptorPool();
  void FreeDescriptorPool(DescPool* desc_pool);

  bool AllocateBuffer(Buffer<VkBuffer>& buffer,
                      uint32_t size,
                      uint32_t usage,
                      VmaMemoryUsage mapping);
  void FreeBuffer(Buffer<VkBuffer> buffer);
  bool UpdateBuffer(VkBuffer buffer,
                    size_t offset,
                    const void* data,
                    size_t data_size);
  void BufferMemoryBarrier(VkBuffer buffer,
                           uint64_t from,
                           uint64_t size,
                           VkPipelineStageFlags src_stage_mask,
                           VkPipelineStageFlags dst_stage_mask,
                           VkAccessFlags src_access,
                           VkAccessFlags dst_sccess);

  bool CreateTexture(Buffer<VkImage>& image,
                     VkImageView& view,
                     DescSet& desc_set,
                     int width,
                     int height,
                     VkImageUsageFlags usage,
                     VmaMemoryUsage mapping);
  void FreeTexture(Buffer<VkImage> image,
                   VkImageView image_view,
                   DescSet desc_set);
  bool UpdateImage(VkImage image, const uint8_t* data, int width, int height);
  void ImageMemoryBarrier(VkImage& image,
                          VkPipelineStageFlags src_stage_mask,
                          VkPipelineStageFlags dst_stage_mask,
                          VkAccessFlags src_access,
                          VkAccessFlags dst_sccess,
                          VkImageLayout old_layout,
                          VkImageLayout new_layout);

  bool CreatePipelineLayout(ShaderVulkan* shader);

  void DrawListBegin();
  void DrawListEnd();

  void SwapBuffers();

  template <typename T>
  bool SetUniformInternal(ShaderVulkan* shader, const std::string& name, T val);

  void ContextLost();

  void InvalidateAllResources();
};

}  // namespace eng

#endif  // RENDERER_VULKAN_H
