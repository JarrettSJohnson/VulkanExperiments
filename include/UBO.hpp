#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "Device.hpp"

#include "Texture.hpp"
#include "VKUtil.hpp"

struct MVP {
  // glm::mat4 model =
  //
  // glm::mat4 view;
  // glm::mat4 proj;
  glm::mat4 mvp = glm::mat4(1.0f);
};

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

class DescriptorSet
{
public:
  struct UBODescriptorItem {
    vk::DescriptorSetLayoutBinding binding;
    vk::Buffer buffer;
    vk::DeviceSize size;
    std::uint32_t idx;
  };
  struct SamplerDescriptorItem {
    vk::DescriptorSetLayoutBinding binding;
    vk::ImageView view;
    vk::Sampler sampler;
    std::uint32_t idx;
  };

public:
  DescriptorSet() = default;

  template <typename UBOType> void addUBO(const UBO<UBOType>& ubo)
  {
    auto& item = m_uniformBindings.emplace_back();
    item.idx = m_idx++;
    item.binding.binding = item.idx;
    item.binding.descriptorType = vk::DescriptorType::eUniformBuffer;
    item.binding.descriptorCount = 1;
    item.binding.stageFlags = ubo.m_shaderStage;
    item.buffer = ubo.buffer();
    item.size = ubo.type_size();
  }

  void addSampler(const vk::ImageView view, const vk::Sampler sampler)
  {
    auto& item = m_samplerBindings.emplace_back();
    item.idx = m_idx++;
    item.binding.binding = item.idx;
    item.binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    item.binding.descriptorCount = 1;
    item.binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
    item.view = view;
    item.sampler = sampler;
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

  void generateLayout(const Device& device)
  {
    m_layout.resize(m_uniformBindings.size() + m_samplerBindings.size());
    for (const auto& binding : m_uniformBindings) {
      m_layout[binding.idx] = binding.binding;
    }
    for (const auto& binding : m_samplerBindings) {
      m_layout[binding.idx] = binding.binding;
    }
    vk::DescriptorSetLayoutCreateInfo createInfo{};
    createInfo.bindingCount = static_cast<std::uint32_t>(m_layout.size());
    createInfo.pBindings = m_layout.data();
    m_descriptorSetLayout =
        device.device().createDescriptorSetLayoutUnique(createInfo);
  }

  void generatePool(const Device& device)
  {
    std::vector<vk::DescriptorPoolSize> descriptorPoolSizes;
    for (const auto& binding : m_layout) {
      auto& descriptorPoolSize = descriptorPoolSizes.emplace_back();
      descriptorPoolSize.type = binding.descriptorType;
      descriptorPoolSize.descriptorCount = 1;
    }

    vk::DescriptorPoolCreateInfo poolCreateInfo{};
    poolCreateInfo.poolSizeCount =
        static_cast<std::uint32_t>(descriptorPoolSizes.size());
    poolCreateInfo.pPoolSizes = descriptorPoolSizes.data();
    poolCreateInfo.maxSets = 1;

    m_descriptorPool =
        device.device().createDescriptorPoolUnique(poolCreateInfo);
    allocate(device);
  }

  void allocate(const Device& device) 
  {
	vk::DescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.descriptorPool = *m_descriptorPool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &*m_descriptorSetLayout;
    m_descriptorSets = device.device().allocateDescriptorSets(allocateInfo);
    updateDescriptors(device);
  }

  void updateDescriptors(const Device& device)
  {
    for (const auto& ubo : m_uniformBindings) {
      for (std::size_t i{0u}; i < m_descriptorSets.size(); ++i) {
        vk::DescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = ubo.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = ubo.size;

        vk::WriteDescriptorSet descriptorWrite{};

        descriptorWrite.dstSet = m_descriptorSets[i];
        descriptorWrite.dstBinding = ubo.binding.binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        device.device().updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
      }
    }
    for (const auto& sampler : m_samplerBindings) {
      for (std::size_t i{0u}; i < m_descriptorSets.size(); ++i) {
        vk::DescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView = sampler.view;
        imageInfo.sampler = sampler.sampler;

        vk::WriteDescriptorSet descriptorWrite{};

        descriptorWrite.dstSet = m_descriptorSets[i];
        descriptorWrite.dstBinding = sampler.binding.binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType =
            vk::DescriptorType::eCombinedImageSampler;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        device.device().updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
      }
    }
  }

  void clear() { *this = DescriptorSet{}; }
  vk::DescriptorSetLayout layout() const { return *m_descriptorSetLayout; }
  vk::DescriptorPool pool() const { return *m_descriptorPool; }
  const std::vector<vk::DescriptorSet>& descriptorSets() const
  {
    return m_descriptorSets;
  }
  private:

  std::uint32_t m_idx{};
  vk::UniqueDescriptorSetLayout m_descriptorSetLayout{};
  vk::UniqueDescriptorPool m_descriptorPool {};
  std::vector<vk::DescriptorSet> m_descriptorSets{};
  std::vector<UBODescriptorItem> m_uniformBindings;
  std::vector<SamplerDescriptorItem> m_samplerBindings;
  std::vector<vk::DescriptorSetLayoutBinding> m_layout;
};