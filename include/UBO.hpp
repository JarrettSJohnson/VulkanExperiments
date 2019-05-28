#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include "Device.hpp"

#include "Texture.hpp"
#include "VKUtil.hpp"

template <typename UBOType> class UBO
{
public:
  using type = UBOType;
  constexpr static auto type_size() { return sizeof(type); }
  UBO(Device& device, vk::ShaderStageFlags shaderStage)
      : m_device{device}, m_shaderStage{shaderStage}
  {
    std::tie(m_buffer, m_memory) = VKUtil::createBuffer(device, sizeof(type),
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);
  };

  UBOType m_ubo;
  vk::Device m_device;
  vk::UniqueBuffer m_buffer;
  vk::UniqueDeviceMemory m_memory;
  vk::ShaderStageFlags m_shaderStage;
  void* mappedMem{};
  UBOType& get() { return m_ubo; }
  void map() { mappedMem = m_device.mapMemory(*m_memory, 0, sizeof(type)); };
  void unmap()
  {
    if (mappedMem) {
      m_device.unmapMemory(*m_memory);
    }
    mappedMem = nullptr;
  }
  void copyData() { std::memcpy(mappedMem, &m_ubo, sizeof(type)); };
  vk::Buffer buffer() const { return *m_buffer; }
};
