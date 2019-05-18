#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.hpp>

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

class DescriptorSetLayout
{
public:
  struct UBODescriptorItem {
    vk::DescriptorSetLayoutBinding binding;
    std::vector<vk::Buffer> buffers;
    vk::DeviceSize size{};
    std::uint32_t idx;
  };
  struct SamplerDescriptorItem {
    vk::DescriptorSetLayoutBinding binding;
    vk::ImageView view;
    vk::Sampler sampler;
    std::uint32_t idx;
  };

public:
  DescriptorSetLayout(std::uint32_t swapchainsize)
      : m_swapchainSize{swapchainsize}
  {
  }
  /*template <typename UBOType> void addUBO(const UBO<UBOType>& ubo)
  {
    vk::DescriptorSetLayoutBinding binding{};
    binding.binding = static_cast<std::uint32_t>(m_bindings.size());
    binding.descriptorType = vk::DescriptorType::eUniformBuffer;
    binding.descriptorCount = 1;
    binding.stageFlags = ubo.m_shaderStage;
    m_bindings.push_back(binding);
  }*/

  template <typename UBOType> void addUBO(const std::vector<UBO<UBOType>>& ubos)
  {
    auto& item = m_UBOBindings.emplace_back();
    item.idx = m_idx++;
    item.binding.binding = item.idx;
    item.binding.descriptorType = vk::DescriptorType::eUniformBuffer;
    item.binding.descriptorCount = 1;
    item.binding.stageFlags = ubos.front().m_shaderStage;
    item.buffers.resize(ubos.size());
    std::transform(ubos.begin(), ubos.end(), item.buffers.begin(),
        [](const auto& ubo) { return *ubo.m_buffer; });
    item.size = ubos.front().type_size();
  }

  void addSampler(const Texture& texture)
  {
    auto& item = m_samplerBindings.emplace_back();
    item.idx = m_idx++;
    item.binding.binding = item.idx;
    item.binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    item.binding.descriptorCount = 1;
    item.binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
    item.view = texture.view();
    item.sampler = texture.sampler();
  }

  vk::UniqueDescriptorSetLayout generateLayout(const Device& device)
  {
    m_layout.resize(m_UBOBindings.size() + m_samplerBindings.size());
    for (const auto& binding : m_UBOBindings) {
      m_layout[binding.idx] = binding.binding;
    }
    for (const auto& binding : m_samplerBindings) {
      m_layout[binding.idx] = binding.binding;
    }
    vk::DescriptorSetLayoutCreateInfo createInfo{};
    createInfo.bindingCount = static_cast<std::uint32_t>(m_layout.size());
    createInfo.pBindings = m_layout.data();
    return device.device().createDescriptorSetLayoutUnique(createInfo);
  }

  vk::UniqueDescriptorPool generatePool(
      const Device& device)
  {
    std::vector<vk::DescriptorPoolSize> descriptorPoolSizes;
    for (const auto& binding : m_layout) {
      auto& descriptorPoolSize = descriptorPoolSizes.emplace_back();
      descriptorPoolSize.type = binding.descriptorType;
      descriptorPoolSize.descriptorCount = m_swapchainSize;
    }

    vk::DescriptorPoolCreateInfo poolCreateInfo{};
    poolCreateInfo.poolSizeCount =
        static_cast<std::uint32_t>(descriptorPoolSizes.size());
    poolCreateInfo.pPoolSizes = descriptorPoolSizes.data();
    poolCreateInfo.maxSets = m_swapchainSize;

    return device.device().createDescriptorPoolUnique(poolCreateInfo);
  }

  void updateDescriptors(const Device& device,
      const std::vector<vk::DescriptorSet>& descriptorSets)
  {
    for (const auto& ubo : m_UBOBindings) {
      for (std::size_t i{0u}; i < m_swapchainSize; ++i) {
        vk::DescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = ubo.buffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = ubo.size;

        vk::WriteDescriptorSet descriptorWrite{};

        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        device.device().updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
      }
    }
    for (const auto& sampler : m_samplerBindings) {
      for (std::size_t i{0u}; i < m_swapchainSize; ++i) {
        vk::DescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView = sampler.view;
        imageInfo.sampler = sampler.sampler;

        vk::WriteDescriptorSet descriptorWrite{};

        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 1;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType =
            vk::DescriptorType::eCombinedImageSampler;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        device.device().updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
      }
    }
  }

  void clear() { *this = DescriptorSetLayout{m_swapchainSize}; }

private:
  std::uint32_t m_swapchainSize{};
  std::uint32_t m_idx{};
  std::vector<UBODescriptorItem> m_UBOBindings;
  std::vector<SamplerDescriptorItem> m_samplerBindings;
  std::vector<vk::DescriptorSetLayoutBinding> m_layout;
};