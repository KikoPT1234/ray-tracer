#include "common.hpp"

#include "gl_init.cpp"

#include "camera.cpp"
#include "obj.cpp"
#include "ppm.hpp"
#include "ray.cpp"
#include "scene.cpp"
#include "shader.cpp"
#include "shape.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace glm;

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void load_camera() {
    glUseProgram(raytrace_shader_id);
    glUniform4fv(camera_position_loc, 1, glm::value_ptr(camera.position));
    glUniform4fv(camera_direction_loc, 1,
                 glm::value_ptr(camera.direction_fov));

    load_viewport(current_width, current_height);
}

void load_objects(GLFWwindow *window, const Scene &scene) {
    const std::vector<GPUMaterial> &materials = scene.materials;
    const std::vector<GPUSphere> &spheres = scene.spheres;
    const std::vector<vec4> &triangleVer = scene.triangleVertices;
    const std::vector<vec4> &triangleNor = scene.triangleNormals;

    glGenBuffers(1, &materialsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 materials.size() * sizeof(GPUMaterial), materials.data(),
                 GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, materialsSSBO);

    glGenBuffers(1, &spheresSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, spheresSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, spheres.size() * sizeof(GPUSphere),
                 spheres.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, spheresSSBO);

    glGenBuffers(1, &triangleVerticesSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangleVerticesSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, triangleVer.size() * sizeof(vec4),
                 triangleVer.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, triangleVerticesSSBO);

    glGenBuffers(1, &triangleNormalsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangleNormalsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, triangleNor.size() * sizeof(vec4),
                 triangleNor.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, triangleNormalsSSBO);

    GLuint viewport_height =
        glGetUniformLocation(raytrace_shader_id, "viewport_height");
    glUniform1f(viewport_height, VIEWPORT_HEIGHT);

    set_resolution(window, WIDTH, HEIGHT);
    load_camera();
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

    // Display-only, so changing it doesn't restart accumulation.
    float exposure = 0.6f;
    GLint exposure_loc = glGetUniformLocation(display_shader_id, "exposure");

    set_resolution(window, WIDTH, HEIGHT);

    // Ping-pong: read last frame's texture, render into the other one's FBO.
    GLuint *texRead = &texA;
    GLuint *texWrite = &texB;
    GLuint *fboWrite = &fboB;

    glfwSwapInterval(0);

    float delta_time;
    float last_frame = glfwGetTime();
    float count_since_fps = 0;
    float delta_since_fps = 0;

    while (!glfwWindowShouldClose(window)) {

        float current_frame = glfwGetTime();
        delta_time = current_frame - last_frame;
        last_frame = current_frame;
        count_since_fps++;
        delta_since_fps += delta_time;

        if (delta_since_fps > 0.5f) {
            std::printf("FPS: %f\n", count_since_fps / delta_since_fps);
            count_since_fps = 0;
            delta_since_fps = 0;
        }

        if (update_pos(window, delta_time)) {
            count = 0;
            load_camera();
        }

        // E / Q: brighten / darken by one stop per second.
        float old_exposure = exposure;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            exposure *= std::exp2(delta_time);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            exposure /= std::exp2(delta_time);
        if (exposure != old_exposure)
            std::printf("Exposure: %f\n", exposure);

        rayShader.use();
        glBindFramebuffer(GL_FRAMEBUFFER, *fboWrite);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, *texRead);
        glUniform1i(frame_count_loc, count);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        std::swap(texRead, texWrite);
        fboWrite = (fboWrite == &fboA) ? &fboB : &fboA;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        displayShader.use();
        glUniform1f(exposure_loc, exposure);
        glBindTexture(GL_TEXTURE_2D, *texRead);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();

        count++;

        if (new_res) {
            new_res = false;
            set_resolution(window, current_width, current_height);
        }
    }
}

Scene build_scene() {
    Scene scene;

    GPUMaterial red_wall = diffuse(rgb_to_vec3(240, 101, 101), .2f);
    GPUMaterial green_wall = diffuse(rgb_to_vec3(88, 224, 108), .2f);
    GPUMaterial blue_wall = diffuse(rgb_to_vec3(29, 102, 219), .2f);
    GPUMaterial gray_wall = diffuse(vec3(.7f), .3f);
    GPUMaterial light_material = emissive(vec3(.95f, .90f, .88f), 30.f);
    GPUMaterial glass_material = glass(1.5f, vec3(0.f));
    GPUMaterial monkey_material = diffuse(vec3(.85f, .6f, .2f), .6f);

    scene.add_materials({red_wall, green_wall, blue_wall, gray_wall,
                         light_material, glass_material, monkey_material});

    const float left = -4.0f;
    const float right = 4.0f;
    const float bottom = -6.0f;
    const float top = 4.0f;
    const float front = 6.0f;
    const float back = -17.0f;

    // Six room walls. The front wall faces inward, so backface culling lets the
    // camera see into the room from outside.
    scene.add_quad(vec3(left, bottom, back), vec3(right, bottom, back),
                   vec3(right, top, back), vec3(left, top, back), 2, 0);
    scene.add_quad(vec3(left, bottom, front), vec3(left, bottom, back),
                   vec3(left, top, back), vec3(left, top, front), 0, 0);
    scene.add_quad(vec3(right, bottom, back), vec3(right, bottom, front),
                   vec3(right, top, front), vec3(right, top, back), 1, 0);
    scene.add_quad(vec3(left, top, back), vec3(right, top, back),
                   vec3(right, top, front), vec3(left, top, front), 3, 0);
    scene.add_quad(vec3(left, bottom, front), vec3(right, bottom, front),
                   vec3(right, bottom, back), vec3(left, bottom, back), 3, 0);
    scene.add_quad(vec3(left, bottom, front), vec3(left, top, front),
                   vec3(right, top, front), vec3(right, bottom, front), 3, 0);

    scene.add_sphere(vec3(0, 4.f, -11), 1.2f, 4);

    // Glass cube in the middle of the room.
    mat4 cube_transform =
        translate(identity<mat4>(), vec3(0.f, -1.f, -8.5f)) *
        rotate(identity<mat4>(), quarter_pi<float>(), vec3(0.f, 1.f, 0.f)) *
        scale(identity<mat4>(), vec3(1.f, 1.f, 1.f));
    // scene.add_cube(cube_transform, glass_material);

    // Monkey behind the glass sphere, facing the camera.
    mat4 monkey_transform = translate(identity<mat4>(), vec3(0.f, 1., -10.5f)) *
                            scale(identity<mat4>(), vec3(0.8f));
    // load_obj(scene, "assets/monkey.obj", 6, 0, monkey_transform);

    // scene.add_sphere(vec3(.0f, 1.f, -6.5f), 1.f, 5);
    scene.add_sphere(vec3(.0f, -1.8f, -11), 1.6f, 5);
    // scene.add_sphere(vec3(.0f, 1.f, -12.5f), 1.f, 5);

    return scene;
}

int main() {
    GLFWwindow *window = init();
    if (window == nullptr)
        return -1;

    load_buffers();

    Shader shader("shaders/shader.vert", "shaders/raytrace.frag");
    shader.use();
    raytrace_shader_id = shader.ID;
    cache_uniform_locations();

    Scene scene = build_scene();

    shader.setInt("sphereCount", scene.spheres.size());
    shader.setInt("triangleCount", scene.triangleVertices.size() / 3);

    camera = {{0, 1.f, 0, 0}, {0, 0, -1, 60.0f}};

    load_objects(window, scene);

    loop(window, shader);

    terminate();
}