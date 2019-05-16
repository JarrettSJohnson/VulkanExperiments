#pragma once


#include <filesystem>
#include <utility>

#include <tiny_obj_loader.h>

#include "Device.hpp"
#include "Vertex.hpp"
#include "VKUtil.hpp"

class Model
{
public:
  Model() = default;
  Model(Device& device, const std::filesystem::path& filename);

  const auto& vertices() const { return m_vertices; };
  const auto& indices() const { return m_indices; };

  const auto vertexBuffer() const { return *m_vertexBuffer; }
  const auto indexBuffer() const { return *m_indexBuffer; }

private:
  std::vector<Vertex> m_vertices;
  std::vector<std::uint32_t> m_indices;

  vk::UniqueBuffer m_vertexBuffer{};
  vk::UniqueDeviceMemory m_vertexBufferMemory{};
  vk::UniqueBuffer m_indexBuffer{};
  vk::UniqueDeviceMemory m_indexBufferMemory{};

  void createVertexBuffers(Device& device) {
    vk::DeviceSize bufferSize = sizeof(m_vertices.front()) * m_vertices.size();
    auto [stagingBuffer, stagingBufferMemory] =
        VKUtil::createBuffer(device, bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent);

    void* data =
        device.device().mapMemory(*stagingBufferMemory, 0, bufferSize);
    std::memcpy(data, m_vertices.data(), static_cast<std::size_t>(bufferSize));
    device.device().unmapMemory(*stagingBufferMemory);

    std::tie(m_vertexBuffer, m_vertexBufferMemory) = VKUtil::createBuffer(device, bufferSize,
        vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    VKUtil::copyBuffer(device, *stagingBuffer, *m_vertexBuffer, bufferSize);
  }

  void createIndexBuffers(Device& device) {
    vk::DeviceSize bufferSize = sizeof(m_indices.front()) * m_indices.size();
    auto [stagingBuffer, stagingBufferMemory] =
        VKUtil::createBuffer(device, bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent);

    void* data =
        device.device().mapMemory(*stagingBufferMemory, 0, bufferSize);
    std::memcpy(data, m_indices.data(), static_cast<std::size_t>(bufferSize));
    device.device().unmapMemory(*stagingBufferMemory);

    std::tie(m_indexBuffer, m_indexBufferMemory) = VKUtil::createBuffer(device, bufferSize,
        vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal);
    VKUtil::copyBuffer(device, *stagingBuffer, *m_indexBuffer, bufferSize);
  }
};