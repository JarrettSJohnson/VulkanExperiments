#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
  // glm::mat4 view() const { return glm::lookAt(m_pos, m_pos + m_dir, up); }
  glm::mat4 view()
  {
    glm::quat qPitch =
        glm::angleAxis(glm::radians(m_pitch), glm::vec3(-1.0f, 0.0f, 0.0f));
    glm::quat qYaw =
        glm::angleAxis(glm::radians(m_yaw), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat qRoll =
        glm::angleAxis(glm::radians(m_roll), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::quat orientation = /*qRoll **/ qPitch * qYaw;
    orientation = glm::normalize(orientation);
    glm::mat4 rotate = glm::mat4_cast(orientation);
    glm::mat4 translate(1.0f);
    translate = glm::translate(translate, -m_pos);
    return rotate * translate;
  }
  glm::vec3& position() { return m_pos; }
  void setDir(glm::vec3 d) { m_dir = d; }
  void setTar(glm::vec3 t) { m_tar = t; }
  glm::vec3 dir() const { return m_dir; }
  glm::vec3 up() const { return m_camUp; }
  void translate(glm::vec3 trans) { m_pos += trans; }
  void moveYawPitch(std::pair<float, float> dYawPitch)
  {
   
    float sensitivity = 0.05f;
    m_yaw += sensitivity * dYawPitch.first;
    m_pitch += sensitivity * dYawPitch.second;
    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    glm::vec3 newDir;
    newDir.x = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    newDir.y = sin(glm::radians(m_pitch));
    newDir.z = -cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_dir = glm::normalize(newDir);
    glm::vec3 newUp;
    newUp.x =
        cos(m_yaw) * sin(m_pitch) * sin(m_roll) - sin(m_yaw) * cos(m_roll);
    newUp.y =
        sin(m_yaw) * sin(m_pitch) * sin(m_roll) + cos(m_yaw) * cos(m_roll);
    newUp.z = cos(m_pitch) * sin(m_roll);
	m_camUp = glm::normalize(newUp);
  }
  void roll(float angle) {
	float sensitivity = 0.05f;
    m_roll += sensitivity * angle;
  }
  // glm::vec3 up = glm::normalize(glm::vec3(0.0f, 0.8f, 0.2f));
  static inline const glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 m_camUp = worldUp;

private:
  glm::vec3 m_pos = glm::vec3(-0.8714f, 1.431f, -0.4454);
  glm::vec3 m_tar = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 m_dir = glm::vec3(0.0f, 0.0f, -1.0f);

  

  float m_yaw{};
  float m_pitch{};
  float m_roll{};
};

    /*glm::mat4 mat(1.0f);
mat = glm::translate(mat, -m_pos);
//mat = glm::rotate(mat, glm::radians(roll), glm::vec3(0.0f, 0.0f, 1.0f));
mat = glm::rotate(mat, glm::radians(pitch), glm::vec3(-1.0f, 0.0f, 0.0f));
mat = glm::rotate(mat, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));*/

/*glm::vec3 front;
front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
front.y = sin(glm::radians(pitch));
front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
m_dir = glm::normalize(front);
glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
auto mat = glm::lookAt(m_pos, m_pos + m_dir, up);*/
// return mat;
/*glm::vec3 rr =
    glm::vec3(sin(glm::radians(m_roll)), cos(glm::radians(m_roll)), 0.0f);
glm::vec3 ww = glm::normalize(m_tar - m_pos);
glm::vec3 uu = glm::normalize(glm::cross(ww, rr));
glm::vec3 vv = glm::normalize(glm::cross(uu, ww));*/
// return glm::yawPitchRoll();
/* glm::quat qRoll =
    glm::angleAxis(glm::radians(m_roll), glm::vec3(0.0f, 0.0f, 1.0f));
 glm::quat orientation = qRoll;
 glm::mat4 rotate = glm::mat4_cast(orientation);
 return rotate * glm::lookAt(m_pos, m_pos + m_dir, m_camUp);*/