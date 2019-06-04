#pragma once

#include <glm/glm.hpp>
#include <random>

#include "Delaunay.hpp"
#include "PostProcess.hpp"

class ScreenShatter : public PostProcess
{
  struct Triangle {
    glm::vec2 traj;
    float rotX;
    float rotY;
    float rotZ;
    float scaleFactor;
  };

public:
  ScreenShatter(Device& device)
      : PostProcess{
            device, "../assets/shatter.vert.spv", "../assets/shatter.frag.spv"}
  {
    RandomCoordGenerator randPts{100};
    const auto& pts = randPts.points();
    std::vector<double> delPts;
    for (const auto& pt : pts) {
      delPts.emplace_back(static_cast<double>(pt.x));
      delPts.emplace_back(static_cast<double>(pt.y));
    }
    delaunator::Delaunator d{delPts};
    std::vector<std::array<glm::vec2, 3>> verts;
    verts.reserve(d.triangles.size());
    m_tris.reserve(d.triangles.size());
    const glm::vec2 quadCenter(0.5f, 0.5f);
    std::mt19937 mt{std::random_device{}()};
    std::uniform_real_distribution<float> dist{-0.5, 0.5};

    for (std::size_t i = 0; i < d.triangles.size(); i += 3) {
      auto& tri = m_tris.emplace_back();
      auto& vert = verts.emplace_back();
      vert[0] = glm::vec2(
          d.coords[2 * d.triangles[i]], d.coords[2 * d.triangles[i] + 1]);
      vert[1] = glm::vec2(d.coords[2 * d.triangles[i + 1]],
          d.coords[2 * d.triangles[i + 1] + 1]);
      vert[2] = glm::vec2(d.coords[2 * d.triangles[i + 2]],
          d.coords[2 * d.triangles[i + 2] + 1]);
      float centerX = (vert[0].x + vert[1].x + vert[2].x) / 3.0f;
      float centerY = (vert[0].y + vert[1].y + vert[2].y) / 3.0f;
      glm::vec2 center(centerX, centerY);
      tri.traj = center - quadCenter;
      tri.rotX = dist(mt);
      tri.rotY = dist(mt);
      tri.rotZ = dist(mt);
      tri.scaleFactor = dist(mt); // random for now--scale by center
    }
    // CREATE VERTEX BUFFER
    vk::DeviceSize bufferSize = sizeof(glm::vec2) * verts.size();

    Buffer stagingBuffer{device};
    stagingBuffer.generate(vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        bufferSize);

    stagingBuffer.mapCopy(verts.data(), bufferSize);

    m_vertexBuffer = Buffer{device};
    m_vertexBuffer.generate(vk::BufferUsageFlagBits::eTransferDst |
                                vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal, bufferSize);

    VKUtil::copyBuffer(
        device, *stagingBuffer.m_buffer, *m_vertexBuffer.m_buffer, bufferSize);
  };
  const auto& triangles() const { return m_tris; }

private:
  float transformDelay{100};
  float totalDuration{2000};
  float currDuration{};
  std::vector<Triangle> m_tris;
  // uniform velocity
  float m_velocity{};
};
