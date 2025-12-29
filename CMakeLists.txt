cmake_minimum_required(VERSION 3.16)
project(raytracer VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenGL REQUIRED)
find_package(glm CONFIG REQUIRED)

# add_subdirectory(external/glad)

add_subdirectory(external/glad)
add_executable(raytracer main.cpp)
target_include_directories(raytracer PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(raytracer
    glm::glm
    OpenGL::GL
    glfw
    glad
)

target_compile_options(raytracer PRIVATE -O3)

add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            ${CMAKE_BINARY_DIR}/assets
)

add_custom_target(copy_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/shaders
            ${CMAKE_BINARY_DIR}/shaders)