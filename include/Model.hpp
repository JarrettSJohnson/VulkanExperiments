#pragma once

#include <filesystem>
#include <optional>
#include <utility>

#include <tiny_obj_loader.h>

#include "Buffer.hpp"
#include "Device.hpp"
#include "VKUtil.hpp"
#include "Vertex.hpp"

class Mesh
{
public:
  Mesh() = default;
  // Model(Device& device, const std::filesystem::path& filename);
  Mesh(Device& device, std::vector<Vertex>&& verts,
      std::vector<std::uint32_t>&& inds)
      : m_vertices{std::move(verts)}, m_indices{std::move(inds)}
  {
    createVertexBuffers(device);
    createIndexBuffers(device);
  }
  const auto& vertices() const { return m_vertices; };
  const auto& indices() const { return m_indices; };
  auto numIndices() const
  {
    return static_cast<std::uint32_t>(m_indices.size());
  }

  const auto vertexBuffer() const { return *m_vertexBuffer->m_buffer; }
  const auto indexBuffer() const { return *m_indexBuffer->m_buffer; }

protected:
  std::vector<Vertex> m_vertices;
  std::vector<std::uint32_t> m_indices;

  std::optional<Buffer> m_vertexBuffer;
  std::optional<Buffer> m_indexBuffer;

  void createVertexBuffers(Device& device)
  {
    vk::DeviceSize bufferSize = sizeof(m_vertices.front()) * m_vertices.size();

    Buffer stagingBuffer{device};
    stagingBuffer.generate(vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        bufferSize);

    stagingBuffer.mapCopy(m_vertices.data(), bufferSize);

    m_vertexBuffer.emplace(device);
    m_vertexBuffer->generate(vk::BufferUsageFlagBits::eTransferDst |
                                 vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal, bufferSize);

    VKUtil::copyBuffer(
        device, *stagingBuffer.m_buffer, *m_vertexBuffer->m_buffer, bufferSize);
  }

  void createIndexBuffers(Device& device)
  {
    vk::DeviceSize bufferSize = sizeof(m_indices.front()) * m_indices.size();
    Buffer stagingBuffer{device};
    stagingBuffer.generate(vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        bufferSize);

    stagingBuffer.mapCopy(m_indices.data(), bufferSize);

    m_indexBuffer.emplace(device);
    m_indexBuffer->generate(vk::BufferUsageFlagBits::eTransferDst |
                                vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal, bufferSize);

    VKUtil::copyBuffer(
        device, *stagingBuffer.m_buffer, *m_indexBuffer->m_buffer, bufferSize);
  }
};

class Model
{
public:
  Model() = default;
  Model(Device& device, const std::filesystem::path& filename);
  const auto& meshes() const { return m_meshes; }

private:
  std::vector<Mesh> m_meshes;
};
