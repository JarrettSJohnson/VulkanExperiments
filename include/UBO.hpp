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
      : m_shaderStage{shaderStage}, m_buffer{device}
  {
    m_buffer.generate(vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        type_size());
  };

  UBOType m_ubo;
  Buffer m_buffer;
  vk::ShaderStageFlags m_shaderStage;
  UBOType& get() { return m_ubo; }
  void map() { m_buffer.map(); }
  void unmap() { m_buffer.unmap(); }
  void copyData() { m_buffer.copyData(&m_ubo, type_size()); }
  vk::Buffer buffer() const { return *m_buffer.m_buffer; }
};
