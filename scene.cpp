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
    vec4 type_r0 = vec4(0.f, 1.f, 0.f, 0.f);

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
          type_r0(float(material_type), (1.0 - ior) / (1.0 + ior), 0.f, 0.f) {}
};

struct GPUSphere {
    vec4 position_radius;
    ivec4 material_index;

    GPUSphere(vec3 center, float radius, int material_index)
        : position_radius(center, radius),
          material_index(material_index, .0f, .0f, .0f) {}
};

// These get memcpy'd straight into the shader's buffers.
static_assert(sizeof(GPUMaterial) == 4 * sizeof(vec4));
static_assert(sizeof(GPUSphere) == 2 * sizeof(vec4));

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
    std::vector<GPUMaterial> materials;
    std::vector<GPUSphere> spheres;
    std::vector<vec4> triangleVertices;
    std::vector<vec4> triangleNormals;

    void add_material(const GPUMaterial &material) {
        materials.push_back(material);
    }

    void add_materials(const std::vector<GPUMaterial> &incMaterials) {
        materials.insert(materials.end(), incMaterials.begin(),
                         incMaterials.end());
    }

    void add_sphere(vec3 center, float radius, int materialIndex) {
        spheres.emplace_back(center, radius, materialIndex);
    }

    // Flat triangle. Its normal comes from the winding (counter-clockwise when
    // seen from the front), which is also the side the shader doesn't cull.
    void add_triangle(vec3 a, vec3 b, vec3 c, int materialIndex,
                      int materialType) {
        vec3 normal = normalize(cross(b - a, c - a));
        add_triangle(a, b, c, normal, normal, normal, materialIndex,
                     materialType);
    }

    // Smooth-shaded triangle with per-vertex normals.
    void add_triangle(vec3 a, vec3 b, vec3 c, vec3 na, vec3 nb, vec3 nc,
                      int materialIndex, int materialType) {
        triangleVertices.push_back(vec4(a, materialIndex));
        triangleVertices.push_back(vec4(b, materialType));
        triangleVertices.push_back(vec4(c, 0));
        triangleNormals.push_back(vec4(na, 0));
        triangleNormals.push_back(vec4(nb, 0));
        triangleNormals.push_back(vec4(nc, 0));
    }

    // Quad a-b-c-d, wound counter-clockwise when seen from the front.
    void add_quad(vec3 a, vec3 b, vec3 c, vec3 d, int materialIndex,
                  int materialType) {
        add_triangle(a, b, c, materialIndex, materialType);
        add_triangle(a, c, d, materialIndex, materialType);
    }

    // Cube spanning [-1, 1] on each axis in object space, placed in the world
    // by `transform`.
    void add_cube(const mat4 &transform, int materialIndex, int materialType) {
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

        add_quad(c001, c101, c111, c011, materialIndex, materialType); // +z
        add_quad(c000, c010, c110, c100, materialIndex, materialType); // -z
        add_quad(c000, c001, c011, c010, materialIndex, materialType); // -x
        add_quad(c100, c110, c111, c101, materialIndex, materialType); // +x
        add_quad(c010, c011, c111, c110, materialIndex, materialType); // +y
        add_quad(c000, c100, c101, c001, materialIndex, materialType); // -y
    }
};

#endif
