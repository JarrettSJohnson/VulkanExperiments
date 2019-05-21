#pragma once

#include <vector>

#include "Model.hpp"

struct Cube : public Model {
  inline static float cubeVert[36][8]={// clang-format off
    // bottom
    // TODO: WTF LAYOUT IS THIS
    -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,  1.0f, -1.0f, -1.0f,  0.0f,
    -1.0f,  0.0f,  1.0f,  0.0f, -1.0f, -1.0f,  1.0f, 0.0f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -1.0f, -1.0f,  0.0f, -1.0f, 0.0f,   1.0f,  0.0f, 1.0f, -1.0f,  1.0f,  0.0f, -1.0f, 0.0f,
     1.0f,  1.0f, -1.0f, -1.0f, 1.0f,   0.0f, -1.0f, 0.0f,  0.0f,  1.0f,

    // top
    -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,

    // front
    -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f,
    1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,

    // back
    -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, -1.0f, 0.0f,
    0.0f, -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f,
    -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,

    // left
    -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
    0.0f, 0.0f, 1.0f, 0.0f, -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f,
    0.0f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

    // right
    1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, -1.0f,
    1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};

  // clang-format on
  Cube(Device& device)
  {
    std::unordered_map<Vertex, uint32_t> uniqueVertices{};
   for (unsigned int i = 0; i < 36; i++) {
      Vertex vert{};
     vert.pos = glm::vec3(cubeVert[i][0], cubeVert[i][1], cubeVert[i][2]);
     vert.normal = glm::vec3(cubeVert[i][5], cubeVert[i][6], cubeVert[i][7]);
     vert.texCoord = glm::vec2(cubeVert[i][3], cubeVert[i][4]);

	 if (uniqueVertices.count(vert) == 0) {
       uniqueVertices[vert] = static_cast<uint32_t>(m_vertices.size());
           m_vertices.push_back(vert);
     }
         m_indices.push_back(uniqueVertices[vert]);
    }
   createVertexBuffers(device);
   createIndexBuffers(device);
  }
};