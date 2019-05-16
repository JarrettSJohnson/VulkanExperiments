#pragma once

#include "Device.hpp"
#include "VKUtil.hpp"

class Swapchain
{
public:
  Swapchain() = default;
  Swapchain(Device& device, vk::SurfaceKHR surface)
  {
    vk::SurfaceCapabilitiesKHR surfaceCapabilities =
        device.m_physicalDevice.getSurfaceCapabilitiesKHR(surface);
    m_extent = surfaceCapabilities.currentExtent;
    vk::Bool32 w = device.m_physicalDevice.getSurfaceSupportKHR(0, surface);

    std::vector<vk::SurfaceFormatKHR> formats =
        device.m_physicalDevice.getSurfaceFormatsKHR(surface);
    m_format = formats.front().format;

    std::vector<vk::PresentModeKHR> presentModes =
        device.m_physicalDevice.getSurfacePresentModesKHR(surface);

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{};
    swapChainCreateInfo.surface = surface;
    swapChainCreateInfo.minImageCount = surfaceCapabilities.minImageCount;
    swapChainCreateInfo.imageFormat = m_format;
    swapChainCreateInfo.imageColorSpace = formats.front().colorSpace;
    swapChainCreateInfo.imageExtent.width = m_extent.width;
    swapChainCreateInfo.imageExtent.height = m_extent.height;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
    swapChainCreateInfo.queueFamilyIndexCount = 1; //todo
    swapChainCreateInfo.pQueueFamilyIndices = 0; //todo
    swapChainCreateInfo.presentMode = presentModes.front();
    swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

    m_swapchain = device.device().createSwapchainKHRUnique(swapChainCreateInfo);

    m_images = device.device().getSwapchainImagesKHR(*m_swapchain);

    m_imageViews.resize(m_images.size());

    for (std::size_t i{0u}; i < m_images.size(); ++i) {
      m_imageViews[i] = VKUtil::createImageView(
          device, m_images[i], m_format, vk::ImageAspectFlagBits::eColor, 1);
    }
  }

  vk::Format format() const { return m_format; }
  vk::Extent3D extent3D() const { 
	  return vk::Extent3D{m_extent.width, m_extent.height, 1};
  }
  vk::Extent2D extent() const { return m_extent; }
  std::size_t size() const { return m_images.size(); }
  vk::ImageView imageView(std::size_t idx) const { return *m_imageViews[idx]; }

  vk::SwapchainKHR swapchain() const { return *m_swapchain; }

private:
  vk::UniqueSwapchainKHR m_swapchain;
  vk::Extent2D m_extent;
  vk::Format m_format;
  std::vector<vk::Image> m_images;
  std::vector<vk::UniqueImageView> m_imageViews{};
};