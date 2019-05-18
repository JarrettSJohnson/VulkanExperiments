#pragma once

#include "Device.hpp"
#include "VKUtil.hpp"

/*class Attachment
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

  const vk::AttachmentDescription attachment() const { return m_attachment; }

private:
};*/

struct FrameBufferAttachmentInfo {
  std::uint32_t width, height;
  std::uint32_t layerCount;
  vk::Format format;
  vk::ImageUsageFlags usage;
  vk::SampleCountFlagBits numSamples;
};

struct FramebufferAttachment {
  vk::AttachmentDescription description{};
  vk::ImageLayout layout{};
  vk::Format format{};
  vk::Extent3D extent{};
  vk::UniqueImage image{};
  vk::UniqueImageView imageView{};
  vk::UniqueDeviceMemory memory{};
};

/*class FrameBufferAttachment
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
  vk::AttachmentDescription m_attachment{};
  vk::ImageLayout m_layout{};
  vk::Format m_format{};
  vk::Extent3D m_extent{};
  vk::UniqueImage m_image{};
  vk::UniqueImageView m_imageView{};
  vk::UniqueDeviceMemory m_memory{};
};*/

class Framebuffer
{
public:
  Framebuffer(Device& device, vk::RenderPass renderPass)
      : m_device{device}, m_renderPass{renderPass}
  {
  }

  // void addAttachment(FrameBufferAttachment&& attachment)
  //{
  // m_attachments.push_back(std::move(attachment));
  //}

  FramebufferAttachment& addAttachment(FrameBufferAttachmentInfo fbAttInfo)
  {
    auto& attachment = m_attachments.emplace_back();

    attachment.format = fbAttInfo.format;

    vk::ImageAspectFlags aspectFlag{};

    if (fbAttInfo.usage & vk::ImageUsageFlagBits::eColorAttachment) {
      aspectFlag = vk::ImageAspectFlagBits::eColor;
    } else if (fbAttInfo.usage &
               vk::ImageUsageFlagBits::eDepthStencilAttachment) {
      if (VKUtil::hasDepthComponent(attachment.format)) {
        aspectFlag = vk::ImageAspectFlagBits::eDepth;
      }

      if (VKUtil::hasStencilComponent(attachment.format)) {
        aspectFlag = vk::ImageAspectFlagBits::eStencil;
      }
    }

    int miplevels = 1;
    vk::Extent3D fboExtent{fbAttInfo.width, fbAttInfo.height, 1};
    std::tie(attachment.image, attachment.memory) =
        VKUtil::createImage(m_device, fboExtent, miplevels,
            fbAttInfo.numSamples,
            attachment.format, vk::ImageTiling::eOptimal, fbAttInfo.usage,
            vk::MemoryPropertyFlagBits::eDeviceLocal);

    attachment.imageView = VKUtil::createImageView(
        m_device.device(), *attachment.image, attachment.format, aspectFlag, 1);

    attachment.description.samples = vk::SampleCountFlagBits::e1;
    attachment.description.loadOp = vk::AttachmentLoadOp::eClear;
    attachment.description.storeOp =
        (fbAttInfo.usage & vk::ImageUsageFlagBits::eSampled)
            ? vk::AttachmentStoreOp::eStore
            : vk::AttachmentStoreOp::eDontCare;
    attachment.description.format = fbAttInfo.format;
    attachment.description.initialLayout = vk::ImageLayout::eUndefined;
    if (VKUtil::hasDepthComponent(attachment.format) ||
        VKUtil::hasStencilComponent(attachment.format)) {
      attachment.description.finalLayout =
          vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal;
    } else {
      attachment.description.finalLayout =
          vk::ImageLayout::eShaderReadOnlyOptimal;
    }
    return attachment;
  }

  /* vk::UniqueFramebuffer generateFramebuffer(Device& device)
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
   }*/

  void generate()
  {

    std::vector<vk::AttachmentReference> colorReferences;
    vk::AttachmentReference depthReference;
    bool hasDepth = false;
    bool hasStencil = false;

    std::uint32_t attIdx{0u};
    for (auto& attachment : m_attachments) {
      if (VKUtil::hasDepthStencilComponent(attachment.format)) {
        depthReference.attachment = attIdx;
        depthReference.layout =
            vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal;
        hasDepth = true;
      } else {
        colorReferences.emplace_back(
            attIdx, vk::ImageLayout::eColorAttachmentOptimal);
      }
      attIdx++;
    }

    vk::SubpassDescription subpassDescription{};
    subpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    std::vector<vk::AttachmentDescription> attachments;
    for (auto& attachment : m_attachments) {
      attachments.push_back(attachment.description);
    }

    std::array<vk::SubpassDependency, 2> subpassDependency{};

    subpassDependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency[0].dstSubpass = 0;
    subpassDependency[0].srcStageMask =
        vk::PipelineStageFlagBits::eBottomOfPipe;
    subpassDependency[0].dstStageMask =
        vk::PipelineStageFlagBits::eColorAttachmentOutput;
    subpassDependency[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
    subpassDependency[0].dstAccessMask =
        vk::AccessFlagBits::eColorAttachmentRead |
        vk::AccessFlagBits::eColorAttachmentWrite;
    subpassDependency[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    subpassDependency[1].srcSubpass = 0;
    subpassDependency[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency[1].srcStageMask =
        vk::PipelineStageFlagBits::eColorAttachmentOutput;
    subpassDependency[1].dstStageMask =
        vk::PipelineStageFlagBits::eBottomOfPipe;
    subpassDependency[1].srcAccessMask =
        vk::AccessFlagBits::eColorAttachmentRead |
        vk::AccessFlagBits::eColorAttachmentWrite;
    subpassDependency[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;
    subpassDependency[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    // RENDER PASS
    vk::RenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.attachmentCount =
        static_cast<std::uint32_t>(attachments.size());
    renderPassCreateInfo.pAttachments = attachments.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = (uint32_t) subpassDependency.size();
    renderPassCreateInfo.pDependencies = subpassDependency.data();

    auto renderPass =
        m_device.device().createRenderPassUnique(renderPassCreateInfo);

    // FRAME BUFFER
    std::vector<vk::ImageView> attachmentViews;
    for (const auto& attachment : m_attachments) {
      attachmentViews.push_back(*attachment.imageView);
    }

    auto extent = m_attachments.front().extent;

    vk::FramebufferCreateInfo framebufferCreateInfo{};
    framebufferCreateInfo.renderPass = m_renderPass;
    framebufferCreateInfo.attachmentCount = (uint32_t) attachmentViews.size();
    framebufferCreateInfo.pAttachments = attachmentViews.data();
    framebufferCreateInfo.width = extent.width;
    framebufferCreateInfo.height = extent.height;
    framebufferCreateInfo.layers = 1; // todo
    vk::UniqueFramebuffer framebuffer =
        m_device.device().createFramebufferUnique(framebufferCreateInfo);
  }

  void clear() { m_attachments.clear(); }

private:
  Device& m_device;
  std::vector<FramebufferAttachment> m_attachments;
  vk::RenderPass m_renderPass{};
};