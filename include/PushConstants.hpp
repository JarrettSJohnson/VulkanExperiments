#pragma once

#include "VKUtil.hpp"

struct PushConstants {
  glm::mat4 model;
};

struct LightUniforms {
  glm::mat4 projview;
  glm::vec4 viewPosition;
  glm::vec4 lightPosition;
  glm::vec4 lightColor;
};
