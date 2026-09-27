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

GLuint VAO, VBO, FBO, texA, texB, spheresSSBO, trianglesSSBO;

GLuint raytrace_shader_id;
GLuint display_shader_id;

int count = 0;

bool new_res = false;
int new_width, new_height;

struct GPUMaterial {
    vec4 color_smoothness;
    vec4 emission_color_strength;
    vec4 glass_absorption_ior;
    vec4 type;
};

struct GPUSphere {
    vec4 position_radius;
    GPUMaterial material;
};

struct GPUTriangle {
    vec4 v1;
    vec4 v2;
    vec4 v3;

    vec4 n1;
    vec4 n2;
    vec4 n3;
    GPUMaterial material;
};

struct GPUCamera {
    vec4 position;
    vec4 direction_fov;
};

GPUCamera camera;

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

void load_camera() {
    glUseProgram(raytrace_shader_id);
    GLuint camera_position =
        glGetUniformLocation(raytrace_shader_id, "camera.position");
    glUniform4fv(camera_position, 1, glm::value_ptr(camera.position));

    GLuint camera_direction =
        glGetUniformLocation(raytrace_shader_id, "camera.direction_fov");
    glUniform4fv(camera_direction, 1, glm::value_ptr(camera.direction_fov));
}

void load_objects(GLFWwindow *window, const GPUCamera &camera,
                  std::vector<GPUSphere> spheres,
                  std::vector<GPUTriangle> triangles) {
    glGenBuffers(1, &spheresSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, spheresSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, spheres.size() * sizeof(GPUSphere),
                 spheres.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, spheresSSBO);

    glGenBuffers(1, &trianglesSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, trianglesSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 triangles.size() * sizeof(GPUTriangle), triangles.data(),
                 GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, trianglesSSBO);

    load_camera();

    GLuint viewport_height =
        glGetUniformLocation(raytrace_shader_id, "viewport_height");
    glUniform1f(viewport_height, VIEWPORT_HEIGHT);

    set_resolution(window, WIDTH, HEIGHT);
}

bool update_pos(GLFWwindow *window, float delta_time) {
    float speed = 3 * delta_time;
    vec3 world_up = vec3(0, 1, 0);
    vec3 direction = camera.direction_fov;
    vec3 right = normalize(cross(direction, world_up));
    vec3 forward = cross(world_up, right);
    float fov = camera.direction_fov.w;

    float angular_speed = two_pi<float>() * 0.25 * delta_time * fov / 80;

    bool out = false;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
        speed *= 5;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT)) {
        angular_speed *= 3;
    }

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        camera.direction_fov.w /= 1.002f;
        out = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        camera.direction_fov.w *= 1.002f;
        out = true;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.position += vec4(forward * speed, 0);
        out = true;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.position += vec4(-forward * speed, 0);
        out = true;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.position += vec4(right * speed, 0);
        out = true;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.position += vec4(-right * speed, 0);
        out = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera.position += vec4(world_up * speed, 0);
        out = true;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        camera.position += vec4(-world_up * speed, 0);
        out = true;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        mat4 identity = glm::identity<mat4>();
        camera.direction_fov =
            vec4((vec3)(glm::rotate(identity, angular_speed, vec3(0, 1, 0)) *
                        vec4((vec3)camera.direction_fov, 0.0)),
                 camera.direction_fov.w);
        out = true;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        mat4 identity = glm::identity<mat4>();
        camera.direction_fov =
            vec4((vec3)(glm::rotate(identity, -angular_speed, vec3(0, 1, 0)) *
                        vec4((vec3)camera.direction_fov, 0.0)),
                 camera.direction_fov.w);
        out = true;
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        mat4 identity = glm::identity<mat4>();
        camera.direction_fov =
            vec4((vec3)(glm::rotate(identity, angular_speed, right) *
                        vec4((vec3)camera.direction_fov, 0.0)),
                 camera.direction_fov.w);
        out = true;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        mat4 identity = glm::identity<mat4>();
        camera.direction_fov =
            vec4((vec3)(glm::rotate(identity, -angular_speed, right) *
                        vec4((vec3)camera.direction_fov, 0.0)),
                 camera.direction_fov.w);
        out = true;
    }

    return out;
}

void loop(GLFWwindow *window, Shader rayShader) {

    Shader displayShader("shaders/shader.vert", "shaders/display.frag");
    display_shader_id = displayShader.ID;

    displayShader.use();
    glUniform1i(glGetUniformLocation(display_shader_id, "image"), 0);

    set_resolution(window, WIDTH, HEIGHT);

    GLuint *texRead = &texA;
    GLuint *texWrite = &texB;

    float delta_time;
    float last_frame;

    while (!glfwWindowShouldClose(window)) {

        float current_frame = glfwGetTime();
        delta_time = current_frame - last_frame;
        last_frame = current_frame;

        if (update_pos(window, delta_time)) {
            count = 0;
            load_camera();
        }

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

    GPUMaterial m1{vec4(1.f, 1.f, 1.f, 1.f), vec4(1.f, 1.f, 1.f, 0.f),
                   vec4(0.05f, 0.05f, 0.05f, 1.5f), vec4(1, 0, 0, 0)};

    vec3 red_color = rgb_to_vec3(240, 101, 101);
    vec3 green_color = rgb_to_vec3(88, 224, 108);
    vec3 blue_color = rgb_to_vec3(29, 102, 219);

    GPUMaterial red_wall{vec4(red_color, .2f), vec4(1.f, 1.f, 1.f, 0.f),
                         vec4(0.f, 0.f, 0.f, 1.f), vec4(0, 0, 0, 0)};

    GPUMaterial green_wall{vec4(green_color, .2f),
                           vec4(1.f, 1.f, 1.f, 0.f), vec4(0.f, 0.f, 0.f, 1.f),
                           vec4(0, 0, 0, 0)};

    GPUMaterial blue_wall{vec4(blue_color, .2f), vec4(1.f, 1.f, 1.f, 0.f),
                          vec4(0.f, 0.f, 0.f, 1.f), vec4(0, 0, 0, 0)};

    GPUMaterial gray_wall{vec4(1.f, 1.f, 1.f, .3f),
                          vec4(1.f, 1.f, 1.f, 0.f), vec4(0.f, 0.f, 0.f, 1.f),
                          vec4(0, 0, 0, 0)};

    GPUMaterial light_material{vec4(1.f, 1.f, 1.f, 0.f),
                               vec4(.95f, .90f, .68f, 4.f),
                               vec4(0.f, 0.f, 0.f, 1.f), vec4(0, 0, 0, 0)};

    GPUSphere ceiling_light{vec4(0, 3.65f, -11, .8f), light_material};

    auto tri = [](vec4 a, vec4 b, vec4 c, vec4 normal, GPUMaterial material) {
        return GPUTriangle{a, b, c, normal, normal, normal, material};
    };

    std::vector<GPUTriangle> triangles;
    auto add_quad = [&](vec4 a, vec4 b, vec4 c, vec4 d, vec4 normal,
                        GPUMaterial material) {
        triangles.push_back(tri(a, b, c, normal, material));
        triangles.push_back(tri(a, c, d, normal, material));
    };

    const float left = -4.0f;
    const float right = 4.0f;
    const float bottom = -3.0f;
    const float top = 4.0f;
    const float front = -5.0f;
    const float back = -17.0f;

    // Six room walls. The front wall faces inward, so backface culling lets the
    // camera see into the room from outside.
    add_quad(vec4(left, bottom, back, 1), vec4(right, bottom, back, 1),
             vec4(right, top, back, 1), vec4(left, top, back, 1),
             vec4(0, 0, 1, 0), blue_wall);
    add_quad(vec4(left, bottom, front, 1), vec4(left, bottom, back, 1),
             vec4(left, top, back, 1), vec4(left, top, front, 1),
             vec4(1, 0, 0, 0), red_wall);
    add_quad(vec4(right, bottom, back, 1), vec4(right, bottom, front, 1),
             vec4(right, top, front, 1), vec4(right, top, back, 1),
             vec4(-1, 0, 0, 0), green_wall);
    add_quad(vec4(left, top, back, 1), vec4(right, top, back, 1),
             vec4(right, top, front, 1), vec4(left, top, front, 1),
             vec4(0, -1, 0, 0), gray_wall);
    add_quad(vec4(left, bottom, front, 1), vec4(right, bottom, front, 1),
             vec4(right, bottom, back, 1), vec4(left, bottom, back, 1),
             vec4(0, 1, 0, 0), gray_wall);
    add_quad(vec4(left, bottom, front, 1), vec4(left, top, front, 1),
             vec4(right, top, front, 1), vec4(right, bottom, front, 1),
             vec4(0, 0, -1, 0), gray_wall);

    // Adds a cube spanning [-1, 1] on each axis in object space, placed in the
    // world by `transform`.
    auto add_cube = [&](const mat4 &transform, GPUMaterial material) {
        mat3 normal_transform = transpose(inverse(mat3(transform)));
        auto corner = [&](float x, float y, float z) {
            return transform * vec4(x, y, z, 1);
        };
        auto normal = [&](float x, float y, float z) {
            return vec4(normalize(normal_transform * vec3(x, y, z)), 0);
        };

        vec4 c000 = corner(-1, -1, -1);
        vec4 c001 = corner(-1, -1, 1);
        vec4 c010 = corner(-1, 1, -1);
        vec4 c011 = corner(-1, 1, 1);
        vec4 c100 = corner(1, -1, -1);
        vec4 c101 = corner(1, -1, 1);
        vec4 c110 = corner(1, 1, -1);
        vec4 c111 = corner(1, 1, 1);

        add_quad(c001, c101, c111, c011, normal(0, 0, 1), material);
        add_quad(c000, c010, c110, c100, normal(0, 0, -1), material);
        add_quad(c000, c001, c011, c010, normal(-1, 0, 0), material);
        add_quad(c100, c110, c111, c101, normal(1, 0, 0), material);
        add_quad(c010, c011, c111, c110, normal(0, 1, 0), material);
        add_quad(c000, c100, c101, c001, normal(0, -1, 0), material);
    };

    // Glass cube in the middle of the room.
    mat4 cube_transform = translate(identity<mat4>(), vec3(0.f, -1.f, -8.5f)) *
                          rotate(identity<mat4>(), quarter_pi<float>(), vec3(0.f, 1.f, 0.f)) *
                          scale(identity<mat4>(), vec3(1.f, 1.f, 1.f));
    // add_cube(cube_transform, m1);

    // Monkey sitting on the floor behind the glass cube, facing the camera.
    GPUMaterial monkey_material{vec4(.85f, .6f, .2f, .6f),
                                vec4(1.f, 1.f, 1.f, 0.f),
                                vec4(0.f, 0.f, 0.f, 1.f), vec4(0, 0, 0, 0)};

    const float monkey_scale = 0.8f;
    mat4 monkey_transform =
        translate(identity<mat4>(),
                  vec3(0.f, 1., -10.5f)) *
        scale(identity<mat4>(), vec3(monkey_scale));
    mat3 monkey_normal_transform =
        transpose(inverse(mat3(monkey_transform)));

    for (const ObjTriangle &t : load_obj("assets/monkey.obj")) {
        GPUTriangle triangle;
        vec4 *vertices[3] = {&triangle.v1, &triangle.v2, &triangle.v3};
        vec4 *normals[3] = {&triangle.n1, &triangle.n2, &triangle.n3};
        for (int i = 0; i < 3; i++) {
            *vertices[i] = monkey_transform * vec4(t.vertices[i], 1);
            *normals[i] =
                vec4(normalize(monkey_normal_transform * t.normals[i]), 0);
        }
        triangle.material = monkey_material;
        triangles.push_back(triangle);
    }

    GPUSphere glass_sphere{vec4(.0f, 1.f, -6.5f, 1.f), m1};

    std::vector<GPUSphere> spheres = {ceiling_light, glass_sphere};

    shader.setInt("sphereCount", spheres.size());
    shader.setInt("triangleCount", triangles.size());

    camera = {{0, 1.f, 0, 0}, {0, 0, -1, 60.0f}};

    load_objects(window, camera, spheres, triangles);

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
