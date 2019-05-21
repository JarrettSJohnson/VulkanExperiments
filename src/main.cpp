#include "Application.hpp"
#include <iostream>

int main()
{
  Application app;
  /*app.m_window = Window{800, 600};
  app.initVulkan();
  app.setupDebugMessenger();
  app.m_surface = app.m_window.createSurface(*app.m_instance);
  app.selectPhysicalDevice();
  app.m_swapchain = Swapchain{app.m_device, *app.m_surface};
  app.createCommandPool();

  app.createUniformBuffers();
  app.m_texture = Texture{app.m_device, "../assets/cat_diff.tga"};
  app.m_model = Model{app.m_device, "../assets/cat.obj"};

  Cube cube{app.m_device};
 // std::vector<UBO<MVP>> mvps;
  //mvps.emplace_back(app.m_device, vk::ShaderStageFlagBits::eVertex);
 // mvps.emplace_back(app.m_device, vk::ShaderStageFlagBits::eVertex);


  DescriptorSetLayout layout{static_cast<std::uint32_t>(app.m_swapchain.size())};
  layout.addUBO(app.m_UBOs);
  layout.addSampler(app.m_texture);
  app.m_descriptorSetLayout = layout.generateLayout(app.m_device);
  app.m_descriptorPool = layout.generatePool(app.m_device);
  std::vector<vk::DescriptorSetLayout> layouts(
      app.m_swapchain.size(), *app.m_descriptorSetLayout);
  vk::DescriptorSetAllocateInfo allocateInfo{};
  allocateInfo.descriptorPool = *app.m_descriptorPool;
  allocateInfo.descriptorSetCount = static_cast<std::uint32_t>(layouts.size());
  allocateInfo.pSetLayouts = layouts.data();

  app.m_descriptorSets = app.m_device.device().allocateDescriptorSets(allocateInfo);
  layout.updateDescriptors(app.m_device, app.m_descriptorSets);
  app.createRenderPass();
  app.createPipeline();

  app.createColorResources();
  app.createDepthResources();
  app.createFramebuffers();

  std::vector<Application::IndexInfo> vBuffers;
  vBuffers.push_back({app.m_model.vertexBuffer(), app.m_model.indexBuffer(),
      static_cast<std::uint32_t>(app.m_model.indices().size())});
  vBuffers.push_back({cube.vertexBuffer(), cube.indexBuffer(),
      static_cast<std::uint32_t>(cube.indices().size())});

  app.createCommandBuffers(vBuffers);
  app.createSyncs();*/
  app.run();
  return 0;
}
