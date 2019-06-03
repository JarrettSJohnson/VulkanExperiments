#include "Application.hpp"
#include "Shader.hpp"

#include "Animation.hpp"
#include "Camera.hpp"
#include "Light.hpp"
#include <chrono>
#include <random>
#include <thread>

void updateCameraTransform(const TransformComponents& tData, Camera& camera)
{
  camera.m_pos = tData.pos;
  camera.m_pitch = tData.pitch;
  camera.m_yaw = tData.yaw;
  // camera.m_roll = tData.roll;
}

using namespace std::chrono_literals;

Application::Application() {}

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
  //////////createCommandBuffers();
  createUniformBuffers();
  // createDescriptorPool();
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
  commandPoolCreateInfo.flags =
      vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
  m_device.m_commandPool =
      m_device.device().createCommandPoolUnique(commandPoolCreateInfo);
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
    blit.srcOffsets[0] = {{0, 0, 0}};
    blit.srcOffsets[1] = {{mipWidth, mipHeight, 1}};
    blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = {{0, 0, 0}};
    blit.dstOffsets[1] = {{mipWidth > 1 ? mipWidth / 2 : 1,
        mipHeight > 1 ? mipHeight / 2 : 1, 1}};
    blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    commandBuffer->blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image,
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
  m_UBO = std::make_unique<UBO<LightUniforms>>(
      m_device, vk::ShaderStageFlagBits::eVertex);
}

void Application::allocateCommandBuffers()
{
  m_commandBuffers.resize(m_framebuffers.size());
  vk::CommandBufferAllocateInfo allocateInfo{};
  allocateInfo.commandPool = *m_device.m_commandPool;
  allocateInfo.level = vk::CommandBufferLevel::ePrimary;
  allocateInfo.commandBufferCount =
      static_cast<std::uint32_t>(m_commandBuffers.size());
  m_commandBuffers =
      m_device.device().allocateCommandBuffersUnique(allocateInfo);
}

void Application::setupCommandBuffers(
    const std::vector<IndexInfo>& buffers, std::size_t currentFrame)
{
  std::size_t i = currentFrame;

  m_commandBuffers[i]->reset(vk::CommandBufferResetFlagBits::eReleaseResources);
  vk::CommandBufferBeginInfo commandBufferBeginInfo{};
  commandBufferBeginInfo.flags =
      vk::CommandBufferUsageFlagBits::eSimultaneousUse;

  m_commandBuffers[i]->begin(commandBufferBeginInfo);

  // BEGIN OFFSCREEN RENDER PASS
  vk::RenderPassBeginInfo renderPassBeginInfo{};
  renderPassBeginInfo.renderPass = offscreenRenderPass->renderpass();
  renderPassBeginInfo.framebuffer = offscreenFB->framebuffer();
  renderPassBeginInfo.renderArea.offset = {{0, 0}};
  renderPassBeginInfo.renderArea.extent = m_swapchain.extent();

  std::array<vk::ClearValue, 2> clearValues{};
  clearValues[0].color = std::array{0.0f, 0.4f, 0.6f, 1.0f};
  clearValues[1].depthStencil = {{1.0f, 0}};
  renderPassBeginInfo.clearValueCount =
      static_cast<std::uint32_t>(clearValues.size());
  renderPassBeginInfo.pClearValues = clearValues.data();

  m_commandBuffers[i]->beginRenderPass(
      renderPassBeginInfo, vk::SubpassContents::eInline);
  m_commandBuffers[i]->bindPipeline(
      vk::PipelineBindPoint::eGraphics, offscreenPipeline.pipeline());

  vk::Viewport viewport;
  viewport.width = m_swapchain.extent().width;
  viewport.height = m_swapchain.extent().height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vk::Rect2D scissor{};
  scissor.extent = m_swapchain.extent();
  scissor.offset = {{0}};

  m_commandBuffers[i]->setViewport(0, viewport);
  m_commandBuffers[i]->setScissor(0, 1, &scissor);

  vk::DeviceSize offsets[] = {0};
  for (auto& buffer : buffers) {
    m_commandBuffers[i]->bindVertexBuffers(0, 1, &buffer.vBuffer, offsets);
    m_commandBuffers[i]->bindIndexBuffer(
        buffer.iBuffer, 0, vk::IndexType::eUint32);
    const auto& offscreenDS = offscreenDescriptorSets.descriptorSets();
    m_commandBuffers[i]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
        offscreenPipelineLayout.layout(), 0,
        static_cast<std::uint32_t>(offscreenDS.size()), offscreenDS.data(), 0,
        nullptr);
    m_commandBuffers[i]->pushConstants(offscreenPipelineLayout.layout(),
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        0, sizeof(buffer.pushConstants), &buffer.pushConstants);
    m_commandBuffers[i]->drawIndexed(buffer.numIndices, 1, 0, 0, 0);
  }
  m_commandBuffers[i]->endRenderPass();

  // VKUtil::transitionImageLayout(m_device,
  //    *offscreenRenderPass->attachments().back().image, m_swapchain.format(),
  //     vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal,
  //     1);

  // BEGIN DEFAULT/FULLSCREEN RENDER PASS
  renderPassBeginInfo.renderPass = m_renderPass->renderpass();
  renderPassBeginInfo.framebuffer = m_framebuffers[i].framebuffer();
  m_commandBuffers[i]->beginRenderPass(
      renderPassBeginInfo, vk::SubpassContents::eInline);
  m_commandBuffers[i]->bindPipeline(
      vk::PipelineBindPoint::eGraphics, m_graphicsPipeline.pipeline());
  const auto& defaultDS = m_DescriptorSet[i].descriptorSets();
  m_commandBuffers[i]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
      m_graphicsPipelineLayout.layout(), 0,
      static_cast<std::uint32_t>(defaultDS.size()), defaultDS.data(), 0,
      nullptr); ///

  /*vk::Viewport viewport;
  viewport.width = m_swapchain.extent().width;
  viewport.height = m_swapchain.extent().height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vk::Rect2D scissor{};
  scissor.extent = m_swapchain.extent();
  scissor.offset = {{0}};*/

  m_commandBuffers[i]->setViewport(0, viewport);
  m_commandBuffers[i]->setScissor(0, 1, &scissor);

  m_commandBuffers[i]->draw(3, 1, 0, 0);

  ui->draw(m_swapchain.extent().width, m_swapchain.extent().height,
      *m_commandBuffers[i]);

  m_commandBuffers[i]->endRenderPass();

  m_commandBuffers[i]->end();
}

void Application::createRenderPass()
{

  offscreenRenderPass = std::make_unique<RenderPass>(m_device);

  FrameBufferAttachmentInfo colorAttachInfo{};
  colorAttachInfo.extent = m_swapchain.extent();
  colorAttachInfo.format = m_swapchain.format();
  colorAttachInfo.numSamples = m_device.m_msaaSamples;
  colorAttachInfo.usage = vk::ImageUsageFlagBits::eTransientAttachment |
                          vk::ImageUsageFlagBits::eColorAttachment;

  FrameBufferAttachmentInfo depthAttachInfo{};
  depthAttachInfo.extent = m_swapchain.extent();
  depthAttachInfo.format = VKUtil::findDepthFormat(m_device);
  depthAttachInfo.numSamples = m_device.m_msaaSamples;
  depthAttachInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;

  FrameBufferAttachmentInfo resolveAttachInfo{};
  resolveAttachInfo.extent = m_swapchain.extent();
  resolveAttachInfo.format = m_swapchain.format();
  resolveAttachInfo.usage = vk::ImageUsageFlagBits::eColorAttachment |
                            vk::ImageUsageFlagBits::eSampled;
  resolveAttachInfo.isResolve = true;

  offscreenRenderPass->addAttachment(colorAttachInfo);
  offscreenRenderPass->addAttachment(depthAttachInfo);
  offscreenRenderPass->addAttachment(resolveAttachInfo);
  offscreenRenderPass->generate();

  m_renderPass = std::make_unique<RenderPass>(m_device);

  FrameBufferAttachmentInfo defaultColorAttachInfo{};
  defaultColorAttachInfo.extent = m_swapchain.extent();
  defaultColorAttachInfo.format = m_swapchain.format();
  defaultColorAttachInfo.numSamples = vk::SampleCountFlagBits::e1;
  defaultColorAttachInfo.usage = vk::ImageUsageFlagBits::eTransientAttachment |
                                 vk::ImageUsageFlagBits::eColorAttachment;
  defaultColorAttachInfo.presented = true;
  m_renderPass->addAttachment(defaultColorAttachInfo);

  m_renderPass->generate();
  int x = 4;
}

void Application::createPipeline()
{
  Shader offscreenVertShader{
      m_device, "../assets/test.vert.spv", Shader::ShaderType::VERTEX};
  Shader offscreenFragShader{
      m_device, "../assets/characters.frag.spv", Shader::ShaderType::FRAGMENT};
  vk::PushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags =
      vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
  pushConstantRange.size = sizeof(DanganPushConstants);

  offscreenPipelineLayout = PipelineLayout{m_device, m_swapchain,
      offscreenDescriptorSets.layout(), pushConstantRange};
  offscreenPipeline =
      Pipeline{m_device, m_swapchain.extent(), m_device.m_msaaSamples};
  offscreenPipeline.addVertexDescription<Vertex>();
  offscreenPipeline.generate(m_device, offscreenPipelineLayout,
      *offscreenRenderPass, offscreenVertShader, offscreenFragShader);

  Shader vertShader{
      m_device, "../assets/fullscreen.vert.spv", Shader::ShaderType::VERTEX};
  Shader fragShader{
      m_device, "../assets/fullscreen.frag.spv", Shader::ShaderType::FRAGMENT};
  m_graphicsPipelineLayout =
      PipelineLayout{m_device, m_swapchain, m_DescriptorSet.front().layout()};
  m_graphicsPipeline =
      Pipeline{m_device, m_swapchain.extent(), vk::SampleCountFlagBits::e1};
  m_graphicsPipeline.changeRasterizationFullscreenTriangle();
  m_graphicsPipeline.generate(m_device, m_graphicsPipelineLayout, *m_renderPass,
      vertShader, fragShader);
  int x = 5;
}

void Application::createFramebuffers()
{
  offscreenFB = std::make_unique<Framebuffer>(m_device, *offscreenRenderPass);
  offscreenFB->generate();

  m_framebuffers.emplace_back(m_device, *m_renderPass);
  m_framebuffers.emplace_back(m_device, *m_renderPass);

  for (std::size_t i{0u}; i < m_framebuffers.size(); ++i) {
    auto& framebuffer = m_framebuffers[i];
    framebuffer.generate(m_swapchain.imageView(i));
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

/*void Application::updateUniformBuffer(uint32_t currentImage, MVP newMVP)
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
      m_swapchain.extent().width / (float) m_swapchain.extent().height,
0.1f, 10.0f); ubo.proj[1][1] *= -1;

  VKUtil::transferToGPU(m_device, *m_UBOs[currentImage].m_memory, ubo);
}*/

std::uint32_t Application::getImageIdx()
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
    // return ;
  } else if (imageIdx.result != vk::Result::eSuccess &&
             imageIdx.result != vk::Result::eSuboptimalKHR) {
    throw std::runtime_error("failed to acquire swapchain image!");
  }
  return imageIdx.value;
}

void Application::drawFrame(std::uint32_t imageIdx)
{
  // updateUniformBuffer(imageIdx.value);

  vk::SubmitInfo submitInfo{};

  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &*m_drawSemaphores[currentFrame];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &*m_presentSemaphores[currentFrame];

  // std::array<vk::CommandBuffer, 2> commandBuffers{
  //    *m_commandBuffers[0], *m_commandBuffers[1]};
  // submitInfo.commandBufferCount =
  //    static_cast<std::uint32_t>(commandBuffers.size());
  // submitInfo.pCommandBuffers = commandBuffers.data();
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &*m_commandBuffers[currentFrame];
  vk::PipelineStageFlags waitStages[] = {
      vk::PipelineStageFlagBits::eColorAttachmentOutput};
  submitInfo.pWaitDstStageMask = waitStages;

  m_device.device().resetFences(1, &*m_fences[currentFrame]);
  m_device.m_graphicsQueue.submit(submitInfo, *m_fences[currentFrame]);
  int x = 5;
}

void Application::present(std::uint32_t imageIdx)
{
  std::array<vk::Semaphore, 1> presentSemaphores{
      *m_presentSemaphores[currentFrame]};
  std::array<vk::SwapchainKHR, 1> swapchains{m_swapchain.swapchain()};
  vk::PresentInfoKHR presentInfo{};
  presentInfo.waitSemaphoreCount =
      static_cast<std::uint32_t>(presentSemaphores.size());
  presentInfo.pWaitSemaphores = presentSemaphores.data();
  presentInfo.swapchainCount = static_cast<std::uint32_t>(swapchains.size());
  presentInfo.pSwapchains = swapchains.data();
  presentInfo.pImageIndices = &imageIdx;
  if (Window::framebufferResized) {
    recreateSwapchain();
    Window::framebufferResized = false;
  }
  auto presentResult = m_device.m_graphicsQueue.presentKHR(presentInfo);
  if (presentResult == vk::Result::eErrorOutOfDateKHR ||
      presentResult == vk::Result::eSuboptimalKHR ||
      Window::framebufferResized) {
    Window::framebufferResized = false;
    recreateSwapchain();
  } else if (presentResult != vk::Result::eSuccess) {
    throw std::runtime_error("failed to present!");
  }
  // m_device.m_graphicsQueue.waitIdle();
  currentFrame = (currentFrame + 1) % framesInFlight;
}

void Application::run()
{
  m_window = Window{1024, 576};
  initVulkan();
  setupDebugMessenger();
  m_surface = m_window.createSurface(*m_instance);
  selectPhysicalDevice();
  m_swapchain = Swapchain{m_device, *m_surface};
  createCommandPool();

  createUniformBuffers();

  std::vector<std::string> names{"mc", "keebo", "hair", "rantaro", "mc",
      "keebo", "hair", "rantaro", "mc", "keebo", "hair", "rantaro", "mc",
      "keebo", "hair", "rantaro"};
  std::mt19937 mt{std::random_device{}()};
  std::uniform_int_distribution<int> dist(0, names.size());
  std::vector<Texture> textures;
  textures.reserve(names.size());
  for (unsigned int i = 0; i < names.size(); ++i) {
    textures.emplace_back(
        m_device, std::string{"../assets/dangan/"} + names[i] + ".png");
  }

  std::vector<Model> models;
  std::vector<glm::mat4> mvps;
  unsigned int num = 16;
  models.reserve(num);
  for (int i = 0; i < num; i++) {
    auto randIdx = dist(mt);
    // models.emplace_back(
    //    m_device, std::string{"../assets/dangan/"} + names[randIdx] +
    //    ".obj");
    models.emplace_back(
        m_device, std::string{"../assets/dangan/"} + names[i] + ".obj");
    mvps.emplace_back(glm::mat4(1.0f));
  }
  float rotation{};
  for (auto& mat : mvps) {
    mat = glm::rotate(mat, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));
    mat = glm::translate(mat, glm::vec3(0.0f, 0.0f, 2.0f));
    mat = glm::rotate(mat, glm::radians(180.f), glm::vec3(0.0f, 1.0f, 0.0f));
    rotation += 360.f / num;
  }

  CubedLight light{m_device};
  // light.light.pos = glm::vec3(0.0f, 0.0f, 0.0f);
  light.light.pos = glm::vec3(3.0, 3.0, 3.0f);
  light.light.color = glm::vec3(1.0f, 1.0f, 1.0f);
  // std::vector<UBO<MVP>> mvps;
  // mvps.emplace_back(app.m_device, vk::ShaderStageFlagBits::eVertex);
  // mvps.emplace_back(app.m_device, vk::ShaderStageFlagBits::eVertex);

  m_UBO = std::make_unique<UBO<LightUniforms>>(
      m_device, vk::ShaderStageFlagBits::eAllGraphics);

  createRenderPass();

  offscreenDescriptorSets.addUBO(*m_UBO);
  // offscreenDescriptorSets.addTexture(textures.front());
  offscreenDescriptorSets.addTextureArray(textures);
  offscreenDescriptorSets.generateLayout(m_device);
  offscreenDescriptorSets.generatePool(m_device);

  vk::UniqueSampler offViewSampler = VKUtil::createTextureSampler(m_device);

  m_DescriptorSet.resize(m_swapchain.size());
  for (auto& descriptor : m_DescriptorSet) {
    descriptor.addSampler(
        *offscreenRenderPass->attachments().back().imageView, *offViewSampler);
    descriptor.generateLayout(m_device);
    descriptor.generatePool(m_device);
  }

  createPipeline();
  createFramebuffers();

  std::vector<Application::IndexInfo> vBuffers;
  /*vBuffers.push_back(
      {m_model.vertexBuffer(), m_model.indexBuffer(), m_model.numIndices()});
  vBuffers.push_back(
      {m_model.vertexBuffer(), m_model.indexBuffer(), m_model.numIndices()});*/
  for (auto& model : models) {
    vBuffers.push_back(
        {model.vertexBuffer(), model.indexBuffer(), model.numIndices()});
  }

  m_UBO->map();

  allocateCommandBuffers();
  createSyncs();

  Camera camera;

  ui = std::make_unique<UIOverlay>(m_device);
  ui->prepareResources();
  ui->preparePipeline(m_renderPass->renderpass());

  AnimateSystem animSys{};
  auto& testAnim = animSys.addAnimation(true);
  auto& keyframe = testAnim.addKeyFrame(700.f);
  TransformComponents beginLinear{};
  beginLinear = transformFromCamera(camera);
  TransformComponents endLinear = beginLinear;
  endLinear.pos = glm::vec3(-0.052f, 1.431f, -0.818f);
  keyframe.setTranslationInterpMode<Linear>(beginLinear, endLinear);

  auto& keyframe2 = testAnim.addKeyFrame(2750.f);
  TransformComponents beginLinear2 = endLinear;
  TransformComponents endLinear2 = beginLinear;
  endLinear2.pos = beginLinear2.pos + glm::vec3(0.1f, 0.0f, 0.0f);
  keyframe2.setTranslationInterpMode<Linear>(beginLinear2, endLinear2);

  auto& keyframe3 = testAnim.addKeyFrame(300.0f);
  TransformComponents beginLinear3 = endLinear2;
  TransformComponents endLinear3 = beginLinear3;
  endLinear3.pos = glm::vec3(0.876f, 0.515f, 0.332f);
  endLinear3.yaw = 112.8f;
  keyframe3.setTranslationInterpMode<Linear>(beginLinear3, endLinear3);

  auto& keyframe4 = testAnim.addKeyFrame(1500.0f);
  TransformComponents beginLinear4 = endLinear3;
  TransformComponents endLinear4 = beginLinear4;
  endLinear4.pos = beginLinear4.pos + glm::vec3(0.0f, 1.1f, 0.0f);
  keyframe4.setTranslationInterpMode<Linear>(beginLinear4, endLinear4);

  TransformComponents cameraTransformData = transformFromCamera(camera);
  testAnim.setTransform(cameraTransformData);

  auto start_frame = std::chrono::high_resolution_clock::now();
  // 60fps = 16.67 ms per frame
  const auto frame_time = std::chrono::microseconds(16667);

  while (!m_window.shouldClose()) {
    m_window.processInputWindow();
    m_window.processInputCamera(camera);
    camera.moveYawPitch(m_window.getMouseOffset());
    // camera.setDir(m_window.getNewDir());

    glfwPollEvents();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration_cast<std::chrono::milliseconds>(frame_time)
                   .count();
    if (animSys.animate(dt))
      updateCameraTransform(cameraTransformData, camera);

    auto model = glm::mat4(1.0f);
    auto view = camera.view();
    auto viewPos = camera.position();
    auto proj = glm::perspective(glm::radians(45.0f),
        m_swapchain.extent().width / (float) m_swapchain.extent().height, 0.1f,
        10.0f);
    proj[1][1] *= -1;

    for (unsigned int i = 0; i < vBuffers.size(); ++i) {
      vBuffers[i].pushConstants.model = mvps[i];
      vBuffers[i].pushConstants.charIdx = i;
    }
    // vBuffers[0].pushConstants.model = model;
    // vBuffers[1].pushConstants.model = light.transform();
    m_UBO->get().projview = proj * view;
    m_UBO->get().viewPosition =
        glm::vec4(viewPos.x, viewPos.y, viewPos.z, 0.0f);
    m_UBO->get().lightPosition = glm::vec4(
        light.light.pos.x, light.light.pos.y, light.light.pos.z, 0.0f);
    m_UBO->get().lightColor = glm::vec4(
        light.light.color.r, light.light.color.g, light.light.color.b, 1.0f);
    auto imageIdx = getImageIdx();
    updateUniformBuffer(imageIdx);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float) m_swapchain.extent().width,
        (float) m_swapchain.extent().height);
    ImGui::NewFrame();
    // ui.update();
    // ui.draw(*m_commandBuffers[imageIdx]);
    auto guiWidth = 300;
    auto guiHeight = 100;
    auto [width, height] = m_swapchain.extent();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::SetNextWindowPos(ImVec2(10, height - guiHeight - 10));
    ImGui::SetNextWindowSize(
        ImVec2(guiWidth, guiHeight), ImGuiSetCond_FirstUseEver);
    ImGui::Begin("Vulkan Example", nullptr);
    // ImGui::TextUnformatted(printVec3(camera.position()).c_str());
    // ImGui::TextUnformatted(printVec3(camera.dir()).c_str());

    ImGui::PushItemWidth(110.0f * 1.0f);
    // OnUpdateUIOverlay(&UIOverlay);
    ImGui::PopItemWidth();

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::Render();

    ui->update();
    setupCommandBuffers(vBuffers, currentFrame);

    drawFrame(imageIdx);

    present(imageIdx);
    auto endTime = start_frame + frame_time;
    // auto remainingTime = endTime -
    // std::chrono::high_resolution_clock::now(); std::cout <<
    // remainingTime.count() << '\n';
    std::this_thread::sleep_until(endTime);
    start_frame += frame_time;
  }
  m_UBO->unmap();
  m_device.device().waitIdle();
}
