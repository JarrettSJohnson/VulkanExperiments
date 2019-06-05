#pragma once

#include <filesystem>

#include "Buffer.hpp"
#include "DescriptorSet.hpp"
#include "Device.hpp"
#include "Framebuffer.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"
#include "Shader.hpp"

class PostProcess
{
public:
  PostProcess(Device& device, const FramebufferAttachment& prevAttachment)
      : m_renderPass{device}
  {
  }
  vk::Buffer vertexBuffer() const { return m_vertexBuffer.buffer(); }
  vk::RenderPass renderpass() const { return m_renderPass.renderpass(); }
  vk::Pipeline pipeline() const { return m_pipeline.pipeline(); }
  vk::PipelineLayout pipelineLayout() const
  {
    return m_pipelineLayout.layout();
  }
  const std::vector<Framebuffer>& framebuffer() const { return m_framebuffers; }
  DescriptorSet m_descriptorSet;

protected:
  Buffer m_vertexBuffer;

  vk::UniqueSampler m_sampler;

  RenderPass m_renderPass;
  ////std::optional<Framebuffer> framebuffer;
  // CHANGE THIS LATER --SHOULD ONLY USE ONE FB
  std::vector<Framebuffer> m_framebuffers;
  PipelineLayout m_pipelineLayout;
  Pipeline m_pipeline;

  void createSamplerAndDescriptorSet(
      const Device& device, vk::ImageView imageView)
  {
    m_sampler = VKUtil::createTextureSampler(device, 1);
    m_descriptorSet.addSampler(imageView, *m_sampler);
    m_descriptorSet.generateLayout(device);
    m_descriptorSet.generatePool(device);
  }

  void createRenderPass(vk::Extent2D extent, vk::Format format)
  {
    FrameBufferAttachmentInfo defaultColorAttachInfo{};
    defaultColorAttachInfo.extent = extent;
    defaultColorAttachInfo.format = format;
    defaultColorAttachInfo.numSamples = vk::SampleCountFlagBits::e1;
    defaultColorAttachInfo.usage =
        vk::ImageUsageFlagBits::eTransientAttachment |
        vk::ImageUsageFlagBits::eColorAttachment;
    defaultColorAttachInfo.presented = true;
    m_renderPass.addAttachment(defaultColorAttachInfo);

    m_renderPass.generate();
  }
};
