#include "engine/renderer/vulkan/vulkan_context.h"

#include <string.h>
#include <array>
#include <limits>
#include <string>

#include "base/log.h"
#include "third_party/vulkan/vk_enum_string_helper.h"

#define GET_PROC_ADDR(func, obj, entrypoint)                      \
  {                                                               \
    entrypoint = (PFN_vk##entrypoint)func(obj, "vk" #entrypoint); \
    if (entrypoint == nullptr) {                                  \
      DLOG << #func << " failed to find vk";                      \
      return false;                                               \
    }                                                             \
  }

namespace eng {

VulkanContext::VulkanContext() {
#if defined(_DEBUG) && !defined(__ANDROID__)
  use_validation_layers_ = true;
#endif
}

VulkanContext::~VulkanContext() {
  if (instance_ != VK_NULL_HANDLE) {
    if (use_validation_layers_)
      DestroyDebugUtilsMessengerEXT(instance_, dbg_messenger_, nullptr);
    vkDestroyInstance(instance_, nullptr);
    instance_ = VK_NULL_HANDLE;
  }
}

bool VulkanContext::Initialize() {
  if (volkInitialize() != VK_SUCCESS) {
    return false;
  }

  if (!CreatePhysicalDevice())
    return false;

  return true;
}

void VulkanContext::Shutdown() {
  if (device_ != VK_NULL_HANDLE) {
    for (int i = 0; i < kFrameLag; i++) {
      vkDestroyFence(device_, fences_[i], nullptr);
      vkDestroySemaphore(device_, image_acquired_semaphores_[i], nullptr);
      vkDestroySemaphore(device_, draw_complete_semaphores_[i], nullptr);
      if (separate_present_queue_) {
        vkDestroySemaphore(device_, image_ownership_semaphores_[i], nullptr);
      }
    }
    vkDestroyDevice(device_, nullptr);
    device_ = VK_NULL_HANDLE;
  }
  buffers_prepared_ = false;
  queues_initialized_ = false;
  separate_present_queue_ = false;
  swapchain_image_count_ = 0;
  command_buffers_.clear();
  window_ = {};
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::DebugMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {
  // This error needs to be ignored because the AMD allocator will mix up memory
  // types on IGP processors.
  if (strstr(callback_data->pMessage, "Mapping an image with layout") !=
          nullptr &&
      strstr(callback_data->pMessage,
             "can result in undefined behavior if this memory is used by the "
             "device") != nullptr) {
    return VK_FALSE;
  }
  // This needs to be ignored because Validator is wrong here.
  if (strstr(callback_data->pMessage,
             "SPIR-V module not valid: Pointer operand") != nullptr &&
      strstr(callback_data->pMessage, "must be a memory object") != nullptr) {
    return VK_FALSE;
  }
  // Workaround for Vulkan-Loader usability bug:
  // https://github.com/KhronosGroup/Vulkan-Loader/issues/262.
  if (strstr(callback_data->pMessage, "wrong ELF class: ELFCLASS32") !=
      nullptr) {
    return VK_FALSE;
  }
  if (callback_data->pMessageIdName &&
      strstr(callback_data->pMessageIdName,
             "UNASSIGNED-CoreValidation-DrawState-ClearCmdBeforeDraw") !=
          nullptr) {
    return VK_FALSE;
  }

  std::string type_string;
  switch (message_type) {
    case (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT):
      type_string = "GENERAL";
      break;
    case (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT):
      type_string = "VALIDATION";
      break;
    case (VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT):
      type_string = "PERFORMANCE";
      break;
    case (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT &
          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT):
      type_string = "VALIDATION|PERFORMANCE";
      break;
  }

  std::string objects_string;
  if (callback_data->objectCount > 0) {
    objects_string =
        "\n\tObjects - " + std::to_string(callback_data->objectCount);
    for (uint32_t object = 0; object < callback_data->objectCount; ++object) {
      objects_string +=
          "\n\t\tObject[" + std::to_string(object) + "]" + " - " +
          string_VkObjectType(callback_data->pObjects[object].objectType) +
          ", Handle " +
          std::to_string(callback_data->pObjects[object].objectHandle);
      if (nullptr != callback_data->pObjects[object].pObjectName &&
          strlen(callback_data->pObjects[object].pObjectName) > 0) {
        objects_string +=
            ", Name \"" +
            std::string(callback_data->pObjects[object].pObjectName) + "\"";
      }
    }
  }

  std::string labels_string;
  if (callback_data->cmdBufLabelCount > 0) {
    labels_string = "\n\tCommand Buffer Labels - " +
                    std::to_string(callback_data->cmdBufLabelCount);
    for (uint32_t cmd_buf_label = 0;
         cmd_buf_label < callback_data->cmdBufLabelCount; ++cmd_buf_label) {
      labels_string +=
          "\n\t\tLabel[" + std::to_string(cmd_buf_label) + "]" + " - " +
          callback_data->pCmdBufLabels[cmd_buf_label].pLabelName + "{ ";
      for (int color_idx = 0; color_idx < 4; ++color_idx) {
        labels_string += std::to_string(
            callback_data->pCmdBufLabels[cmd_buf_label].color[color_idx]);
        if (color_idx < 3) {
          labels_string += ", ";
        }
      }
      labels_string += " }";
    }
  }

  std::string error_message(
      type_string + " - Message Id Number: " +
      std::to_string(callback_data->messageIdNumber) +
      " | Message Id Name: " + callback_data->pMessageIdName + "\n\t" +
      callback_data->pMessage + objects_string + labels_string);

  switch (message_severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      LOG << error_message;
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      LOG << error_message;
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      LOG << error_message;
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      LOG << error_message;
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
      break;
  }

  return VK_FALSE;
}

VkBool32 VulkanContext::CheckLayers(uint32_t check_count,
                                    const char** check_names,
                                    uint32_t layer_count,
                                    VkLayerProperties* layers) {
  for (uint32_t i = 0; i < check_count; i++) {
    VkBool32 found = 0;
    for (uint32_t j = 0; j < layer_count; j++) {
      if (!strcmp(check_names[i], layers[j].layerName)) {
        found = 1;
        break;
      }
    }
    if (!found) {
      DLOG << "Can't find layer: " << check_names[i];
      return 0;
    }
  }
  return 1;
}

bool VulkanContext::CreateValidationLayers() {
  VkResult err;
  const char* instance_validation_layers_alt1[] = {
      "VK_LAYER_KHRONOS_validation"};
  const char* instance_validation_layers_alt2[] = {
      "VK_LAYER_LUNARG_standard_validation"};
  const char* instance_validation_layers_alt3[] = {
      "VK_LAYER_GOOGLE_threading", "VK_LAYER_LUNARG_parameter_validation",
      "VK_LAYER_LUNARG_object_tracker", "VK_LAYER_LUNARG_core_validation",
      "VK_LAYER_GOOGLE_unique_objects"};

  uint32_t instance_layer_count = 0;
  err = vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr);
  if (err) {
    DLOG << "vkEnumerateInstanceLayerProperties failed. Error: " << err;
    return false;
  }

  VkBool32 validation_found = 0;
  uint32_t validation_layer_count = 0;
  const char** instance_validation_layers = nullptr;
  if (instance_layer_count > 0) {
    auto instance_layers =
        std::make_unique<VkLayerProperties[]>(instance_layer_count);
    err = vkEnumerateInstanceLayerProperties(&instance_layer_count,
                                             instance_layers.get());
    if (err) {
      DLOG << "vkEnumerateInstanceLayerProperties failed. Error: " << err;
      return false;
    }

    validation_layer_count = std::size(instance_validation_layers_alt1);
    instance_validation_layers = instance_validation_layers_alt1;
    validation_found =
        CheckLayers(validation_layer_count, instance_validation_layers,
                    instance_layer_count, instance_layers.get());

    // use alternative (deprecated, removed in SDK 1.1.126.0) set of validation
    // layers.
    if (!validation_found) {
      validation_layer_count = std::size(instance_validation_layers_alt2);
      instance_validation_layers = instance_validation_layers_alt2;
      validation_found =
          CheckLayers(validation_layer_count, instance_validation_layers,
                      instance_layer_count, instance_layers.get());
    }

    // use alternative (deprecated, removed in SDK 1.1.121.1) set of validation
    // layers.
    if (!validation_found) {
      validation_layer_count = std::size(instance_validation_layers_alt3);
      instance_validation_layers = instance_validation_layers_alt3;
      validation_found =
          CheckLayers(validation_layer_count, instance_validation_layers,
                      instance_layer_count, instance_layers.get());
    }
  }

  if (validation_found) {
    enabled_layer_count_ = validation_layer_count;
    for (uint32_t i = 0; i < validation_layer_count; i++) {
      enabled_layers_[i] = instance_validation_layers[i];
    }
  } else {
    return false;
  }

  return true;
}

bool VulkanContext::InitializeExtensions() {
  VkResult err;
  uint32_t instance_extension_count = 0;

  enabled_extension_count_ = 0;
  enabled_layer_count_ = 0;
  VkBool32 surfaceExtFound = 0;
  VkBool32 platformSurfaceExtFound = 0;
  memset(extension_names_, 0, sizeof(extension_names_));

  err = vkEnumerateInstanceExtensionProperties(
      nullptr, &instance_extension_count, nullptr);
  if (err) {
    DLOG << "vkEnumerateInstanceExtensionProperties failed. Error: " << err;
    return false;
  }

  if (instance_extension_count > 0) {
    auto instance_extensions =
        std::make_unique<VkExtensionProperties[]>(instance_extension_count);
    err = vkEnumerateInstanceExtensionProperties(
        nullptr, &instance_extension_count, instance_extensions.get());
    if (err) {
      DLOG << "vkEnumerateInstanceExtensionProperties failed. Error: " << err;
      return false;
    }
    for (uint32_t i = 0; i < instance_extension_count; i++) {
      if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME,
                  instance_extensions[i].extensionName)) {
        surfaceExtFound = 1;
        extension_names_[enabled_extension_count_++] =
            VK_KHR_SURFACE_EXTENSION_NAME;
      }

      if (!strcmp(GetPlatformSurfaceExtension(),
                  instance_extensions[i].extensionName)) {
        platformSurfaceExtFound = 1;
        extension_names_[enabled_extension_count_++] =
            GetPlatformSurfaceExtension();
      }
      if (!strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
                  instance_extensions[i].extensionName)) {
        if (use_validation_layers_) {
          extension_names_[enabled_extension_count_++] =
              VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
        }
      }
      if (!strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
                  instance_extensions[i].extensionName)) {
        if (use_validation_layers_) {
          extension_names_[enabled_extension_count_++] =
              VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        }
      }
      if (enabled_extension_count_ >= kMaxExtensions) {
        DLOG << "Enabled extension count reaches kMaxExtensions";
        return false;
      }
    }
  }

  if (!surfaceExtFound) {
    DLOG << "No surface extension found.";
    return false;
  }
  if (!platformSurfaceExtFound) {
    DLOG << "No platform surface extension found.";
    return false;
  }

  return true;
}

bool VulkanContext::CreatePhysicalDevice() {
  if (use_validation_layers_) {
    CreateValidationLayers();
  }

  if (!InitializeExtensions())
    return false;

  const VkApplicationInfo app = {
      /*sType*/ VK_STRUCTURE_TYPE_APPLICATION_INFO,
      /*pNext*/ nullptr,
      /*pApplicationName*/ "kaliber",
      /*applicationVersion*/ 0,
      /*pEngineName*/ "kaliber",
      /*engineVersion*/ 0,
      /*apiVersion*/ VK_API_VERSION_1_0,
  };
  VkInstanceCreateInfo inst_info = {
      /*sType*/ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      /*pNext*/ nullptr,
      /*flags*/ 0,
      /*pApplicationInfo*/ &app,
      /*enabledLayerCount*/ enabled_layer_count_,
      /*ppEnabledLayerNames*/ (const char* const*)enabled_layers_,
      /*enabledExtensionCount*/ enabled_extension_count_,
      /*ppEnabledExtensionNames*/ (const char* const*)extension_names_,
  };

  // This is info for a temp callback to use during CreateInstance. After the
  // instance is created, we use the instance-based function to register the
  // final callback.
  VkDebugUtilsMessengerCreateInfoEXT dbg_messenger_info;
  if (use_validation_layers_) {
    dbg_messenger_info.sType =
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbg_messenger_info.pNext = nullptr;
    dbg_messenger_info.flags = 0;
    dbg_messenger_info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dbg_messenger_info.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dbg_messenger_info.pfnUserCallback = DebugMessengerCallback;
    dbg_messenger_info.pUserData = this;
    inst_info.pNext = &dbg_messenger_info;
  }

  uint32_t gpu_count;

  if (instance_ == VK_NULL_HANDLE) {
    VkResult err = vkCreateInstance(&inst_info, nullptr, &instance_);
    if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
      DLOG
          << "Cannot find a compatible Vulkan installable client driver (ICD).";
      return false;
    }
    if (err == VK_ERROR_EXTENSION_NOT_PRESENT) {
      DLOG
          << "Cannot find a specified extension library. Make sure your layers "
             "path is set appropriately. ";
      return false;
    }
    if (err) {
      DLOG << "vkCreateInstance failed. Error: " << err;
      return false;
    }
  }

  volkLoadInstance(instance_);

  // Make initial call to query gpu_count.
  VkResult err = vkEnumeratePhysicalDevices(instance_, &gpu_count, nullptr);
  if (err) {
    DLOG << "vkEnumeratePhysicalDevices failed. Error: " << err;
    return false;
  }

  if (gpu_count == 0) {
    DLOG << "vkEnumeratePhysicalDevices reported zero accessible devices.";
    return false;
  }

  auto physical_devices = std::make_unique<VkPhysicalDevice[]>(gpu_count);
  err =
      vkEnumeratePhysicalDevices(instance_, &gpu_count, physical_devices.get());
  if (err) {
    DLOG << "vkEnumeratePhysicalDevices failed. Error: " << err;
    return false;
  }
  // Grab the first physical device for now.
  gpu_ = physical_devices[0];

  // Look for device extensions.
  uint32_t device_extension_count = 0;
  VkBool32 swapchain_ext_found = 0;
  enabled_extension_count_ = 0;
  memset(extension_names_, 0, sizeof(extension_names_));

  err = vkEnumerateDeviceExtensionProperties(gpu_, nullptr,
                                             &device_extension_count, nullptr);
  if (err) {
    DLOG << "vkEnumerateDeviceExtensionProperties failed. Error: " << err;
    return false;
  }

  if (device_extension_count > 0) {
    auto device_extensions =
        std::make_unique<VkExtensionProperties[]>(device_extension_count);
    err = vkEnumerateDeviceExtensionProperties(
        gpu_, nullptr, &device_extension_count, device_extensions.get());
    if (err) {
      DLOG << "vkEnumerateDeviceExtensionProperties failed. Error: " << err;
      return false;
    }

    for (uint32_t i = 0; i < device_extension_count; i++) {
      if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                  device_extensions[i].extensionName)) {
        swapchain_ext_found = 1;
        extension_names_[enabled_extension_count_++] =
            VK_KHR_SWAPCHAIN_EXTENSION_NAME;
      }
      if (enabled_extension_count_ >= kMaxExtensions) {
        DLOG << "Enabled extension count reaches kMaxExtensions";
        return false;
      }
    }

    // Enable VK_KHR_maintenance1 extension for old vulkan drivers.
    for (uint32_t i = 0; i < device_extension_count; i++) {
      if (!strcmp(VK_KHR_MAINTENANCE1_EXTENSION_NAME,
                  device_extensions[i].extensionName)) {
        extension_names_[enabled_extension_count_++] =
            VK_KHR_MAINTENANCE1_EXTENSION_NAME;
      }
      if (enabled_extension_count_ >= kMaxExtensions) {
        DLOG << "Enabled extension count reaches kMaxExtensions";
        return false;
      }
    }
  }

  if (!swapchain_ext_found) {
    DLOG << "vkEnumerateDeviceExtensionProperties failed to find "
            "the " VK_KHR_SWAPCHAIN_EXTENSION_NAME " extension.";
    return false;
  }

  if (use_validation_layers_) {
    CreateDebugUtilsMessengerEXT =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance_, "vkCreateDebugUtilsMessengerEXT");
    DestroyDebugUtilsMessengerEXT =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance_, "vkDestroyDebugUtilsMessengerEXT");
    if (nullptr == CreateDebugUtilsMessengerEXT ||
        nullptr == DestroyDebugUtilsMessengerEXT) {
      DLOG << "GetProcAddr: Failed to init VK_EXT_debug_utils";
      return false;
    }

    err = CreateDebugUtilsMessengerEXT(instance_, &dbg_messenger_info, nullptr,
                                       &dbg_messenger_);
    switch (err) {
      case VK_SUCCESS:
        break;
      case VK_ERROR_OUT_OF_HOST_MEMORY:
        DLOG << "CreateDebugUtilsMessengerEXT: out of host memory";
        return false;
      default:
        DLOG << "CreateDebugUtilsMessengerEXT: unknown failure";
        return false;
        break;
    }
  }
  vkGetPhysicalDeviceProperties(gpu_, &gpu_props_);

  LOG << "Vulkan:";
  LOG << "  Name: " << gpu_props_.deviceName;
  LOG << "  Tame: " << string_VkPhysicalDeviceType(gpu_props_.deviceType);
  LOG << "  Vendor ID: " << gpu_props_.vendorID;
  LOG << "  API version: " << VK_VERSION_MAJOR(gpu_props_.apiVersion) << "."
      << VK_VERSION_MINOR(gpu_props_.apiVersion) << "."
      << VK_VERSION_PATCH(gpu_props_.apiVersion);
  LOG << "  Driver version: " << VK_VERSION_MAJOR(gpu_props_.driverVersion)
      << "." << VK_VERSION_MINOR(gpu_props_.driverVersion) << "."
      << VK_VERSION_PATCH(gpu_props_.driverVersion);

  // Call with NULL data to get count,
  vkGetPhysicalDeviceQueueFamilyProperties(gpu_, &queue_family_count_, nullptr);
  if (queue_family_count_ == 0) {
    DLOG << "Failed to query queue family count.";
    return false;
  }

  queue_props_ =
      std::make_unique<VkQueueFamilyProperties[]>(queue_family_count_);
  vkGetPhysicalDeviceQueueFamilyProperties(gpu_, &queue_family_count_,
                                           queue_props_.get());

  // Query fine-grained feature support for this device.
  // If app has specific feature requirements it should check supported features
  // based on this query.
  vkGetPhysicalDeviceFeatures(gpu_, &physical_device_features_);

  GET_PROC_ADDR(vkGetInstanceProcAddr, instance_,
                GetPhysicalDeviceSurfaceSupportKHR);
  GET_PROC_ADDR(vkGetInstanceProcAddr, instance_,
                GetPhysicalDeviceSurfaceCapabilitiesKHR);
  GET_PROC_ADDR(vkGetInstanceProcAddr, instance_,
                GetPhysicalDeviceSurfaceFormatsKHR);
  GET_PROC_ADDR(vkGetInstanceProcAddr, instance_,
                GetPhysicalDeviceSurfacePresentModesKHR);
  GET_PROC_ADDR(vkGetInstanceProcAddr, instance_, GetSwapchainImagesKHR);

  return true;
}

bool VulkanContext::CreateDevice() {
  VkResult err;
  float queue_priorities[1] = {0.0};
  VkDeviceQueueCreateInfo queues[2];
  queues[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queues[0].pNext = nullptr;
  queues[0].queueFamilyIndex = graphics_queue_family_index_;
  queues[0].queueCount = 1;
  queues[0].pQueuePriorities = queue_priorities;
  queues[0].flags = 0;

  VkDeviceCreateInfo sdevice = {
      /*sType*/ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      /*pNext*/ nullptr,
      /*flags*/ 0,
      /*queueCreateInfoCount*/ 1,
      /*pQueueCreateInfos*/ queues,
      /*enabledLayerCount*/ 0,
      /*ppEnabledLayerNames*/ nullptr,
      /*enabledExtensionCount*/ enabled_extension_count_,
      /*ppEnabledExtensionNames*/ (const char* const*)extension_names_,
      /*pEnabledFeatures*/ &physical_device_features_,  // If specific features
                                                        // are required, pass
                                                        // them in here

  };
  if (separate_present_queue_) {
    queues[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queues[1].pNext = nullptr;
    queues[1].queueFamilyIndex = present_queue_family_index_;
    queues[1].queueCount = 1;
    queues[1].pQueuePriorities = queue_priorities;
    queues[1].flags = 0;
    sdevice.queueCreateInfoCount = 2;
  }
  err = vkCreateDevice(gpu_, &sdevice, nullptr, &device_);
  if (err) {
    DLOG << "vkCreateDevice failed. Error: " << err;
    return false;
  }
  return true;
}

bool VulkanContext::InitializeQueues(VkSurfaceKHR surface) {
  // Iterate over each queue to learn whether it supports presenting:
  auto supports_present = std::make_unique<VkBool32[]>(queue_family_count_);
  for (uint32_t i = 0; i < queue_family_count_; i++) {
    GetPhysicalDeviceSurfaceSupportKHR(gpu_, i, surface, &supports_present[i]);
  }

  // Search for a graphics and a present queue in the array of queue families,
  // try to find one that supports both.
  uint32_t graphics_queue_family_index = std::numeric_limits<uint32_t>::max();
  uint32_t present_queue_family_index = std::numeric_limits<uint32_t>::max();
  for (uint32_t i = 0; i < queue_family_count_; i++) {
    if ((queue_props_[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
      if (graphics_queue_family_index == std::numeric_limits<uint32_t>::max()) {
        graphics_queue_family_index = i;
      }

      if (supports_present[i] == VK_TRUE) {
        graphics_queue_family_index = i;
        present_queue_family_index = i;
        break;
      }
    }
  }

  if (present_queue_family_index == std::numeric_limits<uint32_t>::max()) {
    // If didn't find a queue that supports both graphics and present, then find
    // a separate present queue.
    for (uint32_t i = 0; i < queue_family_count_; ++i) {
      if (supports_present[i] == VK_TRUE) {
        present_queue_family_index = i;
        break;
      }
    }
  }

  // Generate error if could not find both a graphics and a present queue
  if (graphics_queue_family_index == std::numeric_limits<uint32_t>::max() ||
      present_queue_family_index == std::numeric_limits<uint32_t>::max()) {
    DLOG << "Could not find both graphics and present queues.";
    return false;
  }

  graphics_queue_family_index_ = graphics_queue_family_index;
  present_queue_family_index_ = present_queue_family_index;
  separate_present_queue_ =
      (graphics_queue_family_index_ != present_queue_family_index_);

  CreateDevice();

  PFN_vkGetDeviceProcAddr GetDeviceProcAddr = nullptr;
  GET_PROC_ADDR(vkGetInstanceProcAddr, instance_, GetDeviceProcAddr);

  GET_PROC_ADDR(GetDeviceProcAddr, device_, CreateSwapchainKHR);
  GET_PROC_ADDR(GetDeviceProcAddr, device_, DestroySwapchainKHR);
  GET_PROC_ADDR(GetDeviceProcAddr, device_, GetSwapchainImagesKHR);
  GET_PROC_ADDR(GetDeviceProcAddr, device_, AcquireNextImageKHR);
  GET_PROC_ADDR(GetDeviceProcAddr, device_, QueuePresentKHR);

  vkGetDeviceQueue(device_, graphics_queue_family_index_, 0, &graphics_queue_);

  if (!separate_present_queue_) {
    present_queue_ = graphics_queue_;
  } else {
    vkGetDeviceQueue(device_, present_queue_family_index_, 0, &present_queue_);
  }

  // Get the list of VkFormat's that are supported.
  uint32_t format_count;
  VkResult err =
      GetPhysicalDeviceSurfaceFormatsKHR(gpu_, surface, &format_count, nullptr);
  if (err) {
    DLOG << "GetPhysicalDeviceSurfaceFormatsKHR failed. Error: " << err;
    return false;
  }
  auto surf_formats = std::make_unique<VkSurfaceFormatKHR[]>(format_count);
  err = GetPhysicalDeviceSurfaceFormatsKHR(gpu_, surface, &format_count,
                                           surf_formats.get());
  if (err) {
    DLOG << "GetPhysicalDeviceSurfaceFormatsKHR failed. Error: " << err;
    return false;
  }

#if defined(__ANDROID__)
  VkFormat desired_format = VK_FORMAT_R8G8B8A8_UNORM;
#elif defined(__linux__)
  VkFormat desired_format = VK_FORMAT_B8G8R8A8_UNORM;
#endif

  // If the format list includes just one entry of VK_FORMAT_UNDEFINED, the
  // surface has no preferred format. Otherwise, at least one supported format
  // will be returned.
  if (true ||
      (format_count == 1 && surf_formats[0].format == VK_FORMAT_UNDEFINED)) {
    format_ = desired_format;
  } else {
    if (format_count < 1) {
      DLOG << "Format count less than 1.";
      return false;
    }
    format_ = surf_formats[0].format;
    for (unsigned i = 0; i < format_count; ++i) {
      if (surf_formats[i].format == desired_format) {
        format_ = desired_format;
        break;
      }
    }
  }
  color_space_ = surf_formats[0].colorSpace;

  if (!CreateSemaphores())
    return false;

  queues_initialized_ = true;
  return true;
}

bool VulkanContext::CreateSemaphores() {
  VkResult err;

  // Create semaphores to synchronize acquiring presentable buffers before
  // rendering and waiting for drawing to be complete before presenting.
  VkSemaphoreCreateInfo semaphoreCreateInfo = {
      /*sType*/ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      /*pNext*/ nullptr,
      /*flags*/ 0,
  };

  // Create fences that we can use to throttle if we get too far ahead of the
  // image presents.
  VkFenceCreateInfo fence_ci = {/*sType*/ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                /*pNext*/ nullptr,
                                /*flags*/ VK_FENCE_CREATE_SIGNALED_BIT};

  for (uint32_t i = 0; i < kFrameLag; i++) {
    err = vkCreateFence(device_, &fence_ci, nullptr, &fences_[i]);
    if (err) {
      DLOG << "vkCreateFence failed. Error: " << err;
      return false;
    }
    err = vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr,
                            &image_acquired_semaphores_[i]);
    if (err) {
      DLOG << "vkCreateSemaphore failed. Error: " << err;
      return false;
    }
    err = vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr,
                            &draw_complete_semaphores_[i]);
    if (err) {
      DLOG << "vkCreateSemaphore failed. Error: " << err;
      return false;
    }
    if (separate_present_queue_) {
      err = vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr,
                              &image_ownership_semaphores_[i]);
      if (err) {
        DLOG << "vkCreateSemaphore failed. Error: " << err;
        return false;
      }
    }
  }
  frame_index_ = 0;

  // Get Memory information and properties.
  vkGetPhysicalDeviceMemoryProperties(gpu_, &memory_properties_);

  return true;
}

void VulkanContext::ResizeWindow(int width, int height) {
  window_.width = width;
  window_.height = height;
  UpdateSwapChain(&window_);
}

void VulkanContext::DestroyWindow() {
  CleanUpSwapChain(&window_);
  vkDestroySurfaceKHR(instance_, window_.surface, nullptr);
}

VkFramebuffer VulkanContext::GetFramebuffer() {
  return buffers_prepared_
             ? window_.swapchain_image_resources[window_.current_buffer]
                   .frame_buffer
             : VK_NULL_HANDLE;
}

bool VulkanContext::CleanUpSwapChain(Window* window) {
  if (!window->swapchain)
    return true;

  vkDeviceWaitIdle(device_);

  vkDestroyImageView(device_, window->depth_view, nullptr);
  vkDestroyImage(device_, window->depth_image, nullptr);
  vkFreeMemory(device_, window->depth_image_memory, nullptr);
  window->depth_view = VK_NULL_HANDLE;
  window->depth_image = VK_NULL_HANDLE;
  window->depth_image_memory = VK_NULL_HANDLE;

  DestroySwapchainKHR(device_, window->swapchain, nullptr);
  window->swapchain = VK_NULL_HANDLE;
  vkDestroyRenderPass(device_, window->render_pass, nullptr);
  if (window->swapchain_image_resources) {
    for (uint32_t i = 0; i < swapchain_image_count_; i++) {
      vkDestroyImageView(device_, window->swapchain_image_resources[i].view,
                         nullptr);
      vkDestroyFramebuffer(
          device_, window->swapchain_image_resources[i].frame_buffer, nullptr);
    }

    window->swapchain_image_resources.reset();
  }

  if (separate_present_queue_)
    vkDestroyCommandPool(device_, window->present_cmd_pool, nullptr);

  return true;
}

bool VulkanContext::UpdateSwapChain(Window* window) {
  VkResult err;

  if (window->swapchain)
    CleanUpSwapChain(window);

  // Check the surface capabilities and formats.
  VkSurfaceCapabilitiesKHR surf_capabilities;
  err = GetPhysicalDeviceSurfaceCapabilitiesKHR(gpu_, window->surface,
                                                &surf_capabilities);
  if (err) {
    DLOG << "GetPhysicalDeviceSurfaceCapabilitiesKHR failed. Error: " << err;
    return false;
  }

  uint32_t present_mode_count;
  err = GetPhysicalDeviceSurfacePresentModesKHR(gpu_, window->surface,
                                                &present_mode_count, nullptr);
  if (err) {
    DLOG << "GetPhysicalDeviceSurfacePresentModesKHR failed. Error: " << err;
    return false;
  }

  auto present_modes = std::make_unique<VkPresentModeKHR[]>(present_mode_count);

  err = GetPhysicalDeviceSurfacePresentModesKHR(
      gpu_, window->surface, &present_mode_count, present_modes.get());
  if (err) {
    DLOG << "GetPhysicalDeviceSurfacePresentModesKHR failed. Error: " << err;
    return false;
  }

  // width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
  if (surf_capabilities.currentExtent.width == 0xFFFFFFFF) {
    // If the surface size is undefined, the size is set to the size of the
    // images requested, which must fit within the minimum and maximum values.
    window->swapchain_extent.width = window->width;
    window->swapchain_extent.height = window->height;

    if (window->swapchain_extent.width <
        surf_capabilities.minImageExtent.width) {
      window->swapchain_extent.width = surf_capabilities.minImageExtent.width;
    } else if (window->swapchain_extent.width >
               surf_capabilities.maxImageExtent.width) {
      window->swapchain_extent.width = surf_capabilities.maxImageExtent.width;
    }

    if (window->swapchain_extent.height <
        surf_capabilities.minImageExtent.height) {
      window->swapchain_extent.height = surf_capabilities.minImageExtent.height;
    } else if (window->swapchain_extent.height >
               surf_capabilities.maxImageExtent.height) {
      window->swapchain_extent.height = surf_capabilities.maxImageExtent.height;
    }
  } else {
    // If the surface size is defined, the swap chain size must match
    window->swapchain_extent = surf_capabilities.currentExtent;
    window->width = surf_capabilities.currentExtent.width;
    window->height = surf_capabilities.currentExtent.height;
  }

  if (window->width == 0 || window->height == 0) {
    // likely window minimized, no swapchain created
    return true;
  }

  // The application will render an image, then pass it to the presentation
  // engine via vkQueuePresentKHR. The presentation engine will display the
  // image for the next VSync cycle, and then it will make it available to the
  // application again. The only present modes which support VSync are:
  //
  // VK_PRESENT_MODE_FIFO_KHR: At each VSync signal, the image in front of the
  // queue displays on screen and is then released. The application will acquire
  // one of the available ones, draw to it and then hand it over to the
  // presentation engine, which will push it to the back of the queue. If
  // rendering is fast the queue can become full. The CPU and the GPU will idle
  // until an image is available again. This behavior works well on mobile
  // because it limits overheating and saves battery life.
  //
  // VK_PRESENT_MODE_MAILBOX_KHR: The application can acquire a new image
  // straight away, render to it, and hand it over to the presentation engine.
  // If an image is queued for presentation, it will be discarded. Being able to
  // keep submitting new frames lets the application ensure it has the latest
  // user input, so input latency can be lower versus FIFO. If the application
  // doesn't throttle CPU and GPU, one of them may be fully utilized, resulting
  // in higher power consumption.
  VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
  VkPresentModeKHR fallback_present_mode = VK_PRESENT_MODE_FIFO_KHR;
  if (swapchain_present_mode != fallback_present_mode) {
    for (size_t i = 0; i < present_mode_count; ++i) {
      if (present_modes[i] == swapchain_present_mode) {
        // Supported.
        fallback_present_mode = swapchain_present_mode;
        break;
      }
    }
  }

  if (swapchain_present_mode != fallback_present_mode) {
    LOG << "Present mode " << swapchain_present_mode << " is not supported";
    swapchain_present_mode = fallback_present_mode;
  }

  // 2 for double buffering, 3 for triple buffering.
  // Double buffering works well if frames can be processed within the interval
  // between VSync signals, which is 16.6ms at a rate of 60 fps. The rendered
  // image is presented to the swapchain, and the previously presented one is
  // made available to the application again. If the GPU cannot process frames
  // fast enough, VSync will be missed and the application will have to wait for
  // another whole VSync cycle, which caps framerate at 30 fps. This may be ok,
  // but triple buffering can deliver higher framerate.
  uint32_t desired_num_of_swapchain_images = 3;
  if (desired_num_of_swapchain_images < surf_capabilities.minImageCount) {
    desired_num_of_swapchain_images = surf_capabilities.minImageCount;
  }
  // If maxImageCount is 0, we can ask for as many images as we want; otherwise
  // we're limited to maxImageCount.
  if ((surf_capabilities.maxImageCount > 0) &&
      (desired_num_of_swapchain_images > surf_capabilities.maxImageCount)) {
    // Application must settle for fewer images than desired.
    desired_num_of_swapchain_images = surf_capabilities.maxImageCount;
  }

  VkSurfaceTransformFlagsKHR pre_transform;
  if (surf_capabilities.supportedTransforms &
      VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
    pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  } else {
    pre_transform = surf_capabilities.currentTransform;
  }

  // Find a supported composite alpha mode. One of these is guaranteed to be
  // set.
  VkCompositeAlphaFlagBitsKHR composite_alpha =
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  VkCompositeAlphaFlagBitsKHR composite_alpha_flags[4] = {
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
      VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
      VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
  };
  for (uint32_t i = 0; i < std::size(composite_alpha_flags); i++) {
    if (surf_capabilities.supportedCompositeAlpha & composite_alpha_flags[i]) {
      composite_alpha = composite_alpha_flags[i];
      break;
    }
  }

  VkSwapchainCreateInfoKHR swapchain_ci = {
      /*sType*/ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      /*pNext*/ nullptr,
      /*flags*/ 0,
      /*surface*/ window->surface,
      /*minImageCount*/ desired_num_of_swapchain_images,
      /*imageFormat*/ format_,
      /*imageColorSpace*/ color_space_,
      /*imageExtent*/
      {
          /*width*/ window->swapchain_extent.width,
          /*height*/ window->swapchain_extent.height,
      },
      /*imageArrayLayers*/ 1,
      /*imageUsage*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      /*imageSharingMode*/ VK_SHARING_MODE_EXCLUSIVE,
      /*queueFamilyIndexCount*/ 0,
      /*pQueueFamilyIndices*/ nullptr,
      /*preTransform*/ (VkSurfaceTransformFlagBitsKHR)pre_transform,
      /*compositeAlpha*/ composite_alpha,
      /*presentMode*/ swapchain_present_mode,
      /*clipped*/ true,
      /*oldSwapchain*/ VK_NULL_HANDLE,
  };

  err = CreateSwapchainKHR(device_, &swapchain_ci, nullptr, &window->swapchain);
  if (err) {
    DLOG << "CreateSwapchainKHR failed. Error: " << err;
    return false;
  }

  uint32_t sp_image_count;
  err = GetSwapchainImagesKHR(device_, window->swapchain, &sp_image_count,
                              nullptr);
  if (err) {
    DLOG << "CreateSwapchainKHR failed. Error: " << err;
    return false;
  }

  if (swapchain_image_count_ == 0) {
    // Assign for the first time.
    swapchain_image_count_ = sp_image_count;
  } else if (swapchain_image_count_ != sp_image_count) {
    DLOG << "Swapchain image count mismatch";
    return false;
  }

  auto swapchain_images = std::make_unique<VkImage[]>(swapchain_image_count_);

  err = GetSwapchainImagesKHR(device_, window->swapchain,
                              &swapchain_image_count_, swapchain_images.get());
  if (err) {
    DLOG << "GetSwapchainImagesKHR failed. Error: " << err;
    return false;
  }

  window->swapchain_image_resources =
      std::make_unique<SwapchainImageResources[]>(swapchain_image_count_);

  for (uint32_t i = 0; i < swapchain_image_count_; i++) {
    VkImageViewCreateInfo color_image_view = {
        /*sType*/ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        /*pNext*/ nullptr,
        /*flags*/ 0,
        /*image*/ swapchain_images[i],
        /*viewType*/ VK_IMAGE_VIEW_TYPE_2D,
        /*format*/ format_,
        /*components*/
        {
            /*r*/ VK_COMPONENT_SWIZZLE_R,
            /*g*/ VK_COMPONENT_SWIZZLE_G,
            /*b*/ VK_COMPONENT_SWIZZLE_B,
            /*a*/ VK_COMPONENT_SWIZZLE_A,
        },
        /*subresourceRange*/
        {/*aspectMask*/ VK_IMAGE_ASPECT_COLOR_BIT,
         /*baseMipLevel*/ 0,
         /*levelCount*/ 1,
         /*baseArrayLayer*/ 0,
         /*layerCount*/ 1},
    };

    window->swapchain_image_resources[i].image = swapchain_images[i];

    color_image_view.image = window->swapchain_image_resources[i].image;

    err = vkCreateImageView(device_, &color_image_view, nullptr,
                            &window->swapchain_image_resources[i].view);
    if (err) {
      DLOG << "vkCreateImageView failed. Error: " << err;
      return false;
    }
  }

  // Framebuffer

  {
    const VkAttachmentDescription color_attachment = {
        /*flags*/ 0,
        /*format*/ format_,
        /*samples*/ VK_SAMPLE_COUNT_1_BIT,
        /*loadOp*/ VK_ATTACHMENT_LOAD_OP_CLEAR,
        /*storeOp*/ VK_ATTACHMENT_STORE_OP_STORE,
        /*stencilLoadOp*/ VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        /*stencilStoreOp*/ VK_ATTACHMENT_STORE_OP_DONT_CARE,
        /*initialLayout*/ VK_IMAGE_LAYOUT_UNDEFINED,
        /*finalLayout*/ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,

    };
    const VkAttachmentReference color_reference = {
        /*attachment*/ 0,
        /*layout*/ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    const VkAttachmentDescription depth_attachment = {
        /*flags*/ 0,
        /*format*/ VK_FORMAT_D24_UNORM_S8_UINT,
        /*samples*/ VK_SAMPLE_COUNT_1_BIT,
        /*loadOp*/ VK_ATTACHMENT_LOAD_OP_CLEAR,
        /*storeOp*/ VK_ATTACHMENT_STORE_OP_DONT_CARE,
        /*stencilLoadOp*/ VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        /*stencilStoreOp*/ VK_ATTACHMENT_STORE_OP_DONT_CARE,
        /*initialLayout*/ VK_IMAGE_LAYOUT_UNDEFINED,
        /*finalLayout*/ VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,

    };
    const VkAttachmentReference depth_reference = {
        /*attachment*/ 1,
        /*layout*/ VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    const VkSubpassDescription subpass = {
        /*flags*/ 0,
        /*pipelineBindPoint*/ VK_PIPELINE_BIND_POINT_GRAPHICS,
        /*inputAttachmentCount*/ 0,
        /*pInputAttachments*/ nullptr,
        /*colorAttachmentCount*/ 1,
        /*pColorAttachments*/ &color_reference,
        /*pResolveAttachments*/ nullptr,
        /*pDepthStencilAttachment*/ &depth_reference,
        /*preserveAttachmentCount*/ 0,
        /*pPreserveAttachments*/ nullptr,
    };

    VkSubpassDependency dependency = {
        /*srcSubpass*/ VK_SUBPASS_EXTERNAL,
        /*dstSubpass*/ 0,
        /*srcStageMask*/ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        /*dstStageMask*/ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        /*srcAccessMask*/ 0,
        /*dstAccessMask*/ VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        /*dependencyFlags*/ 0,
    };

    std::array<VkAttachmentDescription, 2> attachments = {color_attachment,
                                                          depth_attachment};
    const VkRenderPassCreateInfo rp_info = {
        /*sTyp*/ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        /*pNext*/ nullptr,
        /*flags*/ 0,
        /*attachmentCount*/ attachments.size(),
        /*pAttachments*/ attachments.data(),
        /*subpassCount*/ 1,
        /*pSubpasses*/ &subpass,
        /*dependencyCount*/ 1,
        /*pDependencies*/ &dependency,
    };

    err = vkCreateRenderPass(device_, &rp_info, nullptr, &window->render_pass);
    if (err) {
      DLOG << "vkCreateRenderPass failed. Error: " << err;
      return false;
    }

    CreateDepthImage(window);

    for (uint32_t i = 0; i < swapchain_image_count_; i++) {
      std::array<VkImageView, 2> attachments = {
          window->swapchain_image_resources[i].view, window->depth_view};
      const VkFramebufferCreateInfo fb_info = {
          /*sType*/ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
          /*pNext*/ nullptr,
          /*flags*/ 0,
          /*renderPass*/ window->render_pass,
          /*attachmentCount*/ attachments.size(),
          /*pAttachments*/ attachments.data(),
          /*width*/ (uint32_t)window->width,
          /*height*/ (uint32_t)window->height,
          /*layers*/ 1,
      };

      err = vkCreateFramebuffer(
          device_, &fb_info, nullptr,
          &window->swapchain_image_resources[i].frame_buffer);
      if (err) {
        DLOG << "vkCreateFramebuffer failed. Error: " << err;
        return false;
      }
    }
  }

  // Separate present queue

  if (separate_present_queue_) {
    const VkCommandPoolCreateInfo present_cmd_pool_info = {
        /*sType*/ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        /*pNext*/ nullptr,
        /*flags*/ 0,
        /*queueFamilyIndex*/ present_queue_family_index_,
    };
    err = vkCreateCommandPool(device_, &present_cmd_pool_info, nullptr,
                              &window->present_cmd_pool);
    if (err) {
      DLOG << "vkCreateCommandPool failed. Error: " << err;
      return false;
    }

    const VkCommandBufferAllocateInfo present_cmd_info = {
        /*sType*/ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        /*pNext*/ nullptr,
        /*commandPool*/ window->present_cmd_pool,
        /*level*/ VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        /*commandBufferCount*/ 1,
    };
    for (uint32_t i = 0; i < swapchain_image_count_; i++) {
      err = vkAllocateCommandBuffers(
          device_, &present_cmd_info,
          &window->swapchain_image_resources[i].graphics_to_present_cmd);
      if (err) {
        DLOG << "vkAllocateCommandBuffers failed. Error: " << err;
        return false;
      }

      const VkCommandBufferBeginInfo cmd_buf_info = {
          /*sType*/ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
          /*pNext*/ nullptr,
          /*flags*/ VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
          /*pInheritanceInfo*/ nullptr,
      };
      err = vkBeginCommandBuffer(
          window->swapchain_image_resources[i].graphics_to_present_cmd,
          &cmd_buf_info);
      if (err) {
        DLOG << "vkBeginCommandBuffer failed. Error: " << err;
        return false;
      }

      VkImageMemoryBarrier image_ownership_barrier = {
          /*sType*/ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
          /*pNext*/ nullptr,
          /*srcAccessMask*/ 0,
          /*dstAccessMask*/ VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
          /*oldLayout*/ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
          /*newLayout*/ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
          /*srcQueueFamilyIndex*/ graphics_queue_family_index_,
          /*dstQueueFamilyIndex*/ present_queue_family_index_,
          /*image*/ window->swapchain_image_resources[i].image,
          /*subresourceRange*/ {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

      vkCmdPipelineBarrier(
          window->swapchain_image_resources[i].graphics_to_present_cmd,
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0,
          nullptr, 1, &image_ownership_barrier);
      err = vkEndCommandBuffer(
          window->swapchain_image_resources[i].graphics_to_present_cmd);
      if (err) {
        DLOG << "vkEndCommandBuffer failed. Error: " << err;
        return false;
      }
    }
  }

  // Reset current buffer.
  window->current_buffer = 0;

  return true;
}

bool VulkanContext::CreateDepthImage(Window* window) {
  VkImageCreateInfo depth_image_ci = {
      /*sType*/ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      /*pNext*/ nullptr,
      /*flags*/ 0,
      /*imageType*/ VK_IMAGE_TYPE_2D,
      /*format*/ VK_FORMAT_D24_UNORM_S8_UINT,
      /*extent*/
      {
          /*width*/ window->swapchain_extent.width,
          /*height*/ window->swapchain_extent.height,
          /*depth*/ 1,
      },
      /*mipLevels*/ 1,
      /*arrayLayers*/ 1,
      /*samples*/ VK_SAMPLE_COUNT_1_BIT,
      /*tiling*/ VK_IMAGE_TILING_OPTIMAL,
      /*usage*/ VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      /*sharingMode*/ VK_SHARING_MODE_EXCLUSIVE,
      /*queueFamilyIndexCount*/ 0,
      /*pQueueFamilyIndices*/ nullptr,
      /*initialLayout*/ VK_IMAGE_LAYOUT_UNDEFINED,
  };

  VkResult err =
      vkCreateImage(device_, &depth_image_ci, nullptr, &window->depth_image);
  if (err) {
    DLOG << "vkCreateImage failed. Error: " << err;
    return false;
  }

  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements(device_, window->depth_image, &mem_requirements);

  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(gpu_, &memProperties);
  uint32_t mti = 0;
  for (; mti < memProperties.memoryTypeCount; mti++) {
    if ((mem_requirements.memoryTypeBits & (1 << mti)) &&
        (memProperties.memoryTypes[mti].propertyFlags &
         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ==
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
      break;
    }
  }
  if (mti == memProperties.memoryTypeCount) {
    DLOG << "Memort type index not found.";
    return false;
  }

  VkMemoryAllocateInfo alloc_info = {
      /*sType*/ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      /*pNext*/ nullptr,
      /*allocationSize*/ mem_requirements.size,
      /*memoryTypeIndex*/ mti,
  };
  err = vkAllocateMemory(device_, &alloc_info, nullptr,
                         &window->depth_image_memory);
  if (err) {
    DLOG << "vkAllocateMemory failed. Error: " << err;
    return false;
  }

  vkBindImageMemory(device_, window->depth_image, window->depth_image_memory,
                    0);

  VkImageViewCreateInfo image_view_create_info = {
      /*sType*/ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      /*pNext*/ nullptr,
      /*flags*/ 0,
      /*image*/ window->depth_image,
      /*viewType*/ VK_IMAGE_VIEW_TYPE_2D,
      /*format*/ VK_FORMAT_D24_UNORM_S8_UINT,
      /*components*/
      {
          /*r*/ VK_COMPONENT_SWIZZLE_R,
          /*g*/ VK_COMPONENT_SWIZZLE_G,
          /*b*/ VK_COMPONENT_SWIZZLE_B,
          /*a*/ VK_COMPONENT_SWIZZLE_A,
      },
      /*subresourceRange*/
      {
          /*aspectMask*/ VK_IMAGE_ASPECT_DEPTH_BIT,
          /*baseMipLevel*/ 0,
          /*levelCount*/ 1,
          /*baseArrayLayer*/ 0,
          /*layerCount*/ 1,
      },
  };

  err = vkCreateImageView(device_, &image_view_create_info, nullptr,
                          &window->depth_view);

  if (err) {
    vkDestroyImage(device_, window->depth_image, nullptr);
    vkFreeMemory(device_, window->depth_image_memory, nullptr);
    DLOG << "vkCreateImageView failed with error " << std::to_string(err);
    return false;
  }
  return true;
}

void VulkanContext::AppendCommandBuffer(const VkCommandBuffer& command_buffer,
                                        bool front) {
  if (front)
    command_buffers_.insert(command_buffers_.begin(), command_buffer);
  else
    command_buffers_.push_back(command_buffer);
}

void VulkanContext::Flush(bool all) {
  // Ensure everything else pending is executed.
  vkDeviceWaitIdle(device_);

  // Flush the current frame.
  VkSubmitInfo submit_info;
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pNext = nullptr;
  submit_info.pWaitDstStageMask = nullptr;
  submit_info.waitSemaphoreCount = 0;
  submit_info.pWaitSemaphores = nullptr;
  submit_info.commandBufferCount = command_buffers_.size() - (all ? 0 : 1);
  submit_info.pCommandBuffers = command_buffers_.data();
  submit_info.signalSemaphoreCount = 0;
  submit_info.pSignalSemaphores = nullptr;
  VkResult err =
      vkQueueSubmit(graphics_queue_, 1, &submit_info, VK_NULL_HANDLE);
  command_buffers_[0] = nullptr;
  if (err) {
    DLOG << "vkQueueSubmit failed. Error: " << err;
    return;
  }

  auto draw_command_buffer = command_buffers_.back();
  command_buffers_.clear();
  if (!all)
    command_buffers_.push_back(draw_command_buffer);

  vkDeviceWaitIdle(device_);
}

bool VulkanContext::PrepareBuffers() {
  if (!queues_initialized_)
    return true;

  VkResult err;

  // Ensure no more than kFrameLag renderings are outstanding.
  vkWaitForFences(device_, 1, &fences_[frame_index_], VK_TRUE,
                  std::numeric_limits<uint64_t>::max());
  vkResetFences(device_, 1, &fences_[frame_index_]);

  DCHECK(window_.swapchain != VK_NULL_HANDLE);

  do {
    // Get the index of the next available swapchain image:
    err = AcquireNextImageKHR(device_, window_.swapchain,
                              std::numeric_limits<uint64_t>::max(),
                              image_acquired_semaphores_[frame_index_],
                              VK_NULL_HANDLE, &window_.current_buffer);

    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
      // swapchain is out of date (e.g. the window was resized) and must be
      // recreated:
      DLOG << "Swapchain is out of date.";
      UpdateSwapChain(&window_);
    } else if (err == VK_SUBOPTIMAL_KHR) {
      DLOG << "Swapchain is suboptimal.";
      // swapchain is not as optimal as it could be, but the platform's
      // presentation engine will still present the image correctly.
      break;
    } else {
      if (err) {
        DLOG << "AcquireNextImageKHR failed. Error: " << err;
        return false;
      }
    }
  } while (err != VK_SUCCESS);

  buffers_prepared_ = true;

  return true;
}

size_t VulkanContext::GetAndResetFPS() {
  int ret = fps_;
  fps_ = 0;
  return ret;
}

bool VulkanContext::SwapBuffers() {
  if (!queues_initialized_)
    return true;

  VkResult err;

  // Wait for the image acquired semaphore to be signaled to ensure that the
  // image won't be rendered to until the presentation engine has fully released
  // ownership to the application, and it is okay to render to the image.
  VkPipelineStageFlags pipe_stage_flags;
  VkSubmitInfo submit_info;
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pNext = nullptr;
  submit_info.pWaitDstStageMask = &pipe_stage_flags;
  pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &image_acquired_semaphores_[frame_index_];
  submit_info.commandBufferCount = command_buffers_.size();
  submit_info.pCommandBuffers = command_buffers_.data();
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &draw_complete_semaphores_[frame_index_];
  err = vkQueueSubmit(graphics_queue_, 1, &submit_info, fences_[frame_index_]);
  if (err) {
    DLOG << "vkQueueSubmit failed. Error: " << err;
    return false;
  }

  command_buffers_.clear();

  if (separate_present_queue_) {
    // If we are using separate queues, change image ownership to the present
    // queue before presenting, waiting for the draw complete semaphore and
    // signalling the ownership released semaphore when finished.
    VkFence null_fence = VK_NULL_HANDLE;
    pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &draw_complete_semaphores_[frame_index_];
    submit_info.commandBufferCount = 0;

    VkCommandBuffer* cmdbufptr =
        (VkCommandBuffer*)alloca(sizeof(VkCommandBuffer*));
    submit_info.pCommandBuffers = cmdbufptr;

    DCHECK(window_.swapchain != VK_NULL_HANDLE);

    cmdbufptr[submit_info.commandBufferCount] =
        window_.swapchain_image_resources[window_.current_buffer]
            .graphics_to_present_cmd;
    submit_info.commandBufferCount++;

    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &image_ownership_semaphores_[frame_index_];
    err = vkQueueSubmit(present_queue_, 1, &submit_info, null_fence);
    if (err) {
      DLOG << "vkQueueSubmit failed. Error: " << err;
      return false;
    }
  }

  // If we are using separate queues we have to wait for image ownership,
  // otherwise wait for draw complete.
  VkPresentInfoKHR present = {
      /*sType*/ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      /*pNext*/ nullptr,
      /*waitSemaphoreCount*/ 1,
      /*pWaitSemaphores*/
      (separate_present_queue_) ? &image_ownership_semaphores_[frame_index_]
                                : &draw_complete_semaphores_[frame_index_],
      /*swapchainCount*/ 0,
      /*pSwapchain*/ nullptr,
      /*pImageIndices*/ nullptr,
      /*pResults*/ nullptr,
  };

  VkSwapchainKHR* swapchains = (VkSwapchainKHR*)alloca(sizeof(VkSwapchainKHR*));
  uint32_t* pImageIndices = (uint32_t*)alloca(sizeof(uint32_t*));

  present.pSwapchains = swapchains;
  present.pImageIndices = pImageIndices;

  DCHECK(window_.swapchain != VK_NULL_HANDLE);

  swapchains[present.swapchainCount] = window_.swapchain;
  pImageIndices[present.swapchainCount] = window_.current_buffer;
  present.swapchainCount++;

  err = QueuePresentKHR(present_queue_, &present);

  frame_index_ += 1;
  frame_index_ %= kFrameLag;
  fps_++;

  if (err == VK_ERROR_OUT_OF_DATE_KHR) {
    // Swapchain is out of date (e.g. the window was resized) and must be
    // recreated.
    DLOG << "Swapchain is out of date.";
  } else if (err == VK_SUBOPTIMAL_KHR) {
    // Swapchain is not as optimal as it could be, but the platform's
    // presentation engine will still present the image correctly.
    DLOG << "Swapchain is Suboptimal.";
  } else if (err) {
    DLOG << "QueuePresentKHR failed. Error: " << err;
    return false;
  }

  buffers_prepared_ = false;
  return true;
}

}  // namespace eng
