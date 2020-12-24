#ifndef VULKAN_CONTEXT_H
#define VULKAN_CONTEXT_H

#include <memory>
#include <unordered_map>
#include <vector>

#if defined(__ANDROID__)
#include "../../../third_party/android/vulkan_wrapper.h"
#else
#include "../../../third_party/vulkan/vulkan.h"
#endif

#if defined(__ANDROID__)
struct ANativeWindow;
#endif

namespace eng {

// Adapted from godot engin.
// https://github.com/godotengine/godot
class VulkanContext {
 public:
  VulkanContext();
  ~VulkanContext();

  bool Initialize();
  void Shutdown();

#if defined(__ANDROID__)
  bool CreateWindow(ANativeWindow* window, int width, int height);
#elif defined(__linux__)
  bool CreateWindow(Display* display, ::Window window, int width, int height);
#endif

  void ResizeWindow(int width, int height);
  void DestroyWindow();

  VkFramebuffer GetFramebuffer();

  VkRenderPass GetRenderPass() { return window_.render_pass; }

  VkExtent2D GetSwapchainExtent() { return window_.swapchain_extent; }

  void AppendCommandBuffer(const VkCommandBuffer& command_buffer);

  void Flush();

  bool PrepareBuffers();
  bool SwapBuffers();

  VkInstance GetInstance() { return instance_; }
  VkDevice GetDevice() { return device_; }
  VkPhysicalDevice GetPhysicalDevice() { return gpu_; }

  uint32_t GetSwapchainImageCount() const { return swapchain_image_count_; }

  uint32_t GetGraphicsQueue() const { return graphics_queue_family_index_; }

  VkFormat GetScreenFormat() const { return format_; }

  VkPhysicalDeviceLimits GetDeviceLimits() const { return gpu_props_.limits; }

  int GetWidth() { return window_.width; }
  int GetHeight() { return window_.height; }

  size_t GetAndResetFPS();

 private:
  enum { kMaxExtensions = 128, kMaxLayers = 64, kFrameLag = 2 };

  struct SwapchainImageResources {
    VkImage image;
    VkCommandBuffer graphics_to_present_cmd;
    VkImageView view;
    VkFramebuffer frame_buffer;
  };

  struct Window {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::unique_ptr<SwapchainImageResources[]> swapchain_image_resources;
    uint32_t current_buffer = 0;
    int width = 0;
    int height = 0;
    VkCommandPool present_cmd_pool = VK_NULL_HANDLE;
    VkRenderPass render_pass = VK_NULL_HANDLE;
    VkExtent2D swapchain_extent = {0, 0};
  };

  VkInstance instance_ = VK_NULL_HANDLE;
  VkPhysicalDevice gpu_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;

  VkPhysicalDeviceProperties gpu_props_;
  uint32_t queue_family_count_ = 0;
  std::unique_ptr<VkQueueFamilyProperties[]> queue_props_ = nullptr;

  bool buffers_prepared_ = false;

  bool queues_initialized_ = false;
  bool separate_present_queue_ = false;

  uint32_t graphics_queue_family_index_ = 0;
  uint32_t present_queue_family_index_ = 0;
  VkQueue graphics_queue_ = VK_NULL_HANDLE;
  VkQueue present_queue_ = VK_NULL_HANDLE;

  VkColorSpaceKHR color_space_;
  VkFormat format_;

  uint32_t frame_index_ = 0;

  VkSemaphore image_acquired_semaphores_[kFrameLag];
  VkSemaphore draw_complete_semaphores_[kFrameLag];
  VkSemaphore image_ownership_semaphores_[kFrameLag];
  VkFence fences_[kFrameLag];

  VkPhysicalDeviceMemoryProperties memory_properties_;
  VkPhysicalDeviceFeatures physical_device_features_;

  uint32_t swapchain_image_count_ = 0;

  std::vector<VkCommandBuffer> command_buffers_;

  Window window_;

  size_t fps_ = 0;

  // Extensions.
  uint32_t enabled_extension_count_ = 0;
  const char* extension_names_[kMaxExtensions];

  uint32_t enabled_layer_count_ = 0;
  const char* enabled_layers_[kMaxLayers];

  bool use_validation_layers_ = false;

  PFN_vkCreateDebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT;
  PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessengerEXT;

  PFN_vkGetPhysicalDeviceSurfaceSupportKHR GetPhysicalDeviceSurfaceSupportKHR;
  PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR
      GetPhysicalDeviceSurfaceCapabilitiesKHR;
  PFN_vkGetPhysicalDeviceSurfaceFormatsKHR GetPhysicalDeviceSurfaceFormatsKHR;
  PFN_vkGetPhysicalDeviceSurfacePresentModesKHR
      GetPhysicalDeviceSurfacePresentModesKHR;
  PFN_vkCreateSwapchainKHR CreateSwapchainKHR;
  PFN_vkDestroySwapchainKHR DestroySwapchainKHR;
  PFN_vkGetSwapchainImagesKHR GetSwapchainImagesKHR;
  PFN_vkAcquireNextImageKHR AcquireNextImageKHR;
  PFN_vkQueuePresentKHR QueuePresentKHR;

  VkDebugUtilsMessengerEXT dbg_messenger_ = VK_NULL_HANDLE;

  bool CreateValidationLayers();
  bool InitializeExtensions();

  VkBool32 CheckLayers(uint32_t check_count,
                       const char** check_names,
                       uint32_t layer_count,
                       VkLayerProperties* layers);

  static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessengerCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
      VkDebugUtilsMessageTypeFlagsEXT message_type,
      const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
      void* user_data);

  bool CreatePhysicalDevice();

  bool InitializeQueues(VkSurfaceKHR surface);

  bool CreateDevice();

  bool CleanUpSwapChain(Window* window);

  bool UpdateSwapChain(Window* window);

  bool CreateSwapChain();
  bool CreateSemaphores();

  const char* GetPlatformSurfaceExtension() const;

  VulkanContext(const VulkanContext&) = delete;
  VulkanContext& operator=(const VulkanContext&) = delete;
};

}  // namespace eng

#endif  // VULKAN_DEVICE_H
