#pragma once

#define GLFW_INCLUDE_VULKAN

#include "Camera.hpp"
#include <GLFW/glfw3.h>
#include <iostream>

inline std::string printVec3(const glm::vec3 vec)
{
  std::string str = "(";
  str += std::to_string(vec.x);
  str += ", ";
  str += std::to_string(vec.y);
  str += ", ";
  str += std::to_string(vec.z);
  str += ")";
  return str;
}

class Window
{
public:
  Window() = default;
  inline static bool framebufferResized = false;
  Window(int width, int height) : m_width{width}, m_height{height}
  {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    m_window = glfwCreateWindow(m_width, m_height, "Vulkan", nullptr, nullptr);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetCursorPosCallback(
        m_window, [](GLFWwindow* glfwWin, double xpos, double ypos) {
          auto window =
              reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWin));
          window->processMouseDelta(xpos, ypos);
        });
    glfwSetFramebufferSizeCallback(
        m_window, [](GLFWwindow* window, int width, int height) {
          auto app = static_cast<Window*>(glfwGetWindowUserPointer(window));
          Window::framebufferResized = true;
          // app->recreateSwapchain();
          // app->framebufferResized = true;
        });
    if (m_window == nullptr) {
      throw std::runtime_error("failed to create glfw window");
    }
  }
  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;
  Window(Window&& other) noexcept
  {
    m_window = std::exchange(other.m_window, nullptr);
    m_width = std::exchange(other.m_width, 0);
    m_height = std::exchange(other.m_height, 0);
    glfwSetWindowUserPointer(m_window, this);
  };
  Window& operator=(Window&& other) noexcept
  {
    m_window = std::exchange(other.m_window, nullptr);
    m_width = std::exchange(other.m_width, 0);
    m_height = std::exchange(other.m_height, 0);
    glfwSetWindowUserPointer(m_window, this);
    return *this;
  };
  ~Window() { glfwDestroyWindow(m_window); };

  void processInputWindow()
  {
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(m_window, true);
  }

  void processInputCamera(Camera& camera)
  {
    float cameraSpeed = 0.05f; // adjust accordingly
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
      camera.translate(cameraSpeed * camera.dir());
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
      camera.translate(-cameraSpeed * camera.dir());
    if (glfwGetKey(m_window, GLFW_KEY_R) == GLFW_PRESS)
      camera.translate(cameraSpeed * camera.up());
    if (glfwGetKey(m_window, GLFW_KEY_T) == GLFW_PRESS)
      camera.translate(-cameraSpeed * camera.up());
   // if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS)
   //   camera.roll(5);
   // if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS)
   //   camera.roll(-5);
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
      camera.translate(
          -glm::normalize(glm::cross(camera.dir(), camera.up())) * cameraSpeed);
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
      camera.translate(
          glm::normalize(glm::cross(camera.dir(), camera.up())) * cameraSpeed);
    if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS)
      std::cout << printVec3(camera.position()) << std::endl;
  }
  void processMouseDelta(double _xpos, double _ypos)
  {
    float xpos = static_cast<float>(_xpos);
    float ypos = static_cast<float>(_ypos);
    if (firstMouse) {
      lastCoord = std::make_pair(xpos, ypos);
      firstMouse = false;
    }
    offset = std::make_pair(xpos - lastCoord.first, lastCoord.second - ypos);
    lastCoord = std::make_pair(xpos, ypos);

    /*float sensitivity = 0.05f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    pitch = std::clamp(pitch, -89.0f, 89.0f);*/
  }

  /*glm::vec3 getNewDir()
  {
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    return glm::normalize(front);
    // camera.setDir(glm::normalize(front));
  }*/
  std::pair<float, float> getMouseOffset()
  {
    auto ret = offset;
    offset = std::make_pair(0.0f, 0.0f);
	return ret;
  }

  std::pair<int, int> getSize()
  {
    int width, height;
    glfwGetWindowSize(m_window, &width, &height);
    return std::make_pair(width, height);
  }

  auto shouldClose() { return glfwWindowShouldClose(m_window); }

  vk::UniqueSurfaceKHR createSurface(vk::Instance instance)
  {
    VkSurfaceKHR surfacetmp;
    if (glfwCreateWindowSurface(instance, m_window, nullptr, &surfacetmp) !=
        VK_SUCCESS) {
      throw std::runtime_error("failed to create surface");
    }
    return vk::UniqueSurfaceKHR(surfacetmp, instance);
  }

private:
  GLFWwindow* m_window{};
  int m_width{};
  int m_height{};

  bool firstMouse = true;
  std::pair<float, float> offset;
  std::pair<float, float> lastCoord;
  //float lastX = 800.0f / 2.0;
  //float lastY = 600.0 / 2.0;
  //float pitch = 0.0f;
  //float yaw = -90.0f;
};
