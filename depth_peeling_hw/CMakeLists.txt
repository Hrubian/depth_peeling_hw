# Set the minimum CMake version
cmake_minimum_required(VERSION 3.10)

project(depth_peeling_hw)

add_executable(depth_peeling_hw
        main.cpp
        renderer.hpp
        scene_definition.hpp
)
target_sources(depth_peeling_hw PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../glad/src/glad.c
)
target_link_libraries(depth_peeling_hw utils glm::glm glfw OpenGL::GL)
target_include_directories(depth_peeling_hw PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../glad/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../utils
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}
)
