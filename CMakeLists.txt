cmake_minimum_required(VERSION 3.10.0)
project(raytracer VERSION 0.1.0 LANGUAGES C CXX)

find_package(glm CONFIG REQUIRED)
add_executable(raytracer main.cpp)
target_link_libraries(raytracer PRIVATE glm::glm)

target_compile_options(raytracer PRIVATE -O3)

