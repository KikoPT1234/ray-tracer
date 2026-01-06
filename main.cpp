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

int shader_id;

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
    if (shader_id == 0)
        return;
    GLuint resolution = glGetUniformLocation(shader_id, "resolution");
    glUniform2f(resolution, width, height);
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

    glViewport(0, 0, WIDTH, HEIGHT);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    return window;

    // float vertices[] = {
    //     // positions        // colors         // texture coords
    //     0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right
    //     0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
    //     -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left
    //     -0.5f, 0.5f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f  // top left
    // };
    // unsigned int indices[] = {
    //     // note that we start from 0!
    //     3, 2, 0, // first triangle
    //     2, 1, 0  // second triangle
    // };

    // float texCoords[] = {
    //     0.0f, 0.0f, // lower-left corner
    //     1.0f, 0.0f, // lower-right corner
    //     0.5f, 1.0f  // top-center corner
    // };

    // unsigned int VAO;
    // glGenVertexArrays(1, &VAO);
    // glBindVertexArray(VAO);

    // unsigned int VBO;
    // glGenBuffers(1, &VBO);
    // glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
    // GL_STATIC_DRAW);

    // unsigned int EBO;
    // glGenBuffers(1, &EBO);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
    //              GL_STATIC_DRAW);

    // // position attribute
    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
    //                       (void *)0);
    // glEnableVertexAttribArray(0);

    // // color attribute
    // glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
    //                       (void *)(3 * sizeof(float)));
    // glEnableVertexAttribArray(1);

    // // texture attribute
    // glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
    //                       (void *)(6 * sizeof(float)));
    // glEnableVertexAttribArray(2);

    // Shader shader("shaders/shader.vert", "shaders/shader.frag");

    // unsigned int texture1;
    // glGenTextures(1, &texture1);
    // glBindTexture(GL_TEXTURE_2D, texture1);

    // // set the texture wrapping/filtering options (on the currently bound
    // // texture object)
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
    //                 GL_LINEAR_MIPMAP_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // // load and generate the texture
    // int width, height, nrChannels;
    // stbi_set_flip_vertically_on_load(true);
    // unsigned char *data =
    //     stbi_load("assets/container.jpg", &width, &height, &nrChannels, 0);
    // if (data) {
    //     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
    //                  GL_UNSIGNED_BYTE, data);
    //     glGenerateMipmap(GL_TEXTURE_2D);
    // } else {
    //     std::cout << "Failed to load texture" << std::endl;
    // }

    // unsigned int texture2;
    // glGenTextures(1, &texture2);
    // glBindTexture(GL_TEXTURE_2D, texture2);

    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
    //                 GL_LINEAR_MIPMAP_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // data = stbi_load("assets/awesomeface.png", &width, &height, &nrChannels,
    // 0); if (data) {
    //     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA,
    //                  GL_UNSIGNED_BYTE, data);
    //     glGenerateMipmap(GL_TEXTURE_2D);
    // }

    // stbi_image_free(data);

    // shader.use();
    // shader.setInt("texture1", 0);
    // shader.setInt("texture2", 1);

    // while (!glfwWindowShouldClose(window)) {
    //     processInput(window);

    //     glClearColor(.2f, .3f, .3f, 1.0f);
    //     glClear(GL_COLOR_BUFFER_BIT);

    //     // float timeValue = glfwGetTime();
    //     // float greenValue = (sin(timeValue) / 2.0f) + 0.5f;
    //     // int vertexColorLocation =
    //     //     glGetUniformLocation(shader_program, "ourColor");
    //     // glUniform4f(vertexColorLocation, 0.05f, greenValue, 0.0f, 1.0f);
    //     glActiveTexture(GL_TEXTURE0);
    //     glBindTexture(GL_TEXTURE_2D, texture1);
    //     glActiveTexture(GL_TEXTURE1);
    //     glBindTexture(GL_TEXTURE_2D, texture2);
    //     glBindVertexArray(VAO);
    //     // glDrawArrays(GL_TRIANGLES, 0, 3);
    //     glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    //     glBindVertexArray(0);

    //     glfwSwapBuffers(window);
    //     glfwPollEvents();
    // }

    // glDeleteVertexArrays(1, &VAO);
    // glDeleteBuffers(1, &VBO);

    // glfwTerminate();

    // return 0;
}

void load_buffers(GLuint *VAO, GLuint *VBO) {
    float vertices[] = {-1, -1, 3, -1, -1, 3};

    glGenVertexArrays(1, VAO);
    glBindVertexArray(*VAO);

    glGenBuffers(1, VBO);
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
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

    GLuint camera_position = glGetUniformLocation(shader_id, "camera.position");
    glUniform4fv(camera_position, 1, glm::value_ptr(camera.position));

    GLuint camera_direction =
        glGetUniformLocation(shader_id, "camera.direction");
    glUniform4fv(camera_direction, 1, glm::value_ptr(camera.direction));

    GLuint viewport_height = glGetUniformLocation(shader_id, "viewport_height");
    glUniform1f(viewport_height, VIEWPORT_HEIGHT);

    GLuint resolution = glGetUniformLocation(shader_id, "resolution");
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glUniform2f(resolution, width, height);
}

void loop(GLFWwindow *window, GLuint VAO) {
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void terminate() { glfwTerminate(); }

int main() {
    GLFWwindow *window = init();
    if (window == nullptr)
        return -1;

    GLuint VAO, VBO;
    load_buffers(&VAO, &VBO);

    Shader shader("shaders/shader.vert", "shaders/shader.frag");
    shader.use();
    shader_id = shader.ID;

    GPUMaterial m1 {
        vec4(.1f, .2f, .8f, .1f),
        vec4(1.f, 1.f, 1.f, 0.f)
    };
    GPUSphere sphere1 {
        vec4(0, 0, -10, 3),
        m1
    };

    GPUMaterial m2 {
        vec4(.1f, .9f, .3f, .1f),
        vec4(1.f, 1.f, 1.f, 0.f)
    };
    GPUSphere sphere2 {
        vec4(0, -100, -10, 97),
        m2
    };

    GPUMaterial m3 {
        vec4(0, 0, 0, .1f),
        vec4(1.f, 1.f, 1.f, 50.f)
    };
    GPUSphere sphere3 {
        vec4(100, 100, -100, 30),
        m3
    };

    std::vector<GPUSphere> spheres = {sphere1, sphere2, sphere3};

    shader.setInt("sphereCount", spheres.size());

    GPUCamera camera{{0, 0, 0, 0}, {0, 0, -1, 0}};

    load_objects(window, camera, spheres);

    loop(window, VAO);

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
