#ifndef GL_INIT_H
#define GL_INIT_H

// glad must be included before GLFW.
#include <glad/glad.h>

#include "common.hpp"

#include <GLFW/glfw3.h>

#include <cstdio>

GLuint VAO, VBO, FBO, texA, texB, spheresSSBO, trianglesSSBO;

GLuint raytrace_shader_id;
GLuint display_shader_id;

int count = 0;

bool new_res = false;
int new_width, new_height;

void res_changed(GLFWwindow *window, int width, int height) {
    new_res = true;
    new_width = width;
    new_height = height;
}

void set_resolution(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
    if (raytrace_shader_id == 0)
        return;

    glUseProgram(raytrace_shader_id);
    GLuint resolution = glGetUniformLocation(raytrace_shader_id, "resolution");
    glUniform2f(resolution, width, height);
    glUniform1i(glGetUniformLocation(raytrace_shader_id, "prevFrame"), 0);

    glUseProgram(display_shader_id);
    resolution = glGetUniformLocation(display_shader_id, "resolution");
    glUniform2f(resolution, width, height);
    glUniform1i(glGetUniformLocation(display_shader_id, "image"), 0);

    count = 0;

    std::printf("%u, %u\n", width, height);

    if (texA != 0)
        glDeleteTextures(1, &texA);
    if (texB != 0)
        glDeleteTextures(1, &texB);

    if (FBO != 0)
        glDeleteFramebuffers(1, &FBO);

    glGenFramebuffers(1, &FBO);

    glGenTextures(1, &texA);
    glBindTexture(GL_TEXTURE_2D, texA);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA,
                 GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenTextures(1, &texB);
    glBindTexture(GL_TEXTURE_2D, texB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA,
                 GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           texA, 0);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           texB, 0);
    glClear(GL_COLOR_BUFFER_BIT);
}

void set_resolution(GLFWwindow *window) {
    int width, height;

    glfwGetFramebufferSize(window, &width, &height);

    set_resolution(window, width, height);
}

GLFWwindow *init() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window =
        glfwCreateWindow(WIDTH, HEIGHT, "Raytracer", nullptr, nullptr);
    if (window == nullptr) {
        std::printf("Failed to create GLFW window\n");
        glfwTerminate();
        // return -1;
        return nullptr;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::printf("Failed to initialize GLAD\n");
        // return -1;
        return nullptr;
    }

    set_resolution(window, WIDTH, HEIGHT);
    glfwSetFramebufferSizeCallback(window, res_changed);

    return window;
}

void load_buffers() {
    float vertices[] = {-1, -1, 3, -1, -1, 3};

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);
}

void terminate() { glfwTerminate(); }

#endif
