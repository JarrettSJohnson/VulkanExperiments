#pragma once

#include "Vertex.hpp"
#include "Window.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <vulkan/vulkan.hpp>

#include "Cube.hpp"
#include "DescriptorSet.hpp"
#include "Device.hpp"
#include "Framebuffer.hpp"
#include "Model.hpp"
#include "Pipeline.hpp"
#include "PushConstants.hpp"
#include "RenderPass.hpp"
#include "Swapchain.hpp"
#include "Texture.hpp"
#include "UBO.hpp"
#include "UIOverlay.hpp"

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

  // private:
public:
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

  std::unique_ptr<RenderPass> offscreenRenderPass{};
  std::unique_ptr<Framebuffer> offscreenFB{};
  PipelineLayout offscreenPipelineLayout{};
  Pipeline offscreenPipeline{};
  // vk::UniqueDescriptorSetLayout offscreenDescriptorSetLayout{};

  std::unique_ptr<RenderPass> m_renderPass{};
  std::vector<Framebuffer> m_framebuffers{};
  PipelineLayout m_graphicsPipelineLayout{};
  Pipeline m_graphicsPipeline{};
  // vk::UniqueDescriptorSetLayout m_descriptorSetLayout{};

  std::array<vk::UniqueSemaphore, framesInFlight> m_drawSemaphores;
  std::array<vk::UniqueSemaphore, framesInFlight> m_presentSemaphores;
  std::array<vk::UniqueFence, framesInFlight> m_fences;

  Window m_window{};
  std::array<std::uint32_t, 1> m_familyIndex{0u};
#ifdef NDEBUG
  const bool enableValidationLayers = false;
#else
  const bool enableValidationLayers = true;
#endif

  //Model m_model;

  std::uint32_t m_mipLevels{};

  //Texture m_texture{};

  DescriptorSet offscreenDescriptorSets{};
  // TODO: generate takes a size for blah blah swapchain
  std::vector<DescriptorSet> m_DescriptorSet{}; // 2?

  std::unique_ptr<UIOverlay> ui;

  /*vk::UniqueDescriptorPool offscreenDescriptorPool{};
  std::vector<vk::DescriptorSet> offscreenDescriptorSets{};

  vk::UniqueDescriptorPool m_DescriptorPool{};
  std::vector<vk::DescriptorSet> m_DescriptorSets{};*/

public:
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
  void createFramebuffers2();
  void createCommandPool();
  void createColorResources();
  void createDepthResources();

  void createTextureSampler();

  void updateUniformBuffer(uint32_t currentImage)
  {
    // VKUtil::transferToGPU(m_device, *m_UBOs[currentImage].m_memory, newUBOT);
    m_UBO->copyData();
  }
  void createUniformBuffers();
  void createDescriptorPool();
  void createDescriptorSets();
  // void createCommandBuffers();
  std::unique_ptr<UBO<LightUniforms>> m_UBO;
  struct IndexInfo {
    vk::Buffer vBuffer{};
    vk::Buffer iBuffer{};
    std::uint32_t numIndices{};
    PushConstants pushConstants{};
  };
  void allocateCommandBuffers();
  void setupCommandBuffers(
      const std::vector<IndexInfo>& buffers, std::size_t currentFrame);
  void createSyncs();
  std::uint32_t getImageIdx();
  void drawFrame(std::uint32_t imageIdx);
  void present(std::uint32_t imageIdx);

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
