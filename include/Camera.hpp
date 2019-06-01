#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
  glm::mat4 view() const { return glm::lookAt(m_pos, m_pos + m_dir, up); }
  glm::vec3 position() const { return m_pos; }
  void setDir(glm::vec3 d) { m_dir = d; }
  void setTar(glm::vec3 t) { m_tar = t; }
  glm::vec3 dir() const { return m_dir; }
  void translate(glm::vec3 trans) { m_pos += trans; }
  static constexpr glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

private:
  glm::vec3 m_pos = glm::vec3(0.0f, 0.0f, 3.0f);
  glm::vec3 m_tar = glm::vec3(0.0f, 0.0, 0.0f);
  glm::vec3 m_dir = glm::vec3(0.0f, 0.0f, -1.0f);
};