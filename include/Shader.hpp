#pragma once

#include <filesystem>
#include <utility>

#include "Device.hpp"
#include "VKUtil.hpp"

class Shader
{
public:
  enum class ShaderType { VERTEX, FRAGMENT };

  Shader(
      Device& device, const std::filesystem::path& filename, ShaderType sType)
      : m_sType{sType}
  {
    auto data = VKUtil::getFileData(filename);
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.codeSize = data.size();
    createInfo.pCode = reinterpret_cast<std::uint32_t*>(data.data());
    m_module = device.device().createShaderModuleUnique(createInfo);
  }

  vk::ShaderModule getModule() const { return *m_module; }
  vk::PipelineShaderStageCreateInfo shaderCI()
  {
    vk::PipelineShaderStageCreateInfo createInfo{};
    if (m_sType == ShaderType::VERTEX) {
      createInfo.stage = vk::ShaderStageFlagBits::eVertex;
    } else if (m_sType == ShaderType::FRAGMENT) {
      createInfo.stage = vk::ShaderStageFlagBits::eFragment;
    }
    createInfo.module = *m_module;
    createInfo.pName = "main";
    return createInfo;
  }

private:
  ShaderType m_sType{};
  vk::UniqueShaderModule m_module{};
};