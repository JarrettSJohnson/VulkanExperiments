#include "Application.hpp"
#include "Shader.hpp"

#include <chrono>

Application::Application()
{

}

void Application::initVulkan()
{
  vk::ApplicationInfo appInfo{};
  appInfo.pApplicationName = "Vulkan Test";
  appInfo.applicationVersion = 0;
  appInfo.pEngineName = "Vulkan Test Engine";
  appInfo.engineVersion = 0;
  appInfo.apiVersion = VK_API_VERSION_1_1;

  vk::InstanceCreateInfo createInfo{};
  createInfo.pApplicationInfo = &appInfo;

  if (enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<std::uint32_t>(m_enabledLayers.size());
    createInfo.ppEnabledLayerNames = m_enabledLayers.data();
  }
  auto requiredExtensions = getRequiredExtensions();
  createInfo.enabledExtensionCount =
      static_cast<std::uint32_t>(requiredExtensions.size());
  createInfo.ppEnabledExtensionNames = requiredExtensions.data();

  m_instance = vk::createInstanceUnique(createInfo);
  m_dldy.init(*m_instance);
  /*auto layers = vk::enumerateInstanceLayerProperties();

  checkValidationLayerSupport();*/
  // setupDebugMessenger();
}

std::vector<const char*> Application::getRequiredExtensions()
{
  std::vector<const char*> requiredExts;
  std::uint32_t numRequiredExtensionsGLFW{};
  auto requiredExtensionsGLFW =
      glfwGetRequiredInstanceExtensions(&numRequiredExtensionsGLFW);
  for (std::size_t i = 0; i < numRequiredExtensionsGLFW; ++i) {
    requiredExts.push_back(requiredExtensionsGLFW[i]);
  }
  requiredExts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  return requiredExts;
}

void Application::recreateSwapchain()
{
  int width = 0, height = 0;
  while (width == 0 || height == 0) {
    std::tie(width, height) = m_window.getSize();
    glfwWaitEvents();
  }
  m_device.device().waitIdle();
  m_swapchain = Swapchain{m_device, *m_surface};
  createRenderPass();
  createPipeline();
  createFramebuffers();
  createCommandBuffers();
  createUniformBuffers();
  //createDescriptorPool();
  // createCommandBuffers();
}

void Application::selectPhysicalDevice()
{
  m_device = Device{m_instance->enumeratePhysicalDevices().front()};
  m_device.m_msaaSamples = VKUtil::getMaxUsableSampleCount(m_device);
}

void Application::createCommandPool()
{
  vk::CommandPoolCreateInfo commandPoolCreateInfo{};
  commandPoolCreateInfo.queueFamilyIndex = 0;
  m_device.m_commandPool =
      m_device.device().createCommandPoolUnique(commandPoolCreateInfo);
}


void Application::createColorResources()
{
  vk::Format format = m_swapchain.format();
  vk::Extent3D extent = m_swapchain.extent3D();
  std::tie(colorImage, colorImageMemory) =
      VKUtil::createImage(m_device,extent, 1, m_device.m_msaaSamples, format, vk::ImageTiling::eOptimal,
          vk::ImageUsageFlagBits::eTransientAttachment |
              vk::ImageUsageFlagBits::eColorAttachment,
          vk::MemoryPropertyFlagBits::eDeviceLocal);

  colorImageView =
      VKUtil::createImageView(m_device, *colorImage, format, vk::ImageAspectFlagBits::eColor, 1);

  VKUtil::transitionImageLayout(m_device, *colorImage, format, vk::ImageLayout::eUndefined,
      vk::ImageLayout::eColorAttachmentOptimal, 1);
}

void Application::createDepthResources()
{
  vk::Format depthFormat = VKUtil::findDepthFormat(m_device);
  vk::Extent3D extent = m_swapchain.extent3D();
  std::tie(m_depthImage, m_depthImageMemory) = VKUtil::createImage(m_device, extent, 1, m_device.m_msaaSamples, depthFormat, vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eDepthStencilAttachment,
      vk::MemoryPropertyFlagBits::eDeviceLocal);

  m_depthImageView = VKUtil::createImageView(m_device,
      *m_depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);

  VKUtil::transitionImageLayout(m_device, *m_depthImage, depthFormat, vk::ImageLayout::eUndefined,
      vk::ImageLayout::eDepthStencilAttachmentOptimal, 1);
}

void Application::generateMipmaps(vk::Image image, vk::Format format,
    std::uint32_t texWidth, std::uint32_t texHeight, uint32_t mipLevels)
{
  vk::FormatProperties formatProperties =
      m_device.m_physicalDevice.getFormatProperties(format);

  if (!(formatProperties.optimalTilingFeatures &
          vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
    throw std::runtime_error(
        "texture image format does not support linear blitting!");
  }

  auto commandBuffer = VKUtil::beginSingleTimeCommands(m_device);

  vk::ImageMemoryBarrier barrier = {};
  barrier.image = image;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  std::int32_t mipWidth = texWidth;
  std::int32_t mipHeight = texHeight;

  for (uint32_t i = 1; i < mipLevels; i++) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

    commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags{}, 0, nullptr,
        0, nullptr, 1, &barrier);

    vk::ImageBlit blit{};
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
    blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = {0, 0, 0};
    blit.dstOffsets[1] = {
        mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
    blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    commandBuffer->blitImage(image,
        vk::ImageLayout::eTransferSrcOptimal, image,
        vk::ImageLayout::eTransferDstOptimal, 1, &blit, vk::Filter::eLinear);

    barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader, vk::DependencyFlags{}, 0,
        nullptr, 0, nullptr, 1, &barrier);

    if (mipWidth > 1)
      mipWidth /= 2;
    if (mipHeight > 1)
      mipHeight /= 2;
  }

  VKUtil::endSingleTimeCommands(commandBuffer, m_device.m_graphicsQueue);
}



void Application::createUniformBuffers()
{
  m_UBOs.reserve(m_swapchain.size());
  for (int i = 0; i < m_swapchain.size(); ++i) {
    m_UBOs.emplace_back(m_device, vk::ShaderStageFlagBits::eVertex);
  }
}

void Application::createCommandBuffers()
{
  m_commandBuffers.resize(m_framebuffers.size());
  vk::CommandBufferAllocateInfo allocateInfo{};
  allocateInfo.commandPool = *m_device.m_commandPool;
  allocateInfo.level = vk::CommandBufferLevel::ePrimary;
  allocateInfo.commandBufferCount =
      static_cast<std::uint32_t>(m_commandBuffers.size());
  m_commandBuffers =
      m_device.device().allocateCommandBuffersUnique(allocateInfo);

  for (std::size_t i{0u}; i < m_commandBuffers.size(); ++i) {
    vk::CommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.flags =
        vk::CommandBufferUsageFlagBits::eSimultaneousUse;

    vk::RenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.renderPass = *m_renderPass;
    renderPassBeginInfo.framebuffer = *m_framebuffers[i];
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = m_swapchain.extent();

    std::array<float, 4> clearcolorvalues{0.33f, 0.41f, 0.58f, 1.0f};
    std::array<vk::ClearValue, 2> clearValues{};
    clearValues[0].color = clearcolorvalues;
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassBeginInfo.clearValueCount =
        static_cast<std::uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    m_commandBuffers[i]->begin(commandBufferBeginInfo);
    m_commandBuffers[i]->beginRenderPass(
        renderPassBeginInfo, vk::SubpassContents::eInline);
    m_commandBuffers[i]->bindPipeline(
        vk::PipelineBindPoint::eGraphics, m_graphicsPipeline.pipeline());
    vk::Buffer vertexBuffers[] = {m_model.vertexBuffer()};
    vk::DeviceSize offsets[] = {0};
    m_commandBuffers[i]->bindVertexBuffers(0, 1, vertexBuffers, offsets);
    m_commandBuffers[i]->bindIndexBuffer(
        m_model.indexBuffer(), 0, vk::IndexType::eUint32);
    m_commandBuffers[i]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
        m_graphicsPipeline.layout(), 0, 1, &m_descriptorSets[i], 0, nullptr);
    m_commandBuffers[i]->drawIndexed(
        static_cast<std::uint32_t>(m_model.indices().size()), 1, 0, 0, 0);
    m_commandBuffers[i]->endRenderPass();
    m_commandBuffers[i]->end();
  }
}

void Application::createRenderPass()
{
  vk::AttachmentDescription colorAttachment;
  colorAttachment.format = m_swapchain.format();
  colorAttachment.samples = m_device.m_msaaSamples;
  colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
  colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
  colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
  colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
  colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

  vk::AttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

  vk::AttachmentDescription depthAttachment;
  depthAttachment.format = VKUtil::findDepthFormat(m_device);
  depthAttachment.samples = m_device.m_msaaSamples;
  depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
  depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
  depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
  depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
  depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

  vk::AttachmentReference depthAttachmentRef{};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

  vk::AttachmentDescription resolveAttachment{};
  resolveAttachment.format = m_swapchain.format();
  resolveAttachment.samples = vk::SampleCountFlagBits::e1;
  resolveAttachment.loadOp = vk::AttachmentLoadOp::eDontCare;
  resolveAttachment.storeOp = vk::AttachmentStoreOp::eStore;
  resolveAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
  resolveAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  resolveAttachment.initialLayout = vk::ImageLayout::eUndefined;
  resolveAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

  vk::AttachmentReference resolveRef{};
  resolveRef.attachment = 2;
  resolveRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

  vk::SubpassDescription subpassDescription{};
  subpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
  subpassDescription.colorAttachmentCount = 1;
  subpassDescription.pColorAttachments = &colorAttachmentRef;
  subpassDescription.pDepthStencilAttachment = &depthAttachmentRef;
  subpassDescription.pResolveAttachments = &resolveRef;

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

  std::array<vk::AttachmentDescription, 3> attachments{
      colorAttachment, depthAttachment, resolveAttachment};

  vk::RenderPassCreateInfo renderPassCreateInfo{};
  renderPassCreateInfo.attachmentCount =
      static_cast<std::uint32_t>(attachments.size());
  renderPassCreateInfo.pAttachments = attachments.data();
  renderPassCreateInfo.subpassCount = 1;
  renderPassCreateInfo.pSubpasses = &subpassDescription;
  renderPassCreateInfo.dependencyCount = (uint32_t)subpassDependency.size();
  renderPassCreateInfo.pDependencies = subpassDependency.data();

  m_renderPass = m_device.device().createRenderPassUnique(renderPassCreateInfo);
}

void Application::createPipeline()
{
  Shader vertShader{
      m_device, "../assets/test.vert.spv", Shader::ShaderType::VERTEX};
  Shader fragShader{
      m_device, "../assets/test.frag.spv", Shader::ShaderType::FRAGMENT};
  m_graphicsPipeline = Pipeline{m_device, m_swapchain, *m_renderPass, *m_descriptorSetLayout,
	  vertShader, fragShader};
}

void Application::createFramebuffers()
{
  m_framebuffers.resize(m_swapchain.size());

  for (std::size_t i{0u}; i < m_framebuffers.size(); ++i) {
    std::array<vk::ImageView, 3> attachments = {
        *colorImageView, *m_depthImageView, m_swapchain.imageView(i)};

    vk::FramebufferCreateInfo framebufferCreateInfo{};
    framebufferCreateInfo.renderPass = *m_renderPass;
    framebufferCreateInfo.attachmentCount =
        static_cast<std::uint32_t>(attachments.size());
    framebufferCreateInfo.pAttachments = attachments.data();
    framebufferCreateInfo.width = m_swapchain.extent().width;
    framebufferCreateInfo.height = m_swapchain.extent().height;
    framebufferCreateInfo.layers = 1;
    m_framebuffers[i] =
        m_device.device().createFramebufferUnique(framebufferCreateInfo);
  }
}

void Application::createSyncs()
{
  for (std::size_t i{0u}; i < framesInFlight; ++i) {
    vk::SemaphoreCreateInfo drawSemaphoreCreateInfo{};
    m_drawSemaphores[i] =
        m_device.device().createSemaphoreUnique(drawSemaphoreCreateInfo);
    vk::SemaphoreCreateInfo presentSemaphoreCreateInfo{};
    m_presentSemaphores[i] =
        m_device.device().createSemaphoreUnique(presentSemaphoreCreateInfo);
    vk::FenceCreateInfo fenceInfo{};
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    m_fences[i] = m_device.device().createFenceUnique(fenceInfo);
  }
}

void Application::updateUniformBuffer(uint32_t currentImage)
{
  static auto startTime = std::chrono::high_resolution_clock::now();

  auto currentTime = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>(
      currentTime - startTime)
                   .count();

  MVP ubo{};
  // ubo.model = glm::mat4(1.0f);
  // ubo.model = glm::rotate(
  //     glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f,
  //     0.0f, 1.0f));
  // ubo.model = glm::rotate(
  //     ubo.model, time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  ubo.model = glm::rotate(
      glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  // ubo.model = glm::scale(
  //     ubo.model, glm::vec3(0.1, 0.1, 0.1));
  ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
      glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.proj = glm::perspective(glm::radians(45.0f),
      m_swapchain.extent().width / (float) m_swapchain.extent().height, 0.1f, 10.0f);
  ubo.proj[1][1] *= -1;

  VKUtil::transferToGPU(m_device, *m_UBOs[currentImage].m_memory, ubo);
}

void Application::drawFrame()
{
  if (Window::framebufferResized) {
    recreateSwapchain();
    Window::framebufferResized = false;
  }
  m_device.device().waitForFences(1, &*m_fences[currentFrame], VK_TRUE,
      std::numeric_limits<std::uint64_t>::max());

  auto imageIdx = m_device.device().acquireNextImageKHR(m_swapchain.swapchain(),
      std::numeric_limits<std::uint64_t>::max(),
      *m_drawSemaphores[currentFrame], vk::Fence{});

  if (imageIdx.result == vk::Result::eErrorOutOfDateKHR) {
    recreateSwapchain();
    return;
  } else if (imageIdx.result != vk::Result::eSuccess &&
             imageIdx.result != vk::Result::eSuboptimalKHR) {
    throw std::runtime_error("failed to acquire swapchain image!");
  }

  updateUniformBuffer(imageIdx.value);

  vk::SubmitInfo submitInfo{};

  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &*m_drawSemaphores[currentFrame];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &*m_presentSemaphores[currentFrame];

  std::array<vk::CommandBuffer, 2> commandBuffers{
      *m_commandBuffers[0], *m_commandBuffers[1]};
  submitInfo.commandBufferCount =
      static_cast<std::uint32_t>(commandBuffers.size());
  submitInfo.pCommandBuffers = commandBuffers.data();
  vk::PipelineStageFlags waitStages[] = {
      vk::PipelineStageFlagBits::eColorAttachmentOutput};
  submitInfo.pWaitDstStageMask = waitStages;

  m_device.device().resetFences(1, &*m_fences[currentFrame]);
  m_device.m_graphicsQueue.submit(submitInfo, *m_fences[currentFrame]);

  std::array<vk::Semaphore, 1> presentSemaphores{
      *m_presentSemaphores[currentFrame]};
  std::array<vk::SwapchainKHR, 1> swapchains{m_swapchain.swapchain()};
  vk::PresentInfoKHR presentInfo{};
  presentInfo.waitSemaphoreCount =
      static_cast<std::uint32_t>(presentSemaphores.size());
  presentInfo.pWaitSemaphores = presentSemaphores.data();
  presentInfo.swapchainCount = static_cast<std::uint32_t>(swapchains.size());
  presentInfo.pSwapchains = swapchains.data();
  presentInfo.pImageIndices = &imageIdx.value;
  if (Window::framebufferResized) {
    recreateSwapchain();
    Window::framebufferResized = false;
  }
  auto presentResult = m_device.m_graphicsQueue.presentKHR(presentInfo);
  if (presentResult == vk::Result::eErrorOutOfDateKHR ||
      presentResult == vk::Result::eSuboptimalKHR || Window::framebufferResized) {
    Window::framebufferResized = false;
    recreateSwapchain();
  } else if (presentResult != vk::Result::eSuccess) {
    throw std::runtime_error("failed to present!");
  }
  currentFrame = (currentFrame + 1) % framesInFlight;
}

void Application::run()
{
  while (!m_window.shouldClose()) {
    glfwPollEvents();
    drawFrame();
  }
  m_device.device().waitIdle();
}
