#pragma once

#include "Device.hpp"
#include "VKUtil.hpp"
#include <optional>

struct FrameBufferAttachmentInfo {
  vk::Extent2D extent;
  std::uint32_t layerCount{1};
  vk::Format format;
  vk::ImageUsageFlags usage;
  vk::SampleCountFlagBits numSamples{vk::SampleCountFlagBits::e1};
  bool isResolve{false};
  bool presented{false};
};

struct FramebufferAttachment {
  vk::AttachmentDescription description{};
  vk::ImageLayout layout{};
  vk::Extent3D extent{};
  vk::UniqueImage image{};
  vk::UniqueImageView imageView{};
  vk::UniqueDeviceMemory memory{};
  bool isResolve{};
};

class RenderPass
{
public:
  RenderPass() = default;
  RenderPass(Device& device) : m_device{&device} {}
  FramebufferAttachment& addAttachment(FrameBufferAttachmentInfo fbAttInfo)
  {
    auto& attachment = m_attachments.emplace_back();
    attachment.extent =
        vk::Extent3D{fbAttInfo.extent.width, fbAttInfo.extent.height, 1};
    attachment.description.format = fbAttInfo.format;
    attachment.isResolve = fbAttInfo.isResolve;

    vk::ImageAspectFlags aspectFlag{};

    if (fbAttInfo.usage & vk::ImageUsageFlagBits::eColorAttachment) {
      aspectFlag = vk::ImageAspectFlagBits::eColor;
    } else if (fbAttInfo.usage &
               vk::ImageUsageFlagBits::eDepthStencilAttachment) {
      if (VKUtil::hasDepthComponent(attachment.description.format)) {
        aspectFlag = vk::ImageAspectFlagBits::eDepth;
      }

      if (VKUtil::hasStencilComponent(attachment.description.format)) {
        aspectFlag = vk::ImageAspectFlagBits::eStencil;
      }
    }

    attachment.description.format = fbAttInfo.format;

    if (fbAttInfo.isResolve) {
      attachment.description.loadOp = vk::AttachmentLoadOp::eDontCare;
    } else {
      attachment.description.loadOp = vk::AttachmentLoadOp::eClear;
    }

    if (!fbAttInfo.presented) {

      attachment.description.samples = fbAttInfo.numSamples;

      // attachment.description.storeOp =
      //    (fbAttInfo.usage & vk::ImageUsageFlagBits::eSampled)
      //        ? vk::AttachmentStoreOp::eStore
      //       : vk::AttachmentStoreOp::eDontCare;
      // attachment.description.storeOp = vk::AttachmentStoreOp::eStore;
      attachment.description.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
      attachment.description.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
      attachment.description.initialLayout = vk::ImageLayout::eUndefined;

      int miplevels = 1;
      vk::Extent3D fboExtent{
          fbAttInfo.extent.width, fbAttInfo.extent.height, 1};
      std::tie(attachment.image, attachment.memory) = VKUtil::createImage(
          *m_device, fboExtent, miplevels, fbAttInfo.numSamples,
          attachment.description.format, vk::ImageTiling::eOptimal,
          fbAttInfo.usage, vk::MemoryPropertyFlagBits::eDeviceLocal);

      attachment.imageView = VKUtil::createImageView(m_device->device(),
          *attachment.image, attachment.description.format, aspectFlag, 1);

      if (VKUtil::hasDepthComponent(attachment.description.format) ||
          VKUtil::hasStencilComponent(attachment.description.format)) {
        attachment.description.storeOp = vk::AttachmentStoreOp::eDontCare;
        attachment.description.finalLayout =
            vk::ImageLayout::eDepthStencilAttachmentOptimal;
      } else {
        attachment.description.storeOp = vk::AttachmentStoreOp::eStore;
        attachment.description.finalLayout =
            vk::ImageLayout::eColorAttachmentOptimal;
        if (fbAttInfo.isResolve) {
          attachment.description.finalLayout =
              vk::ImageLayout::eShaderReadOnlyOptimal;
        }
      }
      VKUtil::transitionImageLayout(*m_device, *attachment.image,
          attachment.description.format, attachment.description.initialLayout,
          attachment.description.finalLayout, 1);
    } else {

      attachment.description.samples = vk::SampleCountFlagBits::e1;
      attachment.description.loadOp = vk::AttachmentLoadOp::eDontCare;
      attachment.description.storeOp = vk::AttachmentStoreOp::eStore;

      attachment.description.initialLayout = vk::ImageLayout::eUndefined;
      attachment.description.finalLayout = vk::ImageLayout::ePresentSrcKHR;
    }
    return attachment;
  }
  void generate()
  {

    std::vector<vk::AttachmentReference> colorReferences;
    std::optional<vk::AttachmentReference> depthReference;
    std::optional<vk::AttachmentReference> resolveReference;
    bool hasDepth = false;
    bool hasStencil = false;

    std::uint32_t attIdx{0u};
    for (auto& attachment : m_attachments) {
      if (attachment.isResolve) {
        resolveReference = vk::AttachmentReference{};
        resolveReference->attachment = attIdx;
        resolveReference->layout = vk::ImageLayout::eColorAttachmentOptimal;
      } else if (VKUtil::hasDepthStencilComponent(
                     attachment.description.format)) {
        depthReference = vk::AttachmentReference{};
        depthReference->attachment = attIdx;
        depthReference->layout =
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
    subpassDescription.colorAttachmentCount =
        static_cast<std::uint32_t>(colorReferences.size());
    subpassDescription.pColorAttachments = colorReferences.data();
    subpassDescription.pDepthStencilAttachment =
        depthReference.has_value() ? &depthReference.value() : nullptr;
    subpassDescription.pResolveAttachments =
        resolveReference.has_value() ? &resolveReference.value() : nullptr;
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
    renderPassCreateInfo.dependencyCount =
        static_cast<std::uint32_t>(subpassDependency.size());
    renderPassCreateInfo.pDependencies = subpassDependency.data();

    m_renderPass =
        m_device->device().createRenderPassUnique(renderPassCreateInfo);
  }
  vk::RenderPass renderpass() const { return *m_renderPass; }
  const auto& attachments() const { return m_attachments; }
  void clear() { m_attachments.clear(); }

private:
  Device* m_device{nullptr};
  std::vector<FramebufferAttachment> m_attachments;
  vk::UniqueRenderPass m_renderPass{};
};
