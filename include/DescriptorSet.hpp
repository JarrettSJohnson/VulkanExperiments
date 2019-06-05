#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "Device.hpp"

#include "Texture.hpp"
#include "UBO.hpp"
#include "VKUtil.hpp"

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
    vk::ImageView view; // make these to pair
    vk::Sampler sampler;
    std::uint32_t idx;
  };
  struct SamplerArrayDescriptorItem {
    vk::DescriptorSetLayoutBinding binding;
    std::vector<std::pair<vk::ImageView, vk::Sampler>> views;
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

  void addDynamicUBO(const Buffer& dynamicUBO, vk::ShaderStageFlags shaderStage)
  {
    auto& item = m_uniformDynamicBindings.emplace_back();
    item.idx = m_idx++;
    item.binding.binding = item.idx;
    item.binding.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
    item.binding.descriptorCount = 1;
    item.binding.stageFlags = shaderStage;
    item.buffer = dynamicUBO.buffer();
    item.size = dynamicUBO.size();
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
  void addSamplerArray(const std::vector<std::pair<vk::ImageView, vk::Sampler>>&
          combinedSamplers)
  {
    auto& item = m_samplerArrayBindings.emplace_back();
    item.idx = m_idx++;
    item.binding.binding = item.idx;
    item.binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    item.binding.descriptorCount = combinedSamplers.size();
    item.binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
    for (const auto& sampler : combinedSamplers) {
      item.views.emplace_back(sampler.first, sampler.second);
    }
  }
  void addTexture(const Texture& texture)
  {
    addSampler(texture.view(), texture.sampler());
  }
  void addTextureArray(const std::vector<Texture>& textures)
  {
    std::vector<std::pair<vk::ImageView, vk::Sampler>> combinedSamplers;
    for (const auto& texture : textures) {
      combinedSamplers.emplace_back(texture.view(), texture.sampler());
    }
    addSamplerArray(combinedSamplers);
  }
  void generateLayout(const Device& device)
  {
    m_layout.resize(m_uniformBindings.size() + m_uniformDynamicBindings.size() +
                    m_samplerBindings.size() + m_samplerArrayBindings.size());
    for (const auto& binding : m_uniformBindings) {
      m_layout[binding.idx] = binding.binding;
    }
    for (const auto& binding : m_uniformDynamicBindings) {
      m_layout[binding.idx] = binding.binding;
    }
    for (const auto& binding : m_samplerBindings) {
      m_layout[binding.idx] = binding.binding;
    }
    for (const auto& binding : m_samplerArrayBindings) {
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
    for (const auto& ubo : m_uniformDynamicBindings) {
      for (std::size_t i{0u}; i < m_descriptorSets.size(); ++i) {
        vk::DescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = ubo.buffer;
        bufferInfo.offset = 0;
        // bufferInfo.range = ubo.size;
        bufferInfo.range = 256;

        vk::WriteDescriptorSet descriptorWrite{};

        descriptorWrite.dstSet = m_descriptorSets[i];
        descriptorWrite.dstBinding = ubo.binding.binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType =
            vk::DescriptorType::eUniformBufferDynamic;
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
    // for (const auto& sampler : m_samplerArrayBindings) {
    for (std::size_t s{0u}; s < m_samplerArrayBindings.size(); ++s) {
      const auto& sampler = m_samplerArrayBindings[s];
      for (std::size_t i{0u}; i < m_descriptorSets.size(); ++i) {
        std::vector<vk::DescriptorImageInfo> imageInfos;
        imageInfos.reserve(sampler.views.size());
        for (const auto& combinedSampler : sampler.views) {
          auto& imageInfo = imageInfos.emplace_back();
          imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
          imageInfo.imageView = combinedSampler.first;
          imageInfo.sampler = combinedSampler.second;
        }

        vk::WriteDescriptorSet descriptorWrite{};

        descriptorWrite.dstSet = m_descriptorSets[i];
        descriptorWrite.dstBinding = sampler.binding.binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType =
            vk::DescriptorType::eCombinedImageSampler;
        descriptorWrite.descriptorCount = imageInfos.size();
        descriptorWrite.pImageInfo = imageInfos.data();

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
  vk::UniqueDescriptorPool m_descriptorPool{};
  std::vector<vk::DescriptorSet> m_descriptorSets{};
  std::vector<UBODescriptorItem> m_uniformBindings;
  std::vector<UBODescriptorItem> m_uniformDynamicBindings;
  std::vector<SamplerDescriptorItem> m_samplerBindings;
  std::vector<SamplerArrayDescriptorItem> m_samplerArrayBindings;
  std::vector<vk::DescriptorSetLayoutBinding> m_layout;
};
