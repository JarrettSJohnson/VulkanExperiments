#include <cstdint>
#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION

#include "Model.hpp"

Model::Model(Device& device, const std::filesystem::path& filename)
{
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;
  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
          filename.string().c_str())) {
    throw std::runtime_error(warn + err);
  }
  std::unordered_map<Vertex, uint32_t> uniqueVertices{};

  for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
      Vertex vertex = {};

      vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
          attrib.vertices[3 * index.vertex_index + 1],
          attrib.vertices[3 * index.vertex_index + 2]};

      vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
          1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

      vertex.normal = {attrib.normals[3 * index.normal_index + 0],
          attrib.normals[3 * index.normal_index + 1],
          attrib.normals[3 * index.normal_index + 2]};

      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
        m_vertices.push_back(vertex);
      }

      m_indices.push_back(uniqueVertices[vertex]);
    }

    createVertexBuffers(device);
    createIndexBuffers(device);
  }
}
