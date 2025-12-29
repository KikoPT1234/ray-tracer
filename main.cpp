#include <glad/glad.h>

#include "common.hpp"

#include <GLFW/glfw3.h>

#include "camera.cpp"
#include "obj.cpp"
#include "ppm.hpp"
#include "ray.cpp"
#include "shader.cpp"
#include "shape.cpp"

using namespace glm;

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int init() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window =
        glfwCreateWindow(WIDTH, HEIGHT, "Raytracer", nullptr, nullptr);
    if (window == nullptr) {
        std::printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::printf("Failed to initialize GLAD\n");
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    float vertices[] = {
        // positions        // colors
        0.5f,  -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom right
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // bottom left
        0.0f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f  // top
    };
    unsigned int indices[] = {
        // note that we start from 0!
        0,
        1,
        2, // first triangle
    };

    float texCoords[] = {
        0.0f, 0.0f, // lower-left corner
        1.0f, 0.0f, // lower-right corner
        0.5f, 1.0f  // top-center corner
    };

    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    unsigned int EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                 GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);

    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    Shader shader("shaders/shader.vert", "shaders/shader.frag");

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(.2f, .3f, .3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // float timeValue = glfwGetTime();
        // float greenValue = (sin(timeValue) / 2.0f) + 0.5f;
        // int vertexColorLocation =
        //     glGetUniformLocation(shader_program, "ourColor");

        shader.use();
        // glUniform4f(vertexColorLocation, 0.05f, greenValue, 0.0f, 1.0f);
        glBindVertexArray(VAO);
        // glDrawArrays(GL_TRIANGLES, 0, 3);
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glfwTerminate();

    return 0;
}

int main() { init(); }

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
