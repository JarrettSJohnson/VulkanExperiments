#pragma once

#include <filesystem>
#include <optional>
#include <utility>

#include "Device.hpp"
#include "PushConstants.hpp"
#include "Shader.hpp"
#include "Swapchain.hpp"
#include "UBO.hpp"
#include "VKUtil.hpp"

class PipelineLayout
{
public:
  PipelineLayout() = default;
  PipelineLayout(Device& device, Swapchain& swapchain,
      std::optional<vk::DescriptorSetLayout> oDescSetLayout = std::nullopt,
      std::optional<vk::PushConstantRange> oPushConstantRange = std::nullopt)
  {
    vk::PushConstantRange pushConstantRange{};
    std::uint32_t pushConstantRangeCount{};
    const vk::PushConstantRange* pPush{};
    if (oPushConstantRange) {
      // oPushConstantRange->stageFlags =
      //    vk::ShaderStageFlagBits::eVertex |
      //    vk::ShaderStageFlagBits::eFragment;
      // oPushConstantRange->size = sizeof(PushConstants);
      pushConstantRangeCount = 1;
      pPush = &oPushConstantRange.value();
    }

    vk::DescriptorSetLayout descSetLayout{};
    std::uint32_t setLayoutCount{};
    const vk::DescriptorSetLayout* pDescSetLayout{};
    if (oDescSetLayout) {
      setLayoutCount = 1;
      pDescSetLayout = &oDescSetLayout.value();
    }

    vk::PipelineLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.setLayoutCount = setLayoutCount;
    layoutCreateInfo.pSetLayouts = pDescSetLayout;
    layoutCreateInfo.pushConstantRangeCount = pushConstantRangeCount;
    layoutCreateInfo.pPushConstantRanges = pPush;
    m_layout = device.device().createPipelineLayoutUnique(layoutCreateInfo);
  }
  vk::PipelineLayout layout() const { return *m_layout; }

private:
  vk::UniquePipelineLayout m_layout{};
};

class Pipeline
{
public:
  Pipeline() = default;
  Pipeline(
      Device& device, vk::Extent2D extent, vk::SampleCountFlagBits sampleCount)
  {
    inputAssemblyStateCreateInfo.topology =
        vk::PrimitiveTopology::eTriangleList;

    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    scissors.offset = {{0, 0}};
    scissors.extent = extent;

    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissors;

    rasterizationStateCreateInfo.polygonMode = vk::PolygonMode::eFill;
    rasterizationStateCreateInfo.lineWidth = 1.0f;
    rasterizationStateCreateInfo.cullMode = vk::CullModeFlagBits::eBack;
    rasterizationStateCreateInfo.frontFace = vk::FrontFace::eCounterClockwise;

    multisampleStateCreateInfo.rasterizationSamples = sampleCount;

    depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eLess;
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;

    colorBlendAttachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_FALSE;

    colorBlendStateCreateInfo.logicOp = vk::LogicOp::eCopy;
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachment;
  };

  template <typename VertexType> void addVertexDescription()
  {

    bindingDescriptions = VertexType::getBindingDescription();
    attributeDescriptions = VertexType::getAttributeDescriptions();

    vertexInputCreateInfo.vertexBindingDescriptionCount =
        static_cast<std::uint32_t>(bindingDescriptions.size());
    vertexInputCreateInfo.pVertexBindingDescriptions =
        bindingDescriptions.data();
    vertexInputCreateInfo.vertexAttributeDescriptionCount =
        static_cast<std::uint32_t>(attributeDescriptions.size());
    vertexInputCreateInfo.pVertexAttributeDescriptions =
        attributeDescriptions.data();
  }

  void changeRasterizationFullscreenTriangle()
  {
    rasterizationStateCreateInfo.cullMode = vk::CullModeFlagBits::eFront;
    rasterizationStateCreateInfo.frontFace = vk::FrontFace::eCounterClockwise;
  }

  void generate(const Device& device, PipelineLayout& layout,
      RenderPass& renderPass, Shader& vertShader, Shader& fragShader)
  {
    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages{
        vertShader.shaderCI(), fragShader.shaderCI()};

    vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
    graphicsPipelineCreateInfo.stageCount =
        static_cast<std::uint32_t>(shaderStages.size());
    graphicsPipelineCreateInfo.pStages = shaderStages.data();
    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState =
        &inputAssemblyStateCreateInfo;
    graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    graphicsPipelineCreateInfo.pRasterizationState =
        &rasterizationStateCreateInfo;
    graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    graphicsPipelineCreateInfo.pDepthStencilState =
        &depthStencilStateCreateInfo;
    graphicsPipelineCreateInfo.layout = layout.layout();
    graphicsPipelineCreateInfo.renderPass = renderPass.renderpass();
    m_graphicsPipeline = device.device().createGraphicsPipelineUnique(
        vk::PipelineCache{}, graphicsPipelineCreateInfo);
  }

  vk::Pipeline pipeline() const { return *m_graphicsPipeline; }

private:
  vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo{}; // empty?
  std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
  std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
  vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
  vk::Viewport viewport{};
  vk::Rect2D scissors{};
  vk::PipelineViewportStateCreateInfo viewportStateCreateInfo{};
  vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
  vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
  vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
  vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
  vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};

  vk::UniquePipeline m_graphicsPipeline{};
};
