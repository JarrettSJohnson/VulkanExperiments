#include "Device.hpp"

#include "VKUtil.hpp"

Device::Device(vk::PhysicalDevice physicalDevice)
    : m_physicalDevice{physicalDevice},
      m_physicalDeviceProperties{m_physicalDevice.getProperties()},
      m_physicalDeviceMemoryProperties{m_physicalDevice.getMemoryProperties()}
{
  auto queueFamilyProperties = m_physicalDevice.getQueueFamilyProperties();

  vk::DeviceQueueCreateInfo queueCreateInfo{};
  // queueCreateInfo.queueFamilyIndex = m_familyIndex.front(); // TODO: change
  queueCreateInfo.queueFamilyIndex = 0;
  queueCreateInfo.queueCount = 1;
  float priorities = 1.0f;
  queueCreateInfo.pQueuePriorities = &priorities;

  vk::PhysicalDeviceFeatures deviceFeatures{};
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  vk::DeviceCreateInfo deviceCreateInfo{};
  deviceCreateInfo.queueCreateInfoCount = 1;
  deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
  deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

  const std::vector<const char*> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  deviceCreateInfo.enabledExtensionCount =
      static_cast<std::uint32_t>(deviceExtensions.size());
  deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

  m_device = m_physicalDevice.createDeviceUnique(deviceCreateInfo);

  m_graphicsQueue = m_device->getQueue(m_familyIndices.graphics, 0);
  m_transferQueue = m_device->getQueue(m_familyIndices.transfer, 0);
  // m_computeQueue = m_device->getQueue(m_familyIndices.graphics, 0);
}

Device::operator vk::Device()
{
  return *m_device;
};

vk::Device Device::device() const
{
  return *m_device;
}
