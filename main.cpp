#include <glad/glad.h>

#include "common.hpp"

#include <GLFW/glfw3.h>

#include "camera.cpp"
#include "obj.cpp"
#include "ppm.hpp"
#include "ray.cpp"
#include "shader.cpp"
#include "shape.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace glm;

GLuint VAO, VBO, FBO, texA, texB;

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

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
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

struct GPUMaterial {
    vec4 color_smoothness;
    vec4 emission_color_strength;
};

struct GPUSphere {
    vec4 position_radius;
    GPUMaterial material;
};

struct GPUCamera {
    vec4 position;
    vec4 direction;
};

void load_objects(GLFWwindow *window, const GPUCamera &camera,
                  std::vector<GPUSphere> spheres) {
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, spheres.size() * sizeof(GPUSphere),
                 spheres.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

    GLuint camera_position =
        glGetUniformLocation(raytrace_shader_id, "camera.position");
    glUniform4fv(camera_position, 1, glm::value_ptr(camera.position));

    GLuint camera_direction =
        glGetUniformLocation(raytrace_shader_id, "camera.direction");
    glUniform4fv(camera_direction, 1, glm::value_ptr(camera.direction));

    GLuint viewport_height =
        glGetUniformLocation(raytrace_shader_id, "viewport_height");
    glUniform1f(viewport_height, VIEWPORT_HEIGHT);

    set_resolution(window, WIDTH, HEIGHT);
}

void loop(GLFWwindow *window, Shader rayShader) {

    Shader displayShader("shaders/shader.vert", "shaders/display.frag");
    display_shader_id = displayShader.ID;

    displayShader.use();
    glUniform1i(glGetUniformLocation(display_shader_id, "image"), 0);

    set_resolution(window, WIDTH, HEIGHT);

    GLuint *texRead = &texA;
    GLuint *texWrite = &texB;

    while (!glfwWindowShouldClose(window)) {
        rayShader.use();
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, *texWrite, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, *texRead);
        rayShader.setInt("frameCount", count);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        std::swap(texRead, texWrite);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        displayShader.use();
        glBindTexture(GL_TEXTURE_2D, *texRead);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();

        count++;

        if (new_res) {
            new_res = false;
            set_resolution(window, new_width, new_height);
        }
    }
}

void terminate() { glfwTerminate(); }

int main() {
    GLFWwindow *window = init();
    if (window == nullptr)
        return -1;

    load_buffers();

    Shader shader("shaders/shader.vert", "shaders/raytrace.frag");
    shader.use();
    raytrace_shader_id = shader.ID;

    GPUMaterial m1{vec4(.1f, .2f, .8f, .0f), vec4(1.f, 1.f, 1.f, 0.f)};
    GPUSphere sphere1{vec4(0, 0, -10, 3), m1};

    GPUMaterial m2{vec4(.1f, .9f, .3f, 0.0f), vec4(1.f, 1.f, 1.f, 0.f)};
    GPUSphere sphere2{vec4(0, -100, -10, 97), m2};

    GPUMaterial m3{vec4(0, 0, 0, .0f), vec4(1.f, 1.f, 1.f, 6.f)};
    GPUSphere sphere3{vec4(100, 100, -100, 70), m3};

    std::vector<GPUSphere> spheres = {sphere1, sphere2, sphere3};

    shader.setInt("sphereCount", spheres.size());

    GPUCamera camera{{0, 0, 0, 0}, {0, 0, -1, 0}};

    load_objects(window, camera, spheres);

    loop(window, shader);

    terminate();
}

void main_old(int, char **) {

    HitList world;

    // // Red
    // Material m1{dvec3{0.8, .1, 0.2}, dvec3{1, 1, 1}, 0, .9};
    // Sphere sphere1{dvec3{0, 0, -10}, m1, 4};

    // // Green
    // Material m2{dvec3{.2, 0.8, 0.3}, dvec3{1, 1, 1}, 0, .9};
    // Sphere sphere2{dvec3{2, 8, -14}, m2, 4};

    // // // Blue
    // Material m3{dvec3{.2, 0.4, 0.7}, dvec3{1, 1, 1}, 0, .05};
    // Sphere sphere3{dvec3{0, -50, -7}, m3, 46};

    // // White
    // Material m4{dvec3{1, 1, 1}, dvec3{1, 1, 1}, 0, .9};
    // Sphere sphere4{dvec3{2, 8, -6}, m4, 4};

    // // Sun
    Material m5{dvec3{0, 0, 0}, dvec3{1, 1, 1}, 2, 0};
    Sphere sphere5{dvec3{0, 0, 0}, m5, 1};

    // world.add(&sphere1);
    // world.add(&sphere2);
    // world.add(&sphere3);
    // world.add(&sphere4);
    world.add(&sphere5);
    // world.add(&triangle);

    // Monkey
    Material m_monkey{{.2, .4, .7}, {0, 0, 0}, 0, .1};
    Mesh monkey = load_obj_triangles("assets/monkey.obj", m_monkey);
    monkey.set_position({0, 0, -3});
    // monkey.set_rotation({0, 1, 0}, quarter_pi<double>());

    world.add(&monkey);

    Camera camera{{-6, 0, 2}, {2, 0, -1}, 30};

    camera.render(world, 10, 1000);
}
