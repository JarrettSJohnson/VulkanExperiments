#pragma once

#include <glm/glm.hpp>
#include <random>

#include "Delaunay.hpp"
#include "PostProcess.hpp"
#include "Swapchain.hpp"

class ScreenShatter : public PostProcess
{
  // Use for transforms
  struct ShatterTriangle {
    glm::vec3 traj;
    glm::vec3 center;
    glm::vec3 vel;
    glm::vec3 rot;
    glm::vec3 rotVel;
    float scaleFactor;
  };
  struct ShatterTriangleTransform {
    glm::mat4 mat = glm::mat4(1.0f);
  };
  // Use for shader information
  struct ShatterTriangleVertex {
    glm::vec2 pos;
    static auto getBindingDescription()
    {
      std::vector<vk::VertexInputBindingDescription> bindingDescriptions(1);
      bindingDescriptions[0].binding = 0;
      bindingDescriptions[0].stride = sizeof(ShatterTriangleVertex);
      bindingDescriptions[0].inputRate = vk::VertexInputRate::eVertex;
      return bindingDescriptions;
    }
    static auto getAttributeDescriptions()
    {
      std::vector<vk::VertexInputAttributeDescription> attributeDescriptions(1);
      attributeDescriptions[0].binding = 0;
      attributeDescriptions[0].location = 0;
      attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
      attributeDescriptions[0].offset = offsetof(ShatterTriangleVertex, pos);
      return attributeDescriptions;
    }
  };

public:
  ScreenShatter(Device& device, const FramebufferAttachment& prevAttachment)
      : PostProcess{device, prevAttachment}, dynamic_ubo{device}
  {
    // START WITH 0% VEL AND WORK WAY UP TO 100% VEL IN A FEW FRAMES
    // ADD TIME TO PUSH CONSTANT --- TIME % 2000MS / 2000MS is the % of white to
    // show

    RandomCoordGenerator randPts{256};
    auto& pts = randPts.points();
    std::vector<double> delPts;
    for (const auto& pt : pts) {
      delPts.emplace_back(static_cast<double>(pt.x));
      delPts.emplace_back(static_cast<double>(pt.y));
    }
    delaunator::Delaunator d{delPts};
    std::vector<ShatterTriangleVertex> verts;
    verts.reserve(d.triangles.size() * 3);
    m_tris.reserve(d.triangles.size());
    const glm::vec3 quadCenter(0.0f, 0.0f, 0.0f);
    std::mt19937 mt{std::random_device{}()};
    std::uniform_real_distribution<float> dist{-20, 20};
    std::uniform_real_distribution<float> pct{0.5f, 1.0f};

    for (std::size_t i = 0; i < d.triangles.size(); i += 3) {
      auto& tri = m_tris.emplace_back();
      auto& v1 = verts.emplace_back(ShatterTriangleVertex{{d.coords[2 * d.triangles[i]],
              d.coords[2 * d.triangles[i] + 1]}});
      auto& v2 = verts.emplace_back(
          ShatterTriangleVertex{{d.coords[2 * d.triangles[i + 1]],
              d.coords[2 * d.triangles[i + 1] + 1]}});
      auto& v3 = verts.emplace_back(
          ShatterTriangleVertex{{d.coords[2 * d.triangles[i + 2]],
              d.coords[2 * d.triangles[i + 2] + 1]}});
      float centerX = (v1.pos.x + v2.pos.x + v3.pos.x) / 3.0f;
      float centerY = (v1.pos.y + v2.pos.y + v3.pos.y) / 3.0f;
      tri.center = glm::vec3(centerX, centerY, 0.0f);
      auto distFromCenter = tri.center - quadCenter;
      auto dir = distFromCenter / glm::length(distFromCenter);
      auto spd = 1.0f / glm::length(distFromCenter);
      tri.vel = dir * spd * 0.2f * pct(mt);
      tri.vel.z = 0;
      auto scale = 0.01;
      tri.rotVel =
          glm::vec3(dist(mt) * scale, dist(mt) * scale, dist(mt) * scale) * spd;
      tri.scaleFactor = dist(mt); // random for now--scale by center
    }

    transforms.resize(m_tris.size());
    auto deviceAlignment = device.m_physicalDevice.getProperties()
                               .limits.minUniformBufferOffsetAlignment;
    std::size_t uniformSize = sizeof(ShatterTriangleTransform);
    auto dynamicAlignment =
        (uniformSize / deviceAlignment) * deviceAlignment +
        ((uniformSize % deviceAlignment) > 0 ? deviceAlignment : 0);
    auto dynamicUboSize = dynamicAlignment * m_tris.size();
    dynamic_ubo.generate(vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        dynamicUboSize);

    vk::DeviceSize vertexbufferSize =
        sizeof(ShatterTriangleVertex) * verts.size();

    Buffer stagingBuffer{device};
    stagingBuffer.generate(vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        vertexbufferSize);

    stagingBuffer.mapCopy(verts.data(), vertexbufferSize);

    m_vertexBuffer = Buffer{device};
    m_vertexBuffer.generate(vk::BufferUsageFlagBits::eTransferDst |
                                vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal, vertexbufferSize);

    VKUtil::copyBuffer(device, *stagingBuffer.m_buffer,
        *m_vertexBuffer.m_buffer, vertexbufferSize);

    m_sampler = VKUtil::createTextureSampler(device, 1);
    m_descriptorSet.addDynamicUBO(
        dynamic_ubo, vk::ShaderStageFlagBits::eVertex);
    m_descriptorSet.addSampler(*prevAttachment.imageView, *m_sampler);
    m_descriptorSet.generateLayout(device);
    m_descriptorSet.generatePool(device);

    vk::Extent2D extent{
        prevAttachment.extent.width, prevAttachment.extent.height};
    createRenderPass(extent, prevAttachment.description.format);

    m_pipelineLayout = PipelineLayout{
        device, m_descriptorSet.layout() /*, pushConstantRange*/};
    m_pipeline = Pipeline{device, extent, vk::SampleCountFlagBits::e1};
    m_pipeline.addVertexDescription<ShatterTriangleVertex>();
    Shader vertShader{
        device, "../assets/shatter.vert.spv", Shader::ShaderType::VERTEX};
    Shader fragShader{
        device, "../assets/shatter.frag.spv", Shader::ShaderType::FRAGMENT};
    m_pipeline.generate(
        device, m_pipelineLayout, m_renderPass, vertShader, fragShader);

    dynamic_ubo.map();
  };
  const auto& triangles() const { return m_tris; }

  void attachFramebuffer(Device& device, const Swapchain& tempSwapchain)
  {
    m_framebuffers.clear();
    m_framebuffers.emplace_back(device, m_renderPass);
    m_framebuffers.emplace_back(device, m_renderPass);

    for (std::size_t i{0u}; i < m_framebuffers.size(); ++i) {
      auto& framebuffer = m_framebuffers[i];
      framebuffer.generate(tempSwapchain.imageView(i));
    }
  }

  void update(float dt)
  {
    //auto proj = glm::perspective(glm::radians(20.0f), 1024 / (float) 576, 0.1f,
    //    20.0f);
    auto proj = glm::perspective(glm::radians(30.0f), 1.0f, 0.1f,
        20.0f);
    proj[1][1] *= 1;
    auto view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
        glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
   // proj = view = glm::mat4(1.0f);
    currDuration += dt;
    for (unsigned int i = 0;
         i < m_tris.size() && currDuration >= shatterDelay; ++i) {
      auto& transform = transforms[i];
      auto& tri = m_tris[i];

	  if (currDuration > shatterDelay + pushZDelay) {
        tri.vel.z += 0.2;
      }
     // tri.traj.z += tri.vel.z * 0.01f;
      tri.traj += tri.vel * 0.01f;
     // tri.traj.z += 0.005;
      tri.rot += tri.rotVel * 4.0f;
      transform.mat = glm::mat4(1.0f);
      continue;
      glm::mat4 translate = glm::translate(glm::mat4(1.0f), -tri.center);
      glm::quat qPitch =
          glm::angleAxis(glm::radians(tri.rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
      glm::quat qYaw =
          glm::angleAxis(glm::radians(tri.rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
      glm::quat qRoll =
          glm::angleAxis(glm::radians(tri.rot.z), glm::vec3(0.0f, 0.0f, 1.0f));

      glm::quat orientation = glm::normalize(qRoll * qPitch * qYaw);
      glm::mat4 rotate = glm::mat4_cast(orientation);
      auto translate2 = glm::translate(glm::mat4(1.0f), tri.center);
      auto translate3 = glm::translate(glm::mat4(1.0f), tri.traj);
      transform.mat = proj * view * translate3 * translate2 * rotate * translate;
    }
    dynamic_ubo.copyData(transforms.data(),
        sizeof(ShatterTriangleTransform) * transforms.size());
  }

  ~ScreenShatter() { dynamic_ubo.unmap(); }

private:
  float shatterDelay{1000};
  float pushZDelay{1500};
  float currDuration{};
  std::vector<ShatterTriangle> m_tris;
  std::vector<ShatterTriangleTransform> transforms;
  Buffer dynamic_ubo;
  // uniform velocity
  float m_velocity{};
};
