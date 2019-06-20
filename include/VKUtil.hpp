#pragma once

#include <filesystem>
#include <fstream>
#include <tuple>

#include <vulkan/vulkan.hpp>

#include "Device.hpp"

namespace VKUtil
{

inline std::uint32_t findMemoryType(vk::PhysicalDevice physicalDevice,
    std::uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
  vk::PhysicalDeviceMemoryProperties memoryProperties =
      physicalDevice.getMemoryProperties();
  for (std::uint32_t i{0u}; i < memoryProperties.memoryTypeCount; ++i) {
    if (typeFilter & (1 << i) &&
        (memoryProperties.memoryTypes[i].propertyFlags & properties) ==
            properties) {
      return i;
    }
  }
  throw std::runtime_error("failed to find suitable memory type!");
}

inline std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory> createBuffer(
    Device& device, vk::DeviceSize size, vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties)
{
  vk::BufferCreateInfo bufferCreateInfo{};
  bufferCreateInfo.size = size;
  bufferCreateInfo.usage = usage;
  bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
  vk::UniqueBuffer buffer =
      device.device().createBufferUnique(bufferCreateInfo);

  vk::MemoryRequirements memoryRequirements =
      device.device().getBufferMemoryRequirements(*buffer);

  vk::MemoryAllocateInfo memoryAllocateInfo{};
  memoryAllocateInfo.allocationSize = memoryRequirements.size;
  memoryAllocateInfo.memoryTypeIndex = findMemoryType(
      device.m_physicalDevice, memoryRequirements.memoryTypeBits, properties);

  vk::UniqueDeviceMemory bufferMemory =
      device.device().allocateMemoryUnique(memoryAllocateInfo);

  device.device().bindBufferMemory(*buffer, *bufferMemory, 0);
  return std::make_pair(std::move(buffer), std::move(bufferMemory));
}

inline std::pair<vk::UniqueImage, vk::UniqueDeviceMemory> createImage(
    Device& device, vk::Extent3D extent, std::uint32_t mipLevels,
    vk::SampleCountFlagBits numSamples, vk::Format format,
    vk::ImageTiling tiling, vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags memoryProperties)
{
  vk::ImageCreateInfo imageCreateInfo{};
  imageCreateInfo.imageType = vk::ImageType::e2D;
  imageCreateInfo.format = format;
  imageCreateInfo.extent = extent;
  imageCreateInfo.mipLevels = mipLevels;
  imageCreateInfo.arrayLayers = 1;
  imageCreateInfo.samples = numSamples;
  imageCreateInfo.tiling = tiling;
  imageCreateInfo.usage = usage;
  imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
  imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

  vk::UniqueImage image = device.device().createImageUnique(imageCreateInfo);

  vk::MemoryRequirements memRequirements =
      device.device().getImageMemoryRequirements(*image);

  vk::MemoryAllocateInfo allocateInfo{};
  allocateInfo.allocationSize = memRequirements.size;
  allocateInfo.memoryTypeIndex = findMemoryType(device.m_physicalDevice,
      memRequirements.memoryTypeBits, memoryProperties);

  vk::UniqueDeviceMemory deviceMemory =
      device.device().allocateMemoryUnique(allocateInfo);

  device.device().bindImageMemory(*image, *deviceMemory, 0);

  return std::make_pair(std::move(image), std::move(deviceMemory));
}

inline vk::UniqueCommandBuffer beginSingleTimeCommands(Device& device)
{
  vk::UniqueCommandBuffer ret;
  vk::CommandBufferAllocateInfo allocateInfo{};
  allocateInfo.level = vk::CommandBufferLevel::ePrimary;
  allocateInfo.commandPool = *device.m_commandPool;
  allocateInfo.commandBufferCount = 1;

  std::vector<vk::UniqueCommandBuffer> commandBuffers =
      device.device().allocateCommandBuffersUnique(allocateInfo);

  vk::CommandBufferBeginInfo beginInfo{};
  beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

  commandBuffers.front()->begin(beginInfo);
  ret = std::move(commandBuffers.front());
  return ret;
}

inline void endSingleTimeCommands(
    vk::UniqueCommandBuffer& commandBuffer, vk::Queue queue)
{
  commandBuffer->end();

  vk::SubmitInfo submitInfo{};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &*commandBuffer;

  queue.submit(submitInfo, {});
  queue.waitIdle();
}

inline void copyBufferToImage(Device& device, vk::CommandPool commandPool,
    vk::Queue queue, vk::Buffer buffer, vk::Image image, std::uint32_t width,
    std::uint32_t height)
{
  auto commandBuffer = beginSingleTimeCommands(device);

  vk::BufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = {{0, 0, 0}};
  region.imageExtent = {{width, height, 1}};

  commandBuffer->copyBufferToImage(
      buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);

  endSingleTimeCommands(commandBuffer, queue);
}

inline void copyBuffer(Device& device, vk::Buffer srcBuffer,
    vk::Buffer dstBuffer, vk::DeviceSize size)
{
  vk::CommandBufferAllocateInfo allocateInfo = {};
  allocateInfo.level = vk::CommandBufferLevel::ePrimary;
  allocateInfo.commandPool = *device.m_commandPool;
  allocateInfo.commandBufferCount = 1;

  std::vector<vk::UniqueCommandBuffer> commandBuffers =
      device.device().allocateCommandBuffersUnique(allocateInfo);
  auto& commandBuffer = commandBuffers.front();

  vk::CommandBufferBeginInfo beginInfo = {};
  beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

  commandBuffer->begin(beginInfo);

  vk::BufferCopy copyRegion{};
  copyRegion.size = size;
  commandBuffer->copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

  commandBuffer->end();

  vk::SubmitInfo submitInfo{};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &*commandBuffer;

  device.m_graphicsQueue.submit(1, &submitInfo, {});
  device.m_graphicsQueue.waitIdle();
}

/*
FIX COMPILATION ERRORS TO ALLOW THIS

inline void copyBuffer(Device& device, const Buffer& src, Buffer& dst)
{
  copyBuffer(device, *src.m_buffer, *dst.m_buffer);
}
*/
inline bool hasStencilComponent(const vk::Format& format)
{
  return format == vk::Format::eD32SfloatS8Uint ||
         format == vk::Format::eD24UnormS8Uint;
}

inline bool hasDepthComponent(const vk::Format& format)
{
  return format == vk::Format::eD16Unorm || format == vk::Format::eD32Sfloat ||
         format == vk::Format::eD24UnormS8Uint;
}

inline bool hasDepthStencilComponent(const vk::Format& format)
{
  return hasDepthComponent(format) || hasStencilComponent(format);
}

inline void transitionImageLayout(Device& device, vk::Image image,
    vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
    uint32_t mipLevels)
{
  auto commandBuffer = beginSingleTimeCommands(device);

  vk::ImageMemoryBarrier barrier{};
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;

  if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

    if (hasStencilComponent(format)) {
      barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
    }
  } else {
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  }

  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = mipLevels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  vk::PipelineStageFlags srcStage{};
  vk::PipelineStageFlags dstStage{};

  if (oldLayout == vk::ImageLayout::eUndefined &&
      newLayout == vk::ImageLayout::eTransferDstOptimal) {
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

    srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
    dstStage = vk::PipelineStageFlagBits::eTransfer;
  } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
             newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    srcStage = vk::PipelineStageFlagBits::eTransfer;
    dstStage = vk::PipelineStageFlagBits::eFragmentShader;
  } else if (oldLayout == vk::ImageLayout::eUndefined &&
             newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead |
                            vk::AccessFlagBits::eDepthStencilAttachmentWrite;

    srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
    dstStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
  } else if (oldLayout == vk::ImageLayout::eUndefined &&
             newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead |
                            vk::AccessFlagBits::eColorAttachmentWrite;

    srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
    dstStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

  } else if (oldLayout == vk::ImageLayout::eUndefined &&
             newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
    dstStage = vk::PipelineStageFlagBits::eAllGraphics;
  } else {
    throw std::runtime_error("unsupported layout transition!");
  }

  commandBuffer->pipelineBarrier(
      srcStage, dstStage, {}, 0, nullptr, 0, nullptr, 1, &barrier);

  endSingleTimeCommands(commandBuffer, device.m_graphicsQueue);
}

inline vk::UniqueImageView createImageView(vk::Device device, vk::Image image,
    vk::Format format, vk::ImageAspectFlags aspectFlags,
    std::uint32_t mipLevels)
{
  vk::ImageViewCreateInfo imageViewCreateInfo{};
  imageViewCreateInfo.image = image;
  imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
  imageViewCreateInfo.format = format;
  imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
  imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
  imageViewCreateInfo.subresourceRange.levelCount = mipLevels;
  imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
  imageViewCreateInfo.subresourceRange.layerCount = 1;

  return device.createImageViewUnique(imageViewCreateInfo);
}

inline void generateMipmaps(Device& device, vk::Image image, vk::Format format,
    std::uint32_t texWidth, std::uint32_t texHeight, uint32_t mipLevels)
{
  vk::FormatProperties formatProperties =
      device.m_physicalDevice.getFormatProperties(format);

  if (!(formatProperties.optimalTilingFeatures &
          vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
    throw std::runtime_error(
        "texture image format does not support linear blitting!");
  }

  auto commandBuffer = beginSingleTimeCommands(device);

  vk::ImageMemoryBarrier barrier = {};
  barrier.image = image;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  std::int32_t mipWidth = texWidth;
  std::int32_t mipHeight = texHeight;

  for (uint32_t i = 1; i < mipLevels; i++) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

    commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags{}, 0, nullptr,
        0, nullptr, 1, &barrier);

    vk::ImageBlit blit{};
    blit.srcOffsets[0] = {{0, 0, 0}};
    blit.srcOffsets[1] = {{mipWidth, mipHeight, 1}};
    blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = {{0, 0, 0}};
    blit.dstOffsets[1] = {{mipWidth > 1 ? mipWidth / 2 : 1,
        mipHeight > 1 ? mipHeight / 2 : 1, 1}};
    blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    commandBuffer->blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image,
        vk::ImageLayout::eTransferDstOptimal, 1, &blit, vk::Filter::eLinear);

    barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader, vk::DependencyFlags{}, 0,
        nullptr, 0, nullptr, 1, &barrier);

    if (mipWidth > 1)
      mipWidth /= 2;
    if (mipHeight > 1)
      mipHeight /= 2;
  }

  endSingleTimeCommands(commandBuffer, device.m_transferQueue);
}

template <typename UniqueType>
VULKAN_HPP_INLINE std::vector<typename UniqueType::element_type> uniqueToRaw(
    std::vector<UniqueType> const& handles)
{
  std::vector<UniqueType::element_type> newBuffer(handles.size());
  std::transform(handles.begin(), handles.end(), newBuffer.begin(),
      [](auto const& handle) { return handle.get(); });
  return newBuffer;
}

template <typename T>
void transferToGPU(const Device& device, vk::DeviceMemory memory, const T& src)
{
  void* data = device.device().mapMemory(memory, 0, sizeof(T));
  std::memcpy(data, &src, sizeof(T));
  // std::copy(src, src + sizeof(T), data);
  device.device().unmapMemory(memory);
}
inline vk::SampleCountFlagBits getMaxUsableSampleCount(Device& device)
{
  VkPhysicalDeviceProperties physicalDeviceProperties =
      device.m_physicalDeviceProperties;

  VkSampleCountFlags counts =
      std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts,
          physicalDeviceProperties.limits.framebufferDepthSampleCounts);
  if (counts & VK_SAMPLE_COUNT_64_BIT) {
    return vk::SampleCountFlagBits::e64;
  }
  if (counts & VK_SAMPLE_COUNT_32_BIT) {
    return vk::SampleCountFlagBits::e32;
  }
  if (counts & VK_SAMPLE_COUNT_16_BIT) {
    return vk::SampleCountFlagBits::e16;
  }
  if (counts & VK_SAMPLE_COUNT_8_BIT) {
    return vk::SampleCountFlagBits::e8;
  }
  if (counts & VK_SAMPLE_COUNT_4_BIT) {
    return vk::SampleCountFlagBits::e4;
  }
  if (counts & VK_SAMPLE_COUNT_2_BIT) {
    return vk::SampleCountFlagBits::e2;
  }

  return vk::SampleCountFlagBits::e1;
}

inline std::vector<unsigned char> getFileData(const std::filesystem::path& path)
{
  std::ifstream iFILE(path, std::ios::binary);
  return std::vector<unsigned char>((std::istreambuf_iterator<char>(iFILE)),
      std::istreambuf_iterator<char>());
}

inline

    uint32_t
    findMemoryType(Device& device, std::uint32_t typeFilter,
        vk::MemoryPropertyFlags properties)
{
  vk::PhysicalDeviceMemoryProperties memoryProperties =
      device.m_physicalDevice.getMemoryProperties();
  for (std::uint32_t i{0u}; i < memoryProperties.memoryTypeCount; ++i) {
    if (typeFilter & (1 << i) &&
        (memoryProperties.memoryTypes[i].propertyFlags & properties) ==
            properties) {
      return i;
    }
  }
  throw std::runtime_error("failed to find suitable memory type!");
}

inline vk::Format findSupportedFormat(Device& device,
    const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
    vk::FormatFeatureFlags features)
{
  for (auto format : candidates) {
    vk::FormatProperties properties =
        device.m_physicalDevice.getFormatProperties(format);
    if (tiling == vk::ImageTiling::eLinear &&
        (properties.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == vk::ImageTiling::eOptimal &&
               (properties.optimalTilingFeatures & features) == features) {
      return format;
    }
  }
  throw std::runtime_error("Failed to find supported format!");
}

inline vk::Format findDepthFormat(Device& device)
{
  return findSupportedFormat(device,
      {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint,
          vk::Format::eD24UnormS8Uint},
      vk::ImageTiling::eOptimal,
      vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

inline vk::UniqueSampler createTextureSampler(
    const Device& device, std::uint32_t mipLevels = 1)
{
  vk::SamplerCreateInfo samplerInfo{};
  samplerInfo.minFilter = vk::Filter::eLinear;
  samplerInfo.magFilter = vk::Filter::eLinear;
  //////samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToBorder;
  /////CHANGE
  samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
  samplerInfo.addressModeV = samplerInfo.addressModeU;
  samplerInfo.addressModeW = samplerInfo.addressModeU;
  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy = 16;
  samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_TRUE;
  samplerInfo.compareOp = vk::CompareOp::eAlways;
  samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = static_cast<float>(mipLevels);

  return device.device().createSamplerUnique(samplerInfo);
}

} // namespace VKUtil
