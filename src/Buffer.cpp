#include "Buffer.hpp"
#include "VKUtil.hpp"

// make into constructor?
void Buffer::generate(vk::BufferUsageFlags flags,
    vk::MemoryPropertyFlags memPropertyFlags, vk::DeviceSize size)
{
  m_size = size;
  std::tie(m_buffer, m_memory) =
      VKUtil::createBuffer(*m_device, size, flags, memPropertyFlags);
}
