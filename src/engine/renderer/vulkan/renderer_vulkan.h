#ifndef ENGINE_RENDERER_VULKAN_RENDERER_VULKAN_H
#define ENGINE_RENDERER_VULKAN_RENDERER_VULKAN_H

#include <atomic>
#include <memory>
#include <semaphore>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "engine/renderer/vulkan/vulkan_context.h"

#include "base/task_runner.h"
#include "engine/renderer/renderer.h"
#include "third_party/vma/vk_mem_alloc.h"

namespace eng {

class RendererVulkan final : public Renderer {
 public:
  RendererVulkan(base::Closure context_lost_cb);
  ~RendererVulkan() final;

  bool Initialize(Platform* platform) final;
  void Shutdown() final;

  bool IsInitialzed() const final { return device_ != VK_NULL_HANDLE; }

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
  void UpdateTexture(uint64_t resource_id,
                     int width,
                     int height,
                     ImageFormat format,
                     size_t data_size,
                     uint8_t* image_data) final;
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
  void UploadUniforms(uint64_t resource_id) final;

  void PrepareForDrawing() final;
  void Present() final;

  size_t GetAndResetFPS() final;

  const char* GetDebugName() final { return "Vulkan"; }

  RendererType GetRendererType() final { return RendererType::kVulkan; }

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

  std::unordered_map<std::string, std::array<std::vector<uint8_t>, 2>>
      spirv_cache_;

  struct GeometryVulkan {
    Buffer<VkBuffer> buffer;
    size_t buffer_size = 0;
    uint32_t num_vertices = 0;
    uint32_t num_indices = 0;
    size_t vertex_size = 0;
    uint64_t index_data_offset = 0;
    uint64_t index_type_size = 0;
    VkIndexType index_type = VK_INDEX_TYPE_NONE_KHR;
  };

  struct ShaderVulkan {
    std::vector<std::tuple<size_t,  // Variable name hash
                           size_t,  // Variable size
                           size_t   // Push constant offset
                           >>
        variables;
    std::unique_ptr<char[]> push_constants;
    size_t push_constants_size = 0;
    std::vector<std::string> sampler_uniform_names;
    size_t desc_set_count = 0;
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
    VkCommandPool setup_command_pool = VK_NULL_HANDLE;
    VkCommandBuffer setup_command_buffer = VK_NULL_HANDLE;
    VkCommandPool draw_command_pool = VK_NULL_HANDLE;
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

  std::unordered_map<uint64_t, GeometryVulkan> geometries_;
  std::unordered_map<uint64_t, ShaderVulkan> shaders_;
  std::unordered_map<uint64_t, TextureVulkan> textures_;
  uint64_t last_resource_id_ = 0;

  bool context_lost_ = false;

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

  uint64_t active_shader_id_ = 0;

  std::vector<std::unique_ptr<DescPool>> desc_pools_;
  VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> active_descriptor_sets_;

  VkSampler sampler_ = VK_NULL_HANDLE;

  std::thread setup_thread_;
  base::TaskRunner task_runner_;
  std::binary_semaphore semaphore_{0};
  std::atomic<bool> quit_{false};

  bool InitializeInternal();

  void BeginFrame();

  void FlushSetupBuffer();

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
  void UpdateBuffer(VkBuffer buffer,
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

  bool AllocateImage(Buffer<VkImage>& image,
                     VkImageView& view,
                     DescSet& desc_set,
                     VkFormat format,
                     int width,
                     int height,
                     VkImageUsageFlags usage,
                     VmaMemoryUsage mapping);
  void FreeImage(Buffer<VkImage> image,
                 VkImageView image_view,
                 DescSet desc_set);
  void UpdateImage(VkImage image,
                   VkFormat format,
                   const uint8_t* data,
                   int width,
                   int height);
  void ImageMemoryBarrier(VkImage image,
                          VkPipelineStageFlags src_stage_mask,
                          VkPipelineStageFlags dst_stage_mask,
                          VkAccessFlags src_access,
                          VkAccessFlags dst_sccess,
                          VkImageLayout old_layout,
                          VkImageLayout new_layout);

  bool CreatePipelineLayout(ShaderVulkan& shader,
                            const std::vector<uint8_t>& spirv_vertex,
                            const std::vector<uint8_t>& spirv_fragment);

  void DrawListBegin();
  void DrawListEnd();

  void SwapBuffers();

  void SetupThreadMain();

  template <typename T>
  bool SetUniformInternal(ShaderVulkan& shader, const std::string& name, T val);

  bool IsFormatSupported(VkFormat format);

  void DestroyAllResources();
};

}  // namespace eng

#endif  // ENGINE_RENDERER_VULKAN_RENDERER_VULKAN_H
