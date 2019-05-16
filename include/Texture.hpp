#pragma once

#include <filesystem>
#include <utility>

#include "VKUtil.hpp"

class STB_Image
{
public:
  STB_Image() = default;
  STB_Image(const std::filesystem::path& filename);
  STB_Image(const STB_Image&) = delete;
  STB_Image& operator=(const STB_Image&) = delete;
  STB_Image(STB_Image&& other) noexcept;
  STB_Image& operator=(STB_Image&& other) noexcept;
  ~STB_Image();

  const auto data() const { return m_data; }
  auto size() const { return m_width * m_height * pixelSize(); }
  std::pair<int, int> dimensions() { return std::make_pair(m_width, m_height); }

private:
  unsigned char* m_data{nullptr};
  int m_width{};
  int m_height{};

  constexpr std::uint32_t pixelSize() const { return sizeof(std::uint32_t); }
};

class Texture
{
public:
  Texture() = default;
  Texture(Device& device, const std::filesystem::path& path)
      : m_rawImage{path}
  {
    vk::DeviceSize size = m_rawImage.size();
    auto [stagingBuffer, stagingBufferMemory] =
        VKUtil::createBuffer(device, size,
			vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent);
    void* data;
    device.device().mapMemory(
        *stagingBufferMemory, 0, size, vk::MemoryMapFlags{}, &data);
    std::memcpy(data, m_rawImage.data(), static_cast<std::size_t>(size));
    device.device().unmapMemory(*stagingBufferMemory);

    auto [width, height] = m_rawImage.dimensions();

    m_mipLevels =
        static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) +
        1;

    std::tie(m_image, m_textureImageMemory) = VKUtil::createImage(device,
        vk::Extent3D{static_cast<std::uint32_t>(width),
                        static_cast<std::uint32_t>(height), 1},
            m_mipLevels, vk::SampleCountFlagBits::e1,
            vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransferSrc |
                vk::ImageUsageFlagBits::eTransferDst |
                vk::ImageUsageFlagBits::eSampled,
            vk::MemoryPropertyFlagBits::eDeviceLocal);

    VKUtil::transitionImageLayout(device, *m_image, vk::Format::eR8G8B8A8Unorm,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
        m_mipLevels);

    VKUtil::copyBufferToImage(device, *device.m_commandPool, device.m_transferQueue, *stagingBuffer, *m_image,
        static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    VKUtil::generateMipmaps(device, *m_image, vk::Format::eR8G8B8A8Unorm, width, height,
        m_mipLevels);
    m_imageView = VKUtil::createImageView(device, *m_image,
        vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor, 1);
  }
  vk::ImageView view() const { return *m_imageView; };

private:
  STB_Image m_rawImage{};
  vk::UniqueImage m_image{};
  vk::UniqueDeviceMemory m_textureImageMemory{};
  vk::ImageLayout m_imageLayout{};
  vk::UniqueImageView m_imageView{};
  std::uint32_t m_mipLevels{};
  std::uint32_t layerCount{};
  vk::DescriptorImageInfo m_descriptor{};
};