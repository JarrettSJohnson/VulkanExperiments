#pragma once

#include "Vertex.hpp"
#include "Window.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <vulkan/vulkan.hpp>

#include "Device.hpp"
#include "Model.hpp"
#include "Texture.hpp"
#include "UBO.hpp"
#include "Framebuffer.hpp"
#include "Swapchain.hpp"
#include "Pipeline.hpp"

inline VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger)
{
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

inline void DestroyDebugUtilsMessengerEXT(VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator)
{
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

inline void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
}


class Application
{
public:
  Application();
  void run();
 // bool framebufferResized{false};
  void recreateSwapchain();

private:
  static constexpr std::size_t framesInFlight{2};
  std::size_t currentFrame{0};

  std::array<const char*, 1> m_enabledLayers{
      "VK_LAYER_LUNARG_standard_validation"};

  std::array<const char*, 1> m_deviceExtension{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  vk::UniqueInstance m_instance{};
  vk::DispatchLoaderDynamic m_dldy;
  vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic>
      m_debugMessenger{};
  vk::UniqueSurfaceKHR m_surface{};

  Device m_device;

  Swapchain m_swapchain{};

  std::vector<vk::UniqueCommandBuffer> m_commandBuffers{};

  Pipeline m_graphicsPipeline{};
  vk::UniqueRenderPass m_renderPass{};
  vk::UniqueDescriptorSetLayout m_descriptorSetLayout{};
  std::vector<vk::UniqueFramebuffer> m_framebuffers{};
  std::array<vk::UniqueSemaphore, framesInFlight> m_drawSemaphores{};
  std::array<vk::UniqueSemaphore, framesInFlight> m_presentSemaphores{};
  std::array<vk::UniqueFence, framesInFlight> m_fences{};

  Window m_window{};
  std::array<std::uint32_t, 1> m_familyIndex{0u};
#ifdef NDEBUG
  const bool enableValidationLayers = false;
#else
  const bool enableValidationLayers = true;
#endif

  std::vector<UBO<MVP>> m_UBOs;

  Model m_model;

  vk::UniqueImage m_depthImage{};
  vk::UniqueDeviceMemory m_depthImageMemory{};
  vk::UniqueImageView m_depthImageView{};

  vk::UniqueImage colorImage{};
  vk::UniqueDeviceMemory colorImageMemory{};
  vk::UniqueImageView colorImageView{};

  std::uint32_t m_mipLevels{};

  Texture m_texture{};
  vk::UniqueSampler m_textureSampler;

  vk::UniqueDescriptorPool m_descriptorPool{};
  std::vector<vk::DescriptorSet> m_descriptorSets{};

private:
  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
      void* pUserData)
  {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
  }

  void setupDebugMessenger()
  {
    if (!enableValidationLayers)
      return;

    vk::DebugUtilsMessengerCreateInfoEXT debugInfo{};
    debugInfo.messageSeverity =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
    debugInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
    debugInfo.pfnUserCallback = debugCallback;
    m_debugMessenger = m_instance->createDebugUtilsMessengerEXTUnique(
        debugInfo, nullptr, m_dldy);
  }

  bool checkValidationLayerSupport()
  {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : m_enabledLayers) {
      bool layerFound = false;

      for (const auto& layerProperties : availableLayers) {
        if (strcmp(layerName, layerProperties.layerName) == 0) {
          layerFound = true;
          break;
        }
      }

      if (!layerFound) {
        return false;
      }
    }

    return true;
  }

  void initVulkan();
  void selectPhysicalDevice();
  void createDevice();
  void createRenderPass();
  void createDescriptorSetLayout();
  void createPipeline();
  void createFramebuffers();
  void createCommandPool();
  void createColorResources();
  void createDepthResources();

  void createTextureSampler();

  void updateUniformBuffer(uint32_t currentImage);
  void createUniformBuffers();
  void createDescriptorPool();
  void createDescriptorSets();
  void createCommandBuffers();
  void createSyncs();
  void drawFrame();

  // support
  std::vector<const char*> getRequiredExtensions();
  struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  };
  void checkSwapChainSupport() { SwapChainSupportDetails details; }
  void generateMipmaps(vk::Image image, vk::Format format,
      std::uint32_t texWidth, std::uint32_t texHeight, std::uint32_t mipLevels);
};
