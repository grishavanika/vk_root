cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(vk_root)

set(PREV_SOURCES
    main_begin.cpp
    main_00.cpp
    main_01.cpp
    main_02.cpp
    main_03.cpp
    main_04.cpp
    main_05.cpp
    main_06.cpp
    main_07.cpp
    main_08.cpp
    main_09.cpp
    main_10.cpp
    main_11.cpp
    main_12.cpp
    main_13.cpp
    main_14.cpp
    main_15.cpp
    main_16.cpp
    main_17.cpp
    main_18.cpp
    main_19.cpp
    main_20.cpp
    main_21.cpp
    main_22.cpp
    main_23.cpp
    main_24.cpp
    main_25.cpp
    main_26.cpp
    main_27.cpp
    main_28.cpp
    main_29.cpp
    main_30.cpp
)

add_executable(vk_root main.cpp ${PREV_SOURCES})
set_property(SOURCE ${PREV_SOURCES} PROPERTY VS_SETTINGS "ExcludedFromBuild=true")

# Vulkan
add_library(Vulkan_Integrated INTERFACE)
target_include_directories(Vulkan_Integrated INTERFACE
    "C:/VulkanSDK/1.4.309.0/Include")
target_link_libraries(Vulkan_Integrated INTERFACE
    "C:/VulkanSDK/1.4.309.0/Lib/vulkan-1.lib")
target_link_libraries(vk_root PRIVATE Vulkan_Integrated)
# GLFW
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(vk_root PRIVATE glfw)
# GLM
find_package(glm CONFIG REQUIRED)
target_link_libraries(vk_root PRIVATE glm::glm)
# STB
find_package(Stb REQUIRED)
target_include_directories(vk_root PRIVATE ${Stb_INCLUDE_DIR})
# tinyobjloader
find_package(tinyobjloader CONFIG REQUIRED)
target_link_libraries(vk_root PRIVATE tinyobjloader::tinyobjloader)

set_property(TARGET vk_root PROPERTY CXX_STANDARD 23)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "vk_root")
# For debugging: set working directory to the project's root
# so shaders/ path works
set_target_properties(
    vk_root PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
