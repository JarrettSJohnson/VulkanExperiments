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
    glm::vec2 traj;
    float rotX;
    float rotY;
    float rotZ;
    float scaleFactor;
  };
  struct ShatterTriangleTransform alignas(256) {
    glm::mat4 mat = glm::mat4(1.0f);
  };
  // Use for shader information
  struct ShatterTriangleVertex {
    glm::vec2 pos;
    static auto getBindingDescription()
    {
      std::vector<vk::VertexInputBindingDescription> bindingDescriptions(1);
      bindingDescriptions[0].binding = 0;
      bindingDescriptions[0].stride = sizeof(ShatterTriangle);
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
    RandomCoordGenerator randPts{120};
    const auto& pts = randPts.points();
    std::vector<double> delPts;
    for (const auto& pt : pts) {
      delPts.emplace_back(static_cast<double>(pt.x));
      delPts.emplace_back(static_cast<double>(pt.y));
    }
    delaunator::Delaunator d{delPts};
    std::vector<ShatterTriangleVertex> verts;
    verts.reserve(d.triangles.size());
    m_tris.reserve(d.triangles.size());
    const glm::vec2 quadCenter(0.5f, 0.5f);
    std::mt19937 mt{std::random_device{}()};
    std::uniform_real_distribution<float> dist{-0.5, 0.5};

    for (std::size_t i = 0; i < d.triangles.size(); i += 3) {
      auto& tri = m_tris.emplace_back();
      auto& v1 = verts.emplace_back(ShatterTriangleVertex{
          {d.coords[2 * d.triangles[i]], d.coords[2 * d.triangles[i] + 1]}});
      auto& v2 = verts.emplace_back(
          ShatterTriangleVertex{{d.coords[2 * d.triangles[i + 1]],
              d.coords[2 * d.triangles[i + 1] + 1]}});
      auto& v3 = verts.emplace_back(
          ShatterTriangleVertex{{d.coords[2 * d.triangles[i + 2]],
              d.coords[2 * d.triangles[i + 2] + 1]}});
      float centerX = (v1.pos.x + v2.pos.x + v3.pos.x) / 3.0f;
      float centerY = (v1.pos.y + v2.pos.y + v3.pos.y) / 3.0f;
      glm::vec2 center(centerX, centerY);
      tri.traj = center - quadCenter;
      tri.rotX = dist(mt);
      tri.rotY = dist(mt);
      tri.rotZ = dist(mt);
      tri.scaleFactor = dist(mt); // random for now--scale by center
    }

    // Redundant--3 verts in triangle should share the same transform. Change
    // later
    transforms.resize(verts.size());
    /*const auto transformDataSize =
        sizeof(ShatterTriangleTransform) * verts.size();*/
    auto deviceAlignment = device.m_physicalDevice.getProperties()
                               .limits.minUniformBufferOffsetAlignment;
    std::size_t uniformSize = sizeof(ShatterTriangleTransform);
    auto dynamicAlignment =
        (uniformSize / deviceAlignment) * deviceAlignment +
        ((uniformSize % deviceAlignment) > 0 ? deviceAlignment : 0);
    dynamic_ubo.generate(vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        dynamicAlignment * verts.size());

    vk::DeviceSize bufferSize = sizeof(ShatterTriangleVertex) * verts.size();

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
    m_framebuffers.emplace_back(device, m_renderPass);
    m_framebuffers.emplace_back(device, m_renderPass);

    for (std::size_t i{0u}; i < m_framebuffers.size(); ++i) {
      auto& framebuffer = m_framebuffers[i];
      framebuffer.generate(tempSwapchain.imageView(i));
    }
  }

  void update()
  {
    /**
     * TESTING
     */
    for (auto& transform : transforms) {
      // transform.mat =
      //    glm::translate(transform.mat, glm::vec3(0.01f, 0.01f, 0.0f));
    }
    // dynamic_ubo.mapCopy(transforms.data(),
    //    sizeof(ShatterTriangleTransform) * transforms.size());
    dynamic_ubo.copyData(transforms.data(),
        sizeof(ShatterTriangleTransform) * transforms.size());
  }
  ~ScreenShatter() { dynamic_ubo.unmap(); }

private:
  float transformDelay{100};
  float totalDuration{2000};
  float currDuration{};
  std::vector<ShatterTriangle> m_tris;
  // std::vector<UBO<ShatterTriangleTransform>> dynamic_ubo;
  std::vector<ShatterTriangleTransform> transforms;
  Buffer dynamic_ubo; // big pool of data
  // uniform velocity
  float m_velocity{};
};
