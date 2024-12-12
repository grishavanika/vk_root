cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(vk_root)

# Vulkan
add_library(Vulkan_Integrated INTERFACE)
target_include_directories(Vulkan_Integrated INTERFACE
    "C:/VulkanSDK/1.3.296.0/Include")
target_link_libraries(Vulkan_Integrated INTERFACE
    "C:/VulkanSDK/1.3.296.0/Lib/vulkan-1.lib")
# GLFW
find_package(glfw3 CONFIG REQUIRED)
# GLM
find_package(glm CONFIG REQUIRED)

add_executable(vk_root main.cpp)
set_property(TARGET vk_root PROPERTY CXX_STANDARD 23)

target_link_libraries(vk_root PRIVATE Vulkan_Integrated)
target_link_libraries(vk_root PRIVATE glfw)
target_link_libraries(vk_root PRIVATE glm::glm)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "vk_root")
# For debugging: set working directory to the project's root
# so shaders/ path works
set_target_properties(
    vk_root PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
