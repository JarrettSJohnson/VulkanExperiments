cmake_minimum_required (VERSION 3.10)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory("dep/glfw-3.3/")
find_package(Vulkan REQUIRED)

add_executable(VulkanTutorial
    src/main.cpp
    src/Application.cpp
    src/Device.cpp
    src/Model.cpp
    src/Texture.cpp
)

target_include_directories(VulkanTutorial PUBLIC
    include
    dep/glfw-3.3/include
    Vulkan::Vulkan
    dep/glm/
    dep/tinyobjloader/
    dep/stb/
)

set_target_properties(VulkanTutorial PROPERTIES
    CXX_STANDARD 17
    CXX_STANARD_REQUIRED OFF
    CXX_EXTENSIONS OFF
)

link_directories(VulkanTutorial PUBLIC
	#dep/glfw-3.3/src
)

target_link_libraries(VulkanTutorial
    glfw
    Vulkan::Vulkan
)
