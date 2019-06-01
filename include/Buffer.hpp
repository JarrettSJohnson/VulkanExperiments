#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include "Device.hpp"

#include "Texture.hpp"
#include "VKUtil.hpp"

struct Buffer {
  Device* m_device{};
  vk::UniqueBuffer m_buffer{};
  vk::UniqueDeviceMemory m_memory{};
  vk::DeviceSize m_size{};
  vk::DeviceSize m_alignment{};
  void* m_mapped{};

  vk::BufferUsageFlags m_usageFlags{};
  vk::MemoryPropertyFlags m_memoryPropertyFlags{};

  Buffer(Device& device) : m_device{&device} {}

  //make into constructor?
  void generate(vk::BufferUsageFlags flags,
      vk::MemoryPropertyFlags memPropertyFlags, vk::DeviceSize size)
  {
    std::tie(m_buffer, m_memory) =
        VKUtil::createBuffer(*m_device, size, flags, memPropertyFlags);
  }

  void map(vk::DeviceSize size = VK_WHOLE_SIZE, vk::DeviceSize offset = 0)
  {
    m_mapped = m_device->device().mapMemory(*m_memory, offset, size);
  };
  void unmap()
  {
    if (m_mapped) {
      m_device->device().unmapMemory(*m_memory);
    }
    m_mapped = nullptr;
  }
  void copyData(void* src, vk::DeviceSize size)
  {
    std::memcpy(m_mapped, src, size);
  }
  void flush() {
    vk::MappedMemoryRange mappedRange{};
    mappedRange.memory = *m_memory;
    mappedRange.size = VK_WHOLE_SIZE;
    m_device->device().flushMappedMemoryRanges(1, &mappedRange);
  }
};