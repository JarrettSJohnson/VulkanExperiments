#pragma once

#include <filesystem>
#include <utility>

#include "VKUtil.hpp"
#include "Device.hpp"
#include "Shader.hpp"
#include "Swapchain.hpp"
#include "UBO.hpp"

class Pipeline
{
public:
  Pipeline() = default;
  Pipeline(Device& device, Swapchain& swapchain, vk::RenderPass& renderPass,
      vk::DescriptorSetLayout& descSetLayout, Shader& vertShader, Shader& fragShader)
  {

    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages{
        vertShader.shaderCI(), fragShader.shaderCI()};

    vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo{}; // empty?
    auto bindingDescriptions = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputCreateInfo.vertexBindingDescriptionCount =
        static_cast<std::uint32_t>(bindingDescriptions.size());
    vertexInputCreateInfo.pVertexBindingDescriptions =
        bindingDescriptions.data();
    vertexInputCreateInfo.vertexAttributeDescriptionCount =
        static_cast<std::uint32_t>(attributeDescriptions.size());
    vertexInputCreateInfo.pVertexAttributeDescriptions =
        attributeDescriptions.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
    inputAssemblyStateCreateInfo.topology =
        vk::PrimitiveTopology::eTriangleList;

    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain.extent().width);
    viewport.height = static_cast<float>(swapchain.extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissors{};
    scissors.offset = {0, 0};
    scissors.extent = swapchain.extent();

    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo{};
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissors;

    vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
    rasterizationStateCreateInfo.polygonMode = vk::PolygonMode::eFill;
    rasterizationStateCreateInfo.lineWidth = 1.0f;
    rasterizationStateCreateInfo.cullMode = vk::CullModeFlagBits::eBack;
    rasterizationStateCreateInfo.frontFace = vk::FrontFace::eCounterClockwise;

    vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
    multisampleStateCreateInfo.rasterizationSamples = device.m_msaaSamples;

    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
    depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eLess;
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
    colorBlendStateCreateInfo.logicOp = vk::LogicOp::eCopy;
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachment;

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descSetLayout;
    m_pipelineLayout =
        device.device().createPipelineLayoutUnique(pipelineLayoutCreateInfo);

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
    graphicsPipelineCreateInfo.layout = *m_pipelineLayout;
    graphicsPipelineCreateInfo.renderPass = renderPass;

    m_graphicsPipeline = device.device().createGraphicsPipelineUnique(
        vk::PipelineCache{}, graphicsPipelineCreateInfo);
  };

  vk::Pipeline pipeline() const { return *m_graphicsPipeline; }
  vk::PipelineLayout layout() const { return *m_pipelineLayout; }
  private:
  vk::UniquePipelineLayout m_pipelineLayout{};
  vk::UniquePipeline m_graphicsPipeline{};
};