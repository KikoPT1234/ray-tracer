#ifndef SCENE_H
#define SCENE_H

#include "common.hpp"

#include <vector>

// Structs mirrored by the std430 buffers in shaders/raytrace.frag. Fields are
// packed into vec4s to match std430 alignment; use the constructors (or the
// diffuse/emissive/glass helpers) instead of filling the vec4s by hand.

enum class MaterialType { Diffuse = 0, Glass = 1 };

struct GPUMaterial {
    vec4 color_smoothness = vec4(1.f, 1.f, 1.f, 0.f);
    vec4 emission_color_strength = vec4(1.f, 1.f, 1.f, 0.f);
    vec4 glass_absorption_ior = vec4(0.f, 0.f, 0.f, 1.f);
    vec4 type = vec4(0.f);

    // White, fully diffuse.
    GPUMaterial() = default;

    // Smoothness 0 is fully diffuse, 1 is a perfect mirror (or clear glass).
    // `absorption` is Beer-Lambert absorption per unit distance, glass only.
    GPUMaterial(vec3 color, float smoothness = 0.f,
                vec3 emission_color = vec3(1.f), float emission_strength = 0.f,
                MaterialType material_type = MaterialType::Diffuse,
                float ior = 1.f, vec3 absorption = vec3(0.f))
        : color_smoothness(color, smoothness),
          emission_color_strength(emission_color, emission_strength),
          glass_absorption_ior(absorption, ior),
          type(float(material_type), 0.f, 0.f, 0.f) {}
};

struct GPUSphere {
    vec4 position_radius;
    GPUMaterial material;

    GPUSphere(vec3 center, float radius,
              const GPUMaterial &material = GPUMaterial())
        : position_radius(center, radius), material(material) {}
};

struct GPUTriangle {
    vec4 v1;
    vec4 v2;
    vec4 v3;

    vec4 n1;
    vec4 n2;
    vec4 n3;
    GPUMaterial material;

    // Flat triangle. Its normal comes from the winding (counter-clockwise when
    // seen from the front), which is also the side the shader doesn't cull.
    GPUTriangle(vec3 a, vec3 b, vec3 c,
                const GPUMaterial &material = GPUMaterial())
        : GPUTriangle(a, b, c, vec3(0.f), vec3(0.f), vec3(0.f), material) {
        n1 = n2 = n3 = vec4(normalize(cross(b - a, c - a)), 0);
    }

    // Smooth-shaded triangle with per-vertex normals.
    GPUTriangle(vec3 a, vec3 b, vec3 c, vec3 na, vec3 nb, vec3 nc,
                const GPUMaterial &material = GPUMaterial())
        : v1(a, 1), v2(b, 1), v3(c, 1), n1(na, 0), n2(nb, 0), n3(nc, 0),
          material(material) {}
};

// These get memcpy'd straight into the shader's buffers.
static_assert(sizeof(GPUMaterial) == 4 * sizeof(vec4));
static_assert(sizeof(GPUSphere) == 5 * sizeof(vec4));
static_assert(sizeof(GPUTriangle) == 10 * sizeof(vec4));

inline GPUMaterial diffuse(vec3 color, float smoothness = 0.f) {
    return GPUMaterial(color, smoothness);
}

// Light source. The surface itself doesn't reflect anything useful.
inline GPUMaterial emissive(vec3 color, float strength) {
    return GPUMaterial(vec3(1.f), 0.f, color, strength);
}

// Lower smoothness gives frosted glass.
inline GPUMaterial glass(float ior, vec3 absorption = vec3(0.f),
                         float smoothness = 1.f) {
    return GPUMaterial(vec3(1.f), smoothness, vec3(1.f), 0.f,
                       MaterialType::Glass, ior, absorption);
}

struct Scene {
    std::vector<GPUSphere> spheres;
    std::vector<GPUTriangle> triangles;

    void add_sphere(vec3 center, float radius, const GPUMaterial &material) {
        spheres.emplace_back(center, radius, material);
    }

    void add_triangle(vec3 a, vec3 b, vec3 c, const GPUMaterial &material) {
        triangles.emplace_back(a, b, c, material);
    }

    void add_triangles(const std::vector<GPUTriangle> &new_triangles) {
        triangles.insert(triangles.end(), new_triangles.begin(),
                         new_triangles.end());
    }

    // Quad a-b-c-d, wound counter-clockwise when seen from the front.
    void add_quad(vec3 a, vec3 b, vec3 c, vec3 d, const GPUMaterial &material) {
        add_triangle(a, b, c, material);
        add_triangle(a, c, d, material);
    }

    // Cube spanning [-1, 1] on each axis in object space, placed in the world
    // by `transform`.
    void add_cube(const mat4 &transform, const GPUMaterial &material) {
        auto corner = [&](float x, float y, float z) {
            return vec3(transform * vec4(x, y, z, 1));
        };

        vec3 c000 = corner(-1, -1, -1);
        vec3 c001 = corner(-1, -1, 1);
        vec3 c010 = corner(-1, 1, -1);
        vec3 c011 = corner(-1, 1, 1);
        vec3 c100 = corner(1, -1, -1);
        vec3 c101 = corner(1, -1, 1);
        vec3 c110 = corner(1, 1, -1);
        vec3 c111 = corner(1, 1, 1);

        add_quad(c001, c101, c111, c011, material); // +z
        add_quad(c000, c010, c110, c100, material); // -z
        add_quad(c000, c001, c011, c010, material); // -x
        add_quad(c100, c110, c111, c101, material); // +x
        add_quad(c010, c011, c111, c110, material); // +y
        add_quad(c000, c100, c101, c001, material); // -y
    }
};

#endif
