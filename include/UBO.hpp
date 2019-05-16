#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <vector>

#include "Device.hpp"

#include "VKUtil.hpp"

struct MVP {
  glm::mat4 model =
      glm::rotate(glm::mat4(1.0f), 90.0f, glm::vec3(1.0f, 1.0f, 0.0f));
  glm::mat4 view;
  glm::mat4 proj;
};

template <typename UBOType> class UBO
{
public:
  using type = UBOType;
  constexpr static auto type_size() { return sizeof(type); }
  UBO(Device& device, vk::ShaderStageFlags shaderStage)
      : m_shaderStage{shaderStage}
  {
    std::tie(m_buffer, m_memory) = VKUtil::createBuffer(device, sizeof(type),
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);
  };

  UBOType m_ubo;

  vk::UniqueBuffer m_buffer;
  vk::UniqueDeviceMemory m_memory;
  vk::ShaderStageFlags m_shaderStage;
};

class PipelineLayout
{
public:
  template <typename UBOType> void addUBO(const UBO<UBOType>& ubo)
  {
    vk::DescriptorSetLayoutBinding binding{};
    binding.binding = static_cast<std::uint32_t>(m_bindings.size());
    binding.descriptorType = vk::DescriptorType::eUniformBuffer;
    binding.descriptorCount = 1;
    binding.stageFlags = ubo.m_shaderStage;
    m_bindings.push_back(binding);
  }

  void addSampler()
  {
    vk::DescriptorSetLayoutBinding binding{};
    binding.binding = static_cast<std::uint32_t>(m_bindings.size());
    binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    binding.descriptorCount = 1;
    binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
    m_bindings.push_back(binding);
  }

  vk::UniqueDescriptorSetLayout generateLayout(const Device& device)
  {
    vk::DescriptorSetLayoutCreateInfo createInfo{};
    createInfo.bindingCount = static_cast<std::uint32_t>(m_bindings.size());
    createInfo.pBindings = m_bindings.data();
    return device.device().createDescriptorSetLayoutUnique(createInfo);
  }

  void clear() { m_bindings.clear(); }

private:
  std::vector<vk::DescriptorSetLayoutBinding> m_bindings;
};