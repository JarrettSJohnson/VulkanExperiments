#pragma once

#include "Device.hpp"
#include "RenderPass.hpp"
#include "VKUtil.hpp"
#include <optional>

class Framebuffer
{
public:
  Framebuffer(Device& device, RenderPass& renderpass)
      : m_device{device}, m_renderPass{renderpass}
  {
  }

  void generate(std::optional<vk::ImageView> additionalImageView = std::nullopt)
  {

    // FRAME BUFFER
    std::vector<vk::ImageView> attachmentViews;
    for (const auto& attachment : m_renderPass.attachments()) {
      attachmentViews.push_back(*attachment.imageView);
    }
    if (additionalImageView) {
      attachmentViews.back() = additionalImageView.value();
    }

    auto extent = m_renderPass.attachments().front().extent;

    vk::FramebufferCreateInfo framebufferCreateInfo{};
    framebufferCreateInfo.renderPass = m_renderPass.renderpass();
    framebufferCreateInfo.attachmentCount = (uint32_t) attachmentViews.size();
    framebufferCreateInfo.pAttachments = attachmentViews.data();
    framebufferCreateInfo.width = extent.width;
    framebufferCreateInfo.height = extent.height;
    framebufferCreateInfo.layers = 1; // todo
    m_framebuffer =
        m_device.device().createFramebufferUnique(framebufferCreateInfo);
  }

  vk::Framebuffer framebuffer() const { return *m_framebuffer; }

private:
  Device& m_device;
  RenderPass& m_renderPass;
  vk::UniqueFramebuffer m_framebuffer{};
};