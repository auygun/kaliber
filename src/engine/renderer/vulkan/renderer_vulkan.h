#ifndef ENGINE_RENDERER_VULKAN_RENDERER_VULKAN_H
#define ENGINE_RENDERER_VULKAN_RENDERER_VULKAN_H

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <list>
#include <map>
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

struct SpvReflectShaderModule;

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
  uint64_t CreateGeometry(VertexDescription vertex_description,
                          DataType index_description = kDataType_Invalid) final;
  void UpdateGeometry(uint64_t resource_id,
                      size_t num_vertices,
                      const void* vertices,
                      size_t num_indices,
                      const void* indices) final;
  void DestroyGeometry(uint64_t resource_id) final;
  void ActivateGeometry(uint64_t resource_id) final;
  void Draw(size_t num_indices = 0,
            size_t first_index = 0,
            size_t instance_count = 1,
            size_t first_instance = 0) final;

  uint64_t CreateTexture() final;
  void UpdateTexture(uint64_t resource_id, std::unique_ptr<Image> image) final;
  void UpdateTexture(uint64_t resource_id,
                     std::vector<std::unique_ptr<Image>> images) final;
  void UpdateTexture(uint64_t resource_id,
                     int width,
                     int height,
                     int num_mip_levels,
                     int mip_level,
                     ImageFormat format,
                     size_t data_size,
                     uint8_t* image_data) final;
  void DestroyTexture(uint64_t resource_id) final;

  uint64_t CreateShader(std::unique_ptr<ShaderSource> source,
                        const VertexDescription& vertex_description,
                        Primitive primitive,
                        bool enable_depth_test,
                        bool wireframe,
                        CullMode cull_mode,
                        bool premultiplied_alpha = false) final;
  void DestroyShader(uint64_t resource_id) final;
  void ActivateShader(uint64_t resource_id) final;

  void UpdatePushConstants(size_t size, const void* data) final;

  uint64_t CreateBuffer(uint64_t shader_id,
                        size_t set,
                        size_t binding,
                        uint32_t buffer_size) final;
  void UpdateBuffer(uint64_t resource_id, const void* data, size_t size) final;
  void DestroyBuffer(uint64_t resource_id) final;

  uint64_t CreateDescriptorSet(
      uint64_t shader_id,
      size_t set,
      const std::vector<std::vector<uint64_t>>& textures,
      const std::vector<uint64_t>& buffers) final;
  void ActivateDescriptorSet(uint64_t resource_id) final;
  void DestroyDescriptorSet(uint64_t resource_id) final;

  void PrepareForDrawing() final;
  void Present() final;

  uint64_t CreateRenderTarget(ImageFormat format,
                              int width,
                              int height,
                              bool depth) final;
  void ActivateRenderTarget(uint64_t render_target_id) final;
  void DestroyRenderTarget(uint64_t render_target_id) final;
  void EndRenderPassToDefault() final;
  uint64_t GetRenderTargetColorTexture(uint64_t render_target_id) final;
  void EndRenderPass() final;

  size_t GetAndResetFPS() final;

  const char* GetDebugName() final { return "Vulkan"; }

  RendererType GetRendererType() final { return RendererType::kVulkan; }

 private:
  // VkBuffer or VkImage with allocator.
  template <typename T>
  using Buffer = std::tuple<T, VmaAllocation>;

  enum DescriptorType {
    kDescriptorType_Uninitialized = -1,
    kSamplerWithTexture,
    kUniformBuffer,
    kStorageBuffer,
    kDescriptorType_Max
  };

  struct DescriptorPoolKey {
    uint32_t descriptor_count[kDescriptorType_Max] = {};

    bool operator<(const DescriptorPoolKey& other) const {
      return memcmp(descriptor_count, other.descriptor_count,
                    sizeof(descriptor_count)) < 0;
    }
  };

  // Descriptor pools with usage counts.
  using DescriptorPools = std::list<std::pair<VkDescriptorPool, uint32_t>>;

  // Containers to keep information of resources to be destroyed.
  using BufferDeathRow = std::vector<Buffer<VkBuffer>>;
  using FrameBufferDeathRow = std::vector<VkFramebuffer>;
  using ImageDeathRow = std::vector<std::tuple<Buffer<VkImage>, VkImageView>>;
  using DescriptorSetDeathRow =
      std::vector<std::tuple<VkDescriptorSet,
                             DescriptorPoolKey,
                             DescriptorPools::iterator>>;
  using PipelineDeathRow =
      std::vector<std::tuple<VkPipeline, VkPipelineLayout>>;

  struct DescriptorBindingInfo {
    std::string name;  // TODO: remove if not needed.
    VkDescriptorType descriptor_type = (VkDescriptorType)-1;
    VkShaderStageFlags stage_flags = 0;
    size_t length = 0;  // Size of arrays (in total elements), or UBOs (in
                        // bytes * total elements).
  };

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
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    std::vector<std::vector<DescriptorBindingInfo>> bindings_per_set;
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;

    // Stored for creating pipeline variants per render pass.
    std::string name;
    VertexDescription vertex_description;
    Primitive primitive = kPrimitive_Triangles;
    bool enable_depth_test = false;
    bool wireframe = false;
    CullMode cull_mode = CullMode::kNone;
    bool premultiplied_alpha = false;
    std::map<VkRenderPass, VkPipeline> pipeline_variants;
  };

  struct TextureVulkan {
    Buffer<VkImage> image;
    VkImageView view = VK_NULL_HANDLE;
    int width = 0;
    int height = 0;
    int num_mip_levels = 0;
  };

  struct BufferVulkan {
    Buffer<VkBuffer> buffer;
    size_t buffer_size = 0;
    VkDescriptorType descriptor_type = (VkDescriptorType)-1;
  };

  struct DescriptorSetVulkan {
    uint32_t set = 0;
    VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
    DescriptorPoolKey pool_key;
    DescriptorPools::iterator pools_it;
  };

  struct RenderTarget {
    uint64_t color_texture_id = 0;
    uint64_t depth_texture_id = 0;
    VkFramebuffer frame_buffer = VK_NULL_HANDLE;
    VkRenderPass render_pass = VK_NULL_HANDLE;
    VkImageLayout last_image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    uint32_t width = 0;
    uint32_t height = 0;
  };

  // Each frame contains 2 command buffers with separate synchronization scopes.
  // One for creating resources (recorded outside a render pass) and another for
  // drawing (recorded inside a render pass). Also contains list of resources to
  // be destroyed when the frame is cycled. There are 2 or 3 frames (double or
  // triple buffering) that are cycled constantly.
  struct Frame {
    VkCommandPool setup_command_pool = VK_NULL_HANDLE;
    VkCommandBuffer setup_command_buffer = VK_NULL_HANDLE;
    VkCommandPool draw_command_pool = VK_NULL_HANDLE;
    VkCommandBuffer draw_command_buffer = VK_NULL_HANDLE;

    BufferDeathRow buffers_to_destroy;
    FrameBufferDeathRow frame_buffers_to_destroy;
    ImageDeathRow images_to_destroy;
    DescriptorSetDeathRow descriptor_sets_to_destroy;
    PipelineDeathRow pipelines_to_destroy;
  };

  struct StagingBuffer {
    Buffer<VkBuffer> buffer{VK_NULL_HANDLE, nullptr};
    uint64_t frame_used = 0;
    uint32_t fill_amount = 0;
    VmaAllocationInfo alloc_info;
  };

  struct RenderPassKey {
    VkFormat color_format;
    VkAttachmentLoadOp load_op;
    VkImageLayout initial_layout;
    VkImageLayout final_layout;
    VkFormat depth_format;

    bool operator<(const RenderPassKey& other) const {
      return std::tie(color_format, load_op, initial_layout, final_layout,
                      depth_format) <
             std::tie(other.color_format, other.load_op, other.initial_layout,
                      other.final_layout, other.depth_format);
    }
  };

  std::map<RenderPassKey, VkRenderPass> render_pass_pool_;

  std::unordered_map<uint64_t, GeometryVulkan> geometries_;
  std::unordered_map<uint64_t, ShaderVulkan> shaders_;
  std::unordered_map<uint64_t, TextureVulkan> textures_;
  std::unordered_map<uint64_t, BufferVulkan> buffers_;
  std::unordered_map<uint64_t, DescriptorSetVulkan> descriptor_sets_;
  std::unordered_map<uint64_t, RenderTarget> render_targets_;
  uint64_t last_resource_id_ = 0;

  bool context_lost_ = false;

  VulkanContext context_;

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

  uint32_t active_geometry_vertex_count_ = 0;
  uint32_t active_geometry_index_count_ = 0;

  uint64_t active_render_target_id_ = 0;
  bool in_default_render_pass_ = false;

  std::map<DescriptorPoolKey, DescriptorPools> descriptor_pools_map_;

  VkSampler sampler_ = VK_NULL_HANDLE;

  std::thread setup_thread_;
  base::TaskRunner task_runner_;
  std::counting_semaphore<> semaphore_{0};
  std::atomic<bool> quit_{false};

  bool InitializeInternal();

  void BeginFrame();

  void FlushSetupBuffer();

  void FreePendingResources(int frame);

  VkRenderPass GetOrCreateRenderPass(VkFormat color_format,
                                     VkAttachmentLoadOp load_op,
                                     VkImageLayout initial_layout,
                                     VkImageLayout final_layout,
                                     VkFormat depth_format);

  void MemoryBarrier(VkPipelineStageFlags src_stage_mask,
                     VkPipelineStageFlags dst_stage_mask,
                     VkAccessFlags src_access,
                     VkAccessFlags dst_access);
  void FullBarrier();

  bool AllocateStagingBuffer(uint32_t amount,
                             uint32_t segment,
                             uint32_t alignment,
                             uint32_t& alloc_offset,
                             uint32_t& alloc_size);
  bool InsertStagingBuffer();

  bool AllocateBuffer(Buffer<VkBuffer>& buffer,
                      uint32_t size,
                      uint32_t usage,
                      VmaMemoryUsage mapping);
  void FreeBuffer(Buffer<VkBuffer> buffer);
  void CopyBuffer(VkBuffer buffer,
                  size_t offset,
                  const void* data,
                  size_t data_size);
  void BufferMemoryBarrier(VkBuffer buffer,
                           uint64_t from,
                           uint64_t size,
                           VkPipelineStageFlags src_stage_mask,
                           VkPipelineStageFlags dst_stage_mask,
                           VkAccessFlags src_access,
                           VkAccessFlags dst_access);

  void FreeFrameBuffer(VkFramebuffer frame_buffer);

  bool AllocateImage(Buffer<VkImage>& image,
                     VkImageView& view,
                     VkFormat format,
                     int width,
                     int height,
                     int mip_levels,
                     VkImageUsageFlags usage,
                     VmaMemoryUsage mapping,
                     VkMemoryPropertyFlags mapping_flags);
  void FreeImage(Buffer<VkImage> image, VkImageView image_view);
  void CopyImage(VkImage image,
                 VkFormat format,
                 const uint8_t* data,
                 int width,
                 int height,
                 int mip_level);
  void ImageMemoryBarrier(VkCommandBuffer command_buffer,
                          VkImage image,
                          VkPipelineStageFlags src_stage_mask,
                          VkPipelineStageFlags dst_stage_mask,
                          VkAccessFlags src_access,
                          VkAccessFlags dst_access,
                          VkImageLayout old_layout,
                          VkImageLayout new_layout);

  bool ParseDescriptorBindings(
      std::vector<std::vector<DescriptorBindingInfo>>& bindings_per_set,
      const SpvReflectShaderModule* module,
      VkShaderStageFlagBits shader_stage_flag);
  bool CreatePipelineLayout(ShaderVulkan& shader,
                            const std::vector<uint8_t>& spirv_vertex,
                            const std::vector<uint8_t>& spirv_fragment);
  VkPipeline CreatePipeline(ShaderVulkan& shader, VkRenderPass render_pass);
  VkPipeline GetPipelineForCurrentRenderPass(ShaderVulkan& shader);

  void DrawListBegin();
  void DrawListEnd();

  void SwapBuffers();

  void SetupThreadMain();

  bool IsFormatSupported(VkFormat format);

  void DestroyAllResources();

  bool GetOrCreateDescriptorPool(DescriptorPoolKey key,
                                 DescriptorPools::iterator& pools_it);
  void UnreferenceDescriptorPool(DescriptorPoolKey key,
                                 DescriptorPools::iterator pools_it);
};

}  // namespace eng

#endif  // ENGINE_RENDERER_VULKAN_RENDERER_VULKAN_H
