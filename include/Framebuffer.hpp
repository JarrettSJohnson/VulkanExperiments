#pragma once

#include "Device.hpp"
#include "VKUtil.hpp"

class Attachment
{
public:
  enum class AttachmentType { COLOR, DEPTHSTENCIL };

  Attachment(AttachmentType aType, vk::Format format,
      vk::SampleCountFlagBits numSamples = vk::SampleCountFlagBits::e1)
      : m_aType{aType}
  {
    m_attachment.format = format;
    m_attachment.samples = numSamples;
    m_attachment.loadOp = vk::AttachmentLoadOp::eClear;
    m_attachment.storeOp = vk::AttachmentStoreOp::eStore;
    m_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    m_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    m_attachment.initialLayout = vk::ImageLayout::eUndefined;
    m_attachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
  }

  vk::AttachmentReference reference() const
  {
    vk::AttachmentReference reference{};
    reference.layout = m_attachment.finalLayout;
    return reference;
  }

private:
  const AttachmentType m_aType;
  vk::AttachmentDescription m_attachment{};
};

class FrameBufferAttachment
{
public:
  FrameBufferAttachment(Device& device, vk::ImageLayout layout,
      vk::Format format, vk::ImageUsageFlagBits usageFlagBits,
      vk::ImageAspectFlagBits aspectFlagBits, vk::Extent2D extent,
      vk::SampleCountFlagBits numSamples = vk::SampleCountFlagBits::e1)
      : m_layout{layout}, m_format{format}
  {
    vk::Extent3D fboExtent{extent.width, extent.height, 1};
    std::tie(m_image, m_memory) = VKUtil::createImage(device, fboExtent, 1,
        numSamples, m_format, vk::ImageTiling::eOptimal, usageFlagBits,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    m_imageView = VKUtil::createImageView(
        device.device(), *m_image, m_format, aspectFlagBits, 1);

    VKUtil::transitionImageLayout(
        device, *m_image, m_format, vk::ImageLayout::eUndefined, m_layout, 1);
  }

  vk::ImageView imageView() const { return *m_imageView; }
  std::pair<int, int> size() const
  {
    return std::make_pair(m_extent.width, m_extent.height);
  }

private:
  vk::ImageLayout m_layout{};
  vk::Format m_format{};
  vk::Extent3D m_extent{};
  vk::UniqueImage m_image{};
  vk::UniqueImageView m_imageView{};
  vk::UniqueDeviceMemory m_memory{};
};

class Framebuffer
{
public:
  Framebuffer(vk::RenderPass renderPass) : m_renderPass{renderPass} {}

  void addAttachment(FrameBufferAttachment&& attachment)
  {
    m_attachments.push_back(std::move(attachment));
  }

  vk::UniqueFramebuffer generateFramebuffer(Device& device)
  {
    std::vector<vk::ImageView> m_attachmentViews;
    for (const auto& attachment : m_attachments) {
      m_attachmentViews.push_back(attachment.imageView());
    }
    auto [width, height] = m_attachments.front().size();

    vk::FramebufferCreateInfo framebufferCreateInfo{};
    framebufferCreateInfo.renderPass = m_renderPass;
    framebufferCreateInfo.attachmentCount =
        static_cast<std::uint32_t>(m_attachmentViews.size());
    framebufferCreateInfo.pAttachments = m_attachmentViews.data();
    framebufferCreateInfo.width = width;
    framebufferCreateInfo.height = height;
    framebufferCreateInfo.layers = 1;
    return device.device().createFramebufferUnique(framebufferCreateInfo);
  }

  void clear() { m_attachments.clear(); }

private:
  std::vector<FrameBufferAttachment> m_attachments;
  vk::RenderPass m_renderPass{};
};