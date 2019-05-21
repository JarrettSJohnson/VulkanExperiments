#pragma once

#include "Cube.hpp"
#include <glm/glm.hpp>

struct Light {
  glm::vec3 pos = glm::vec3(2.f, 2.0f, 0.0f);
  glm::vec3 color = glm::vec3(0.2f, 0.3f, 0.7f);
};

struct CubedLight {
  Cube model;
  Light light{};
  constexpr static float scalef = 0.05f;
  inline static glm::vec3 scale = glm::vec3(scalef, scalef, scalef);
  CubedLight(Device& device) : model{device} {}
  glm::mat4 transform() const
  {
    return glm::translate(glm::scale(glm::mat4(1.0), scale), light.pos);
  }
};