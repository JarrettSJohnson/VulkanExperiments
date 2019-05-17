#pragma once

#include <cstdint>

#include <vulkan/vulkan.hpp>

struct QueueFamilyIndices {
  std::uint32_t graphics;
  std::uint32_t compute;
  std::uint32_t transfer;
};

struct Device {
  Device() = default;
  Device(vk::PhysicalDevice physicalDevice);
  operator vk::Device();
  vk::Device device() const;

  vk::PhysicalDevice m_physicalDevice{};
  vk::UniqueDevice m_device{};

  QueueFamilyIndices m_familyIndices{};
  vk::Queue m_graphicsQueue{};
  vk::Queue m_transferQueue{};
  //vk::queue m_computeQueue{};

  vk::PhysicalDeviceProperties m_physicalDeviceProperties{};
  vk::PhysicalDeviceFeatures m_features{};
  vk::PhysicalDeviceMemoryProperties m_physicalDeviceMemoryProperties{};
  std::vector<vk::QueueFamilyProperties> queueFamilyProperties{};
  std::vector<std::string> supportedExtentions;
  vk::UniqueCommandPool m_commandPool{};

  vk::SampleCountFlagBits m_msaaSamples;
};