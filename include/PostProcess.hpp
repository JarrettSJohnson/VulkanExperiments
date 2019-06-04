#pragma once

#include <filesystem>

#include "Buffer.hpp"
#include "DescriptorSet.hpp"
#include "Device.hpp"
#include "Framebuffer.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"
#include "Shader.hpp"

class PostProcess
{
public:
  PostProcess(Device& device, const std::filesystem::path& VertShader,
      const std::filesystem::path& FragShader)
  {
    Shader vertShader{device, VertShader, Shader::ShaderType::VERTEX};
    Shader fragShader{device, FragShader, Shader::ShaderType::FRAGMENT};
  }

protected:
  Buffer m_vertexBuffer;
  RenderPass m_renderPass;
  DescriptorSet m_descriptorSet;
  Pipeline m_pipeline;
};
