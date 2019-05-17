#pragma once

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

class Window
{
public:
  Window() = default;
  inline static bool framebufferResized = false;
  Window(int width, int height) : m_width{width}, m_height{height}
  {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_window = glfwCreateWindow(m_width, m_height, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(m_window, this);
    // glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
    glfwSetFramebufferSizeCallback(
        m_window, [](GLFWwindow* window, int width, int height) {
          auto app =
              reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
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
  Window(Window&& other) noexcept {
	m_window = std::exchange(other.m_window, nullptr);
    m_width = std::exchange(other.m_width, 0);
    m_height = std::exchange(other.m_height, 0);
  };
  Window& operator=(Window&& other) noexcept
  {
    m_window = std::exchange(other.m_window, nullptr);
    m_width = std::exchange(other.m_width, 0);
    m_height = std::exchange(other.m_height, 0);
    return *this;
  };
  ~Window() { glfwDestroyWindow(m_window);  };

  std::pair<int, int> getSize() {
    int width, height;
    glfwGetWindowSize(m_window, &width, &height);
	return std::make_pair(width, height);
  }

  auto shouldClose()
  {
    return glfwWindowShouldClose(m_window);
  }

  vk::UniqueSurfaceKHR createSurface(vk::Instance instance) {
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
};