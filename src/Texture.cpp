#pragma once

#include <filesystem>
#include <utility>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Texture.hpp"
#include "VKUtil.hpp"


STB_Image::STB_Image(const std::filesystem::path& filename)
{
    int channels;
    m_data = stbi_load(filename.string().c_str(), &m_width, &m_height,
        &channels, STBI_rgb_alpha);
    if (!m_data) {
      throw std::runtime_error{"Could not load texture!"};
    }
 }

STB_Image::STB_Image(STB_Image&& other) noexcept
{
  m_data = std::exchange(other.m_data, nullptr);
  m_width = std::exchange(other.m_width, 0);
  m_height = std::exchange(other.m_height, 0);
};
STB_Image& STB_Image::operator=(STB_Image&& other) noexcept
{
  m_data = std::exchange(other.m_data, nullptr);
  m_width = std::exchange(other.m_width, 0);
  m_height = std::exchange(other.m_height, 0);
  return *this;
};
STB_Image::~STB_Image() { stbi_image_free(m_data); }
