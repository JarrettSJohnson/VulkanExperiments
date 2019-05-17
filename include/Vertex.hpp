#pragma once

#include <vulkan/vulkan.hpp>
#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>


struct Vertex{
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 texCoord;


  static std::array<vk::VertexInputBindingDescription, 1> getBindingDescription(){
    std::array<vk::VertexInputBindingDescription, 1> bindingDescriptions{};
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(Vertex);
    bindingDescriptions[0].inputRate = vk::VertexInputRate::eVertex;
    return bindingDescriptions;
  }
  static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions(){
    std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[1].offset = offsetof(Vertex, normal);
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
    return attributeDescriptions;
  }

  bool operator==(const Vertex& other) const
  {
    return pos == other.pos && normal == other.normal &&
           texCoord == other.texCoord;
  }
};

namespace std
{
template <> struct hash<Vertex> {
  size_t operator()(Vertex const& vertex) const
  {
    return ((hash<glm::vec3>()(vertex.pos) ^
                (hash<glm::vec3>()(vertex.normal) << 1)) >>
               1) ^
           (hash<glm::vec2>()(vertex.texCoord) << 1);
  }
};
} // namespace std