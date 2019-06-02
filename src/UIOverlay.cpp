/*
 * UI overlay class using ImGui
 *
 * Copyright (C) 2017 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT)
 * (http://opensource.org/licenses/MIT)
 */

#include <assert.h>
#include <iomanip>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Shader.hpp"
#include "UIOverlay.hpp"
#include "VKUtil.hpp"

UIOverlay::UIOverlay(Device& paramDevice)
    : device{&paramDevice}, vertexBuffer{paramDevice}, indexBuffer{paramDevice}
{
  // Init ImGui
  ImGui::CreateContext();
  // Color scheme
  ImGuiStyle& style = ImGui::GetStyle();
  style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
  style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
  style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.0f, 0.0f, 0.0f, 0.1f);
  style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
  style.Colors[ImGuiCol_Header] = ImVec4(0.8f, 0.0f, 0.0f, 0.4f);
  style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
  style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
  style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
  style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
  style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
  style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
  style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
  style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
  style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
  style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
  style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
  // Dimensions
  ImGuiIO& io = ImGui::GetIO();
  io.FontGlobalScale = scale;
}

/** Prepare all vulkan resources required to render the UI overlay */
void UIOverlay::prepareResources()
{
  ImGuiIO& io = ImGui::GetIO();

  // Create font texture -> Use Texture class????
  unsigned char* fontData;
  int texWidth, texHeight;
  io.Fonts->AddFontFromFileTTF("../assets/Roboto-Medium.ttf", 16.0f);
  io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
  vk::DeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

  auto [stagingBuffer, stagingBufferMemory] = VKUtil::createBuffer(*device,
      uploadSize, vk::BufferUsageFlagBits::eTransferSrc,
      vk::MemoryPropertyFlagBits::eHostVisible |
          vk::MemoryPropertyFlagBits::eHostCoherent);

  void* data;
  device->device().mapMemory(*stagingBufferMemory, 0,
      static_cast<std::size_t>(uploadSize), vk::MemoryMapFlags{}, &data);
  std::memcpy(data, fontData, uploadSize);
  device->device().unmapMemory(*stagingBufferMemory);

  std::tie(fontImage, fontMemory) = VKUtil::createImage(*device,
      vk::Extent3D{static_cast<std::uint32_t>(texWidth),
          static_cast<std::uint32_t>(texHeight), 1},
      1, vk::SampleCountFlagBits::e1, vk::Format::eR8G8B8A8Unorm,
      vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
      vk::MemoryPropertyFlagBits::eDeviceLocal);

  VKUtil::transitionImageLayout(*device, *fontImage, vk::Format::eR8G8B8A8Unorm,
      vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
      1 /*m_mipLevels*/); // eTransferDstOptimal?

  VKUtil::copyBufferToImage(*device, *device->m_commandPool,
      device->m_transferQueue, *stagingBuffer, *fontImage,
      static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

  fontView = VKUtil::createImageView(*device, *fontImage,
      vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor, 1);

  VKUtil::transitionImageLayout(*device, *fontImage, vk::Format::eR8G8B8A8Unorm,
      vk::ImageLayout::eTransferDstOptimal,
      vk::ImageLayout::eShaderReadOnlyOptimal,
      1 /*m_mipLevels*/); // eTransferDstOptimal?

  // Font texture Sampler
  vk::SamplerCreateInfo samplerInfo{};
  samplerInfo.magFilter = vk::Filter::eLinear;
  samplerInfo.minFilter = vk::Filter::eLinear;
  samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
  samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
  samplerInfo.addressModeV = samplerInfo.addressModeU;
  samplerInfo.addressModeW = samplerInfo.addressModeU;
  samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
  sampler = device->device().createSamplerUnique(samplerInfo);

  // Descriptor pool
  vk::DescriptorPoolSize descriptorPoolSize{};
  descriptorPoolSize.type = vk::DescriptorType::eCombinedImageSampler;
  descriptorPoolSize.descriptorCount = 1;

  vk::DescriptorPoolCreateInfo descriptorPoolInfo{};
  descriptorPoolInfo.poolSizeCount = 1;
  descriptorPoolInfo.pPoolSizes = &descriptorPoolSize;
  descriptorPoolInfo.maxSets = 2;
  descriptorPool =
      device->device().createDescriptorPoolUnique(descriptorPoolInfo);

  // Descriptor set layout
  vk::DescriptorSetLayoutBinding setLayoutBinding{};
  setLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
  setLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
  setLayoutBinding.binding = 0;
  setLayoutBinding.descriptorCount = 1;

  vk::DescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo{};
  descriptorLayoutCreateInfo.bindingCount = 1;
  descriptorLayoutCreateInfo.pBindings = &setLayoutBinding;
  descriptorSetLayout = device->device().createDescriptorSetLayoutUnique(
      descriptorLayoutCreateInfo);

  // Descriptor set
  vk::DescriptorSetAllocateInfo allocInfo{};
  allocInfo.descriptorPool = *descriptorPool;
  allocInfo.pSetLayouts = &*descriptorSetLayout;
  allocInfo.descriptorSetCount = 1;
  descriptorSet = device->device().allocateDescriptorSets(allocInfo).front();

  vk::DescriptorImageInfo descriptorImageInfo{};
  descriptorImageInfo.sampler = *sampler;
  descriptorImageInfo.imageView = *fontView;
  descriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

  vk::WriteDescriptorSet writeDescriptorSet{};
  writeDescriptorSet.dstSet = descriptorSet;
  writeDescriptorSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
  writeDescriptorSet.descriptorCount = 1;
  writeDescriptorSet.dstBinding = 0;
  writeDescriptorSet.pImageInfo = &descriptorImageInfo;
  device->device().updateDescriptorSets(1, &writeDescriptorSet, 0, nullptr);
}

/** Prepare a separate pipeline for the UI overlay rendering decoupled from the
 * main application */
void UIOverlay::preparePipeline(const vk::RenderPass renderPass)
{
  Shader vertShader{
      *device, "../assets/uioverlay.vert.spv", Shader::ShaderType::VERTEX};
  Shader fragShader{
      *device, "../assets/uioverlay.frag.spv", Shader::ShaderType::FRAGMENT};

  std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages{
      vertShader.shaderCI(), fragShader.shaderCI()};

  // Pipeline layout
  // Push constants for UI rendering parameters
  vk::PushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(PushConstBlock);

  vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
  pipelineLayoutCreateInfo.setLayoutCount = 1;
  pipelineLayoutCreateInfo.pSetLayouts = &*descriptorSetLayout;
  pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
  pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
  pipelineLayout =
      device->device().createPipelineLayoutUnique(pipelineLayoutCreateInfo);

  // Setup graphics pipeline for UI rendering
  vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
  inputAssemblyStateCreateInfo.topology = vk::PrimitiveTopology::eTriangleList;

  vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
  rasterizationStateCreateInfo.polygonMode = vk::PolygonMode::eFill;
  rasterizationStateCreateInfo.cullMode = vk::CullModeFlagBits::eNone;
  rasterizationStateCreateInfo.frontFace = vk::FrontFace::eCounterClockwise;
  rasterizationStateCreateInfo.lineWidth = 1.0f;

  // Enable blending
  vk::PipelineColorBlendAttachmentState blendAttachmentState{};
  blendAttachmentState.blendEnable = VK_TRUE;
  blendAttachmentState.colorWriteMask =
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
  blendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
  blendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
  blendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
  blendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
  blendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eZero;
  blendAttachmentState.alphaBlendOp = vk::BlendOp::eAdd;

  vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
  colorBlendStateCreateInfo.attachmentCount = 1;
  colorBlendStateCreateInfo.pAttachments = &blendAttachmentState;

  vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
  depthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eAlways;

  vk::PipelineViewportStateCreateInfo viewportStateCreateInfo{};
  viewportStateCreateInfo.viewportCount = 1;
  viewportStateCreateInfo.scissorCount = 1;

  vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
  multisampleStateCreateInfo.rasterizationSamples = rasterizationSamples;

  std::vector<vk::DynamicState> dynamicStateEnables = {
      vk::DynamicState::eViewport, vk::DynamicState::eScissor};

  vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
  dynamicStateCreateInfo.dynamicStateCount =
      static_cast<std::uint32_t>(dynamicStateEnables.size());
  dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();

  vk::GraphicsPipelineCreateInfo pipelineCreateInfo{};
  pipelineCreateInfo.layout = *pipelineLayout;
  pipelineCreateInfo.renderPass = renderPass;
  pipelineCreateInfo.basePipelineIndex = -1;

  pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
  pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
  pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
  pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
  pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
  pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
  pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
  pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  pipelineCreateInfo.pStages = shaderStages.data();
  pipelineCreateInfo.subpass = subpass;

  // Vertex bindings an attributes based on ImGui vertex definition
  vk::VertexInputBindingDescription vertexInputBindings{};
  vertexInputBindings.binding = 0;
  vertexInputBindings.stride = sizeof(ImDrawVert);
  vertexInputBindings.inputRate = vk::VertexInputRate::eVertex;

  vk::VertexInputAttributeDescription posAttDesc{};
  posAttDesc.location = 0;
  posAttDesc.binding = 0;
  posAttDesc.format = vk::Format::eR32G32Sfloat;
  posAttDesc.offset = offsetof(ImDrawVert, pos);

  vk::VertexInputAttributeDescription uvAttDesc{};
  uvAttDesc.location = 1;
  uvAttDesc.binding = 0;
  uvAttDesc.format = vk::Format::eR32G32Sfloat;
  uvAttDesc.offset = offsetof(ImDrawVert, uv);

  vk::VertexInputAttributeDescription norAttDesc{};
  norAttDesc.location = 2;
  norAttDesc.binding = 0;
  norAttDesc.format = vk::Format::eR8G8B8A8Unorm;
  norAttDesc.offset = offsetof(ImDrawVert, col);

  std::vector<vk::VertexInputAttributeDescription> vertexInputAttributes = {
      posAttDesc, uvAttDesc, norAttDesc};

  vk::PipelineVertexInputStateCreateInfo vertexInputState{};
  vertexInputState.vertexBindingDescriptionCount = 1;
  vertexInputState.pVertexBindingDescriptions = &vertexInputBindings;
  vertexInputState.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(vertexInputAttributes.size());
  vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

  pipelineCreateInfo.pVertexInputState = &vertexInputState;

  pipeline = device->device().createGraphicsPipelineUnique(
      vk::PipelineCache{}, pipelineCreateInfo);
}

/** Update vertex and index buffer containing the imGui elements when required
 */
bool UIOverlay::update()
{
  ImDrawData* imDrawData = ImGui::GetDrawData();
  bool updateCmdBuffers = false;

  if (!imDrawData) {
    return false;
  };
  
  // Note: Alignment is done inside buffer creation
  vk::DeviceSize vertexBufferSize =
      imDrawData->TotalVtxCount * sizeof(ImDrawVert);
  vk::DeviceSize indexBufferSize =
      imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

  // Update buffers only if vertex or index count has been changed compared to
  // current buffer size
  if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
    return false;
  }

  // Vertex buffer
  if ((!vertexBuffer.m_buffer) || (vertexCount != imDrawData->TotalVtxCount)) {
    vertexBuffer.unmap();
    /// vertexBuffer.destroy();
    vk::BufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    bufferCreateInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
    bufferCreateInfo.size = vertexBufferSize;
    vertexBuffer.m_buffer =
        device->device().createBufferUnique(bufferCreateInfo);
    vertexBuffer.generate(vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible, vertexBufferSize);
    vertexCount = imDrawData->TotalVtxCount;
    vertexBuffer.unmap();
    vertexBuffer.map();
    updateCmdBuffers = true;
  }
  /// indexBuffer.destroy();
  //  vk::BufferUsageFlagBits::eIndexBuffer,
  //     vk::MemoryPropertyFlagBits::eHostVisible
  // Index buffer
  VkDeviceSize indexSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);
  if ((!indexBuffer.m_buffer) || (indexCount < imDrawData->TotalIdxCount)) {
    indexBuffer.unmap();
    vk::BufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    bufferCreateInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer;
    bufferCreateInfo.size = indexBufferSize;

    indexBuffer.m_buffer =
        device->device().createBufferUnique(bufferCreateInfo);
    indexBuffer.generate(vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible, indexBufferSize);

    indexCount = imDrawData->TotalIdxCount;
    indexBuffer.map();
    updateCmdBuffers = true;
  }

  // Upload data
  ImDrawVert* vtxDst = (ImDrawVert*) vertexBuffer.m_mapped;
  ImDrawIdx* idxDst = (ImDrawIdx*) indexBuffer.m_mapped;

  for (int n = 0; n < imDrawData->CmdListsCount; n++) {
    const ImDrawList* cmd_list = imDrawData->CmdLists[n];
    memcpy(vtxDst, cmd_list->VtxBuffer.Data,
        cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
    memcpy(idxDst, cmd_list->IdxBuffer.Data,
        cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
    vtxDst += cmd_list->VtxBuffer.Size;
    idxDst += cmd_list->IdxBuffer.Size;
  }

  // Flush to make writes visible to GPU
  vertexBuffer.flush();
  indexBuffer.flush();

  return updateCmdBuffers;
}

void UIOverlay::draw(std::uint32_t width, std::uint32_t height,
    const vk::CommandBuffer commandBuffer)
{
  ImDrawData* imDrawData = ImGui::GetDrawData();
  int32_t vertexOffset = 0;
  int32_t indexOffset = 0;

  if ((!imDrawData) || (imDrawData->CmdListsCount == 0)) {
    return;
  }

  ImGuiIO& io = ImGui::GetIO();

  vk::Viewport viewport;
  viewport.width = (float) width;
  viewport.height = (float) height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  //vk::Rect2D scissor{};
 // scissor.extent = vk::Extent2D{width, height};
//  commandBuffer.setViewport(0, 1, &viewport);
 // commandBuffer.setScissor(0, 1, &scissor);

  commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
  commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
      *pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

  pushConstBlock.scale =
      glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
  pushConstBlock.translate = glm::vec2(-1.0f);
  commandBuffer.pushConstants(*pipelineLayout, vk::ShaderStageFlagBits::eVertex,
      0, sizeof(PushConstBlock), &pushConstBlock);

  vk::DeviceSize offsets[1] = {0};
  commandBuffer.bindVertexBuffers(0, 1, &*vertexBuffer.m_buffer, offsets);
  commandBuffer.bindIndexBuffer(
      *indexBuffer.m_buffer, 0, vk::IndexType::eUint16);

  for (int32_t i = 0; i < imDrawData->CmdListsCount; i++) {
    const ImDrawList* cmd_list = imDrawData->CmdLists[i];
    for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
      const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
      vk::Rect2D scissorRect;
      scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
      scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
      scissorRect.extent.width =
          (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
      scissorRect.extent.height =
          (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
      commandBuffer.setScissor(0, 1, &scissorRect);
      commandBuffer.drawIndexed(
          pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
      indexOffset += pcmd->ElemCount;
    }
    vertexOffset += cmd_list->VtxBuffer.Size;
  }
}

void UIOverlay::resize(uint32_t width, uint32_t height)
{
  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize = ImVec2((float) (width), (float) (height));
}

void UIOverlay::freeResources()
{
  ImGui::DestroyContext();
  /*vertexBuffer.destroy();
  indexBuffer.destroy();
  vkDestroyImageView(device->logicalDevice, fontView, nullptr);
  vkDestroyImage(device->logicalDevice, fontImage, nullptr);
  vkFreeMemory(device->logicalDevice, fontMemory, nullptr);
  vkDestroySampler(device->logicalDevice, sampler, nullptr);
  vkDestroyDescriptorSetLayout(device->logicalDevice, descriptorSetLayout,
  nullptr); vkDestroyDescriptorPool(device->logicalDevice, descriptorPool,
  nullptr); vkDestroyPipelineLayout(device->logicalDevice, pipelineLayout,
  nullptr); vkDestroyPipeline(device->logicalDevice, pipeline, nullptr);*/
}

bool UIOverlay::header(const char* caption)
{
  return ImGui::CollapsingHeader(caption, ImGuiTreeNodeFlags_DefaultOpen);
}

bool UIOverlay::checkBox(const char* caption, bool* value)
{
  bool res = ImGui::Checkbox(caption, value);
  if (res) {
    updated = true;
  };
  return res;
}

bool UIOverlay::checkBox(const char* caption, int32_t* value)
{
  bool val = (*value == 1);
  bool res = ImGui::Checkbox(caption, &val);
  *value = val;
  if (res) {
    updated = true;
  };
  return res;
}

bool UIOverlay::inputFloat(
    const char* caption, float* value, float step, uint32_t precision)
{
  bool res = ImGui::InputFloat(caption, value, step, step * 10.0f, precision);
  if (res) {
    updated = true;
  };
  return res;
}

bool UIOverlay::sliderFloat(
    const char* caption, float* value, float min, float max)
{
  bool res = ImGui::SliderFloat(caption, value, min, max);
  if (res) {
    updated = true;
  };
  return res;
}

bool UIOverlay::sliderInt(
    const char* caption, int32_t* value, int32_t min, int32_t max)
{
  bool res = ImGui::SliderInt(caption, value, min, max);
  if (res) {
    updated = true;
  };
  return res;
}

bool UIOverlay::comboBox(
    const char* caption, int32_t* itemindex, std::vector<std::string> items)
{
  if (items.empty()) {
    return false;
  }
  std::vector<const char*> charitems;
  charitems.reserve(items.size());
  for (size_t i = 0; i < items.size(); i++) {
    charitems.push_back(items[i].c_str());
  }
  uint32_t itemCount = static_cast<uint32_t>(charitems.size());
  bool res =
      ImGui::Combo(caption, itemindex, &charitems[0], itemCount, itemCount);
  if (res) {
    updated = true;
  };
  return res;
}

bool UIOverlay::button(const char* caption)
{
  bool res = ImGui::Button(caption);
  if (res) {
    updated = true;
  };
  return res;
}

void UIOverlay::text(const char* formatstr, ...)
{
  va_list args;
  va_start(args, formatstr);
  ImGui::TextV(formatstr, args);
  va_end(args);
}
