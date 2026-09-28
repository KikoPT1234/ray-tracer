#version 460 core

struct Camera {
    vec4 position;
    vec4 direction_fov;
};

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct Material {
    // xyz = base tint/albedo, w = smoothness. Lower smoothness adds blur.
    vec4 color_smoothness;
    // xyz = emitted light color, w = emitted light strength.
    vec4 emission_color_strength;
    // For glass: xyz = absorption per unit distance, w = refractive index.
    vec4 glass_absorption_ior;
    // x == 1 means dielectric/glass; anything else uses diffuse/specular scatter.
    vec4 type_r0;
};

struct Sphere {
    vec4 position_radius;
    int material_index;
};

struct HitInfo {
    bool did_hit;
    int index;
    int type;
    vec3 normal;
    float t;
};

uniform Camera camera;

layout (std430, binding = 0) buffer Materials {
    Material materials[];
};

layout (std430, binding = 1) buffer Spheres {
    Sphere spheres[];
};

layout (std430, binding = 2) buffer TriangleVertices {
    vec4 triangle_vertices[];
};

layout (std430, binding = 3) buffer TriangleNormals {
    vec4 triangle_normals[];
};

uniform int sphereCount;
uniform int triangleCount;

uniform vec2 resolution;
uniform float viewport_height;

uniform vec3 dx;
uniform vec3 dy;
uniform vec3 lt;

uniform sampler2D prevFrame;   // texture from last frame
uniform int frameCount;

out vec4 FragColor;

// Shared distance threshold for rejecting self-hits and nudging new rays off surfaces.
const float RAY_EPSILON = 1e-5;
const float BARY_EPSILON = 1e-7;

float random(inout uint seed) {
    seed = seed * 747796405 + 2891336453;
    uint result = ((seed >> ((seed >> 28) + 4)) ^ seed) * 277803737;
    result = (result >> 22) ^ result;
    return result * 2.3283064e-10;
}

float random_between(float start, float end, inout uint seed) {
    return (end - start) * random(seed) + start;
}

vec3 random_unit_vector(inout uint seed) {
    float z = random_between(-1, 1, seed);
    float r = sqrt(max(0.0, 1.0 - z*z));
    float phi = random_between(1e-8, 3.1415926535 * 2.0f, seed);

    return vec3(
        r * cos(phi),
        r * sin(phi),
        z
    );
}

vec3 roughen_direction(vec3 direction, float roughness, inout uint seed) {
    if (roughness == 0.0) return direction;
    // Perturb perfect reflection/refraction to make rough glass or glossy blur.
    return normalize(direction + random_unit_vector(seed) * roughness * roughness);
}

bool triangle_intersection(int index, inout float t, Ray ray, inout float u_out, inout float v_out) {
    index *= 3;
    vec4 v1 = triangle_vertices[index];
    vec4 v2 = triangle_vertices[index + 1];
    vec4 v3 = triangle_vertices[index + 2];
    
    vec3 ab = (v2 - v1).xyz;
    vec3 ac = (v3 - v1).xyz;
    vec3 ao = ray.origin - v1.xyz;

    vec3 pvec = cross(ray.direction, ac);
    float determinant = dot(ab, pvec);

    bool two_sided = v2.w == 1;

    if ((!two_sided && determinant < RAY_EPSILON) ||
        (two_sided && abs(determinant) < RAY_EPSILON)) {
        return false;
    }

    float invDet = 1.0 / determinant;

    // Calculate dst to triangle & barycentric coordinates of intersection.
    // Glass is two-sided, so negative determinants are allowed and handled by invDet.
    float u = dot(ao, pvec) * invDet;
    if (u < -BARY_EPSILON || u > 1.0 + BARY_EPSILON) {
        return false;
    }

    vec3 qvec = cross(ao, ab);
    float dst = dot(ac, qvec) * invDet;

    // Behind the ray, or farther than the closest hit found so far.
    if (dst < RAY_EPSILON || dst >= t) {
        return false;
    }


    float v = dot(ray.direction, qvec) * invDet;
    if (v < -BARY_EPSILON || v + u > 1.0 + BARY_EPSILON) {
        return false;
    }


    u_out = clamp(u, 0.0, 1.0);
    v_out = clamp(v, 0.0, 1.0);
    t = dst;

    return true;

    // info.normal = normalize(n1.xyz * w + n2.xyz * u + n3.xyz * v);
}

bool sphere_intersection(int index, inout float t, Ray ray) {
    Sphere sphere = spheres[index];
    vec3 op = sphere.position_radius.xyz - ray.origin.xyz;
    vec3 rd = ray.direction.xyz;
    float radius = sphere.position_radius.w;
    float h = dot(rd, op);
    float c = dot(op, op) - radius * radius;

    float discriminant = h * h - c;
    if (discriminant < 0) {
        return false;
    }

    float sqrt_discriminant = sqrt(discriminant);
    float x = (h - sqrt_discriminant);

    // Something else that's closer has already been found
    if (x > t) return false;

    // If the near root is behind/too close, try the far root so rays inside spheres can exit.
    if (x <= RAY_EPSILON)
        x = (h + sqrt_discriminant);

    if (x > t) return false;

    if (x <= RAY_EPSILON)
        return false;
    else {
        t = x;
        return true;
    }
}

HitInfo intersect_ray(Ray ray, inout uint seed) {
    HitInfo info;
    info.did_hit = false;
    info.t = 1.0 / 0.0;

    bool sphere_hit = false;

    for (int i = 0; i < sphereCount; i++) {
        if (sphere_intersection(i, info.t, ray)) {
            sphere_hit = true;
            info.index = i;
        }
    }

    bool triangle_hit = false;
    float u = 0.0, v = 0.0;    

    for (int i = 0; i < triangleCount; i++) {
        if (triangle_intersection(i, info.t, ray, u, v)) {
            triangle_hit = true;
            info.index = i;
        }
    }

    // float scatter_probability = info.t * 0.001;

    // float scatter_t = random(seed);

    // if (scatter_t <= scatter_probability) {
    //     info.did_hit = true;
    //     info.t = scatter_t;
    //     info.normal = random_unit_vector(seed);
    //     info.type = 3;
    //     return info;
    // }

    if (sphere_hit && !triangle_hit) {
        Sphere sphere = spheres[info.index];
        vec3 hit_point = ray.origin + info.t * ray.direction;
        info.normal = (hit_point - sphere.position_radius.xyz) / sphere.position_radius.w;
        info.type = 0;
        info.did_hit = true;
    } else if (triangle_hit) {
        int index = info.index * 3;
        vec4 n1 = triangle_normals[index];
        vec4 n2 = triangle_normals[index + 1];
        vec4 n3 = triangle_normals[index + 2];
        float w = clamp(1.0 - v - u, 0.0, 1.0);

        info.normal = normalize(n1.xyz * w + n2.xyz * u + n3.xyz * v);
        info.type = 1;
        info.did_hit = true;
    }

    return info;
}

vec3 lerp(vec3 a, vec3 b, float h) {
    return a * (1.0f-h) + b * h;
}

Material get_material(HitInfo hit) {
    if (hit.type == 0) return materials[spheres[hit.index].material_index];
    else if (hit.type == 1) return materials[int(triangle_vertices[hit.index * 3].w)];
}

// Diffuse/specular bounce off a surface, blended by the material's smoothness.
vec3 calculate_reflection(Ray ray, Material material, vec3 normal, inout uint seed) {
    float smoothness = material.color_smoothness.w;
    vec3 diffuse_direction = normalize(normal + random_unit_vector(seed));
    vec3 specular_direction = reflect(ray.direction, normal);
    vec3 reflect_direction =
        normalize(lerp(diffuse_direction, specular_direction, smoothness));

    return reflect_direction;
}

// Glass bounce: randomly reflects or refracts according to Fresnel (Schlick),
// and applies Beer-Lambert absorption to `color` when the ray exits the glass.
vec3 calculate_refraction(Ray ray, HitInfo hit, Material material, inout vec3 color,
                          inout uint seed) {
    vec3 glass_absorption = material.glass_absorption_ior.xyz;
    float refractive_index = material.glass_absorption_ior.w;

    vec3 outward_normal = hit.normal;
    // Front-face tells us whether the ray is entering or leaving the glass.
    bool front_face = dot(ray.direction, outward_normal) < 0.0;
    vec3 normal = front_face ? outward_normal : -outward_normal;
    float eta = front_face ? 1.0 / refractive_index : refractive_index;
    float hit_cos = min(-dot(ray.direction, normal), 1.0);
    float hit_sine2 = max(0.0, 1.0 - hit_cos * hit_cos);
    float sine2_outgoing = hit_sine2 * eta * eta;

    // If Snell's law would require sin(theta) > 1, this is total internal reflection.
    bool cannot_refract = sine2_outgoing > 1.0;

    vec3 direction;

    if (!cannot_refract) {
        float cos_outgoing = sqrt(max(0.0, 1.0 - sine2_outgoing));
        // Schlick approximation: probability that this bounce reflects instead of refracts.
        // float r0 = material.type_r0.y;
        // r0 *= r0;
        // float x = 1.0 - (eta <= 1 ? hit_cos : cos_outgoing);
        // float x2 = x * x;
        // float reflectance = r0 + (1.0 - r0) * x2 * x2 * x;

        // Fresnel equations
        float rs = (eta * hit_cos - cos_outgoing) / (eta * hit_cos + cos_outgoing);
        float rp = (hit_cos - eta * cos_outgoing) / (hit_cos + eta * cos_outgoing);
        float reflectance = 0.5 * (rs * rs + rp * rp);

        vec3 refract_direction =
            eta * ray.direction + (eta * hit_cos - cos_outgoing) * normal;

        bool refracted = random(seed) >= reflectance;
        if (refracted)
            direction = refract_direction;
        // reflection direction instead
        else direction = calculate_reflection(ray, material, normal, seed);
    } else {
        direction = calculate_reflection(ray, material, normal, seed);
    }

    if (!front_face) {
        // Beer-Lambert absorption: longer paths through glass tint/darken more.
        color *= exp(-max(glass_absorption, vec3(0.0)) * hit.t);
    }

    return direction;
}

// Picks the next ray direction for a hit, updating `color` for any absorption.
vec3 scatter(Ray ray, HitInfo hit, Material material, inout vec3 color, inout uint seed) {
    float smoothness = material.color_smoothness.w;
    float roughness = clamp(1.0 - smoothness, 0.0, 1.0);

    vec3 direction;

    if (material.type_r0.x == 1) {
        direction = roughen_direction(calculate_refraction(ray, hit, material, color, seed), roughness, seed);
    } else {
        direction = calculate_reflection(ray, material, hit.normal, seed);
    }

    return direction;
}

// `direction` must be unit length (every ray direction in trace_ray already is).
vec3 environment_light(vec3 direction) {
    float a = 0.5 * (direction.y + 1.0);
    vec3 environment_light = ((1.0 - a) * vec3(1.0, 1.0, 1.0) +
                                a * vec3(0.1, 0.4, 1.0));
    float sky_intensity = 1;
    // vec3 environment_light{0, 0, 0};
    return environment_light * sky_intensity;
}

vec3 trace_ray(Ray ray, int bounces, inout uint seed) {
    vec3 color = vec3(1, 1, 1);
    vec3 light = vec3(0, 0, 0);

    for (int i = 0; i <= bounces; i++) {
        HitInfo hit = intersect_ray(ray, seed);
        if (!hit.did_hit) {
            light += color * environment_light(ray.direction);
            break;
        }

        bool front_face = dot(hit.normal, ray.direction) < 0.0;

        if (front_face) {
            float scatter_dist = -log(random(seed)) / 0.1;

            if (scatter_dist <= hit.t) {
                vec3 hit_point = ray.origin + scatter_dist * ray.direction;
                ray = Ray(hit_point, random_unit_vector(seed));
                continue;
            }
        }

        Material material = get_material(hit);

        vec3 emission_color = material.emission_color_strength.xyz;
        float emission_strength = material.emission_color_strength.w;
        vec3 emitted_light = emission_color * emission_strength;
        light += emitted_light * color;
        color *= material.color_smoothness.xyz;

        if (i >= 2) {
            // Random early exit if ray colour is nearly 0 (can't contribute much to final result)
            float p = max(color.r, max(color.g, color.b));
            if (random(seed) >= p) {
                break;
            }

            color *= 1.0 / p;

            if (i == bounces) break;
        }

        vec3 direction = scatter(ray, hit, material, color, seed);
        vec3 hit_point = ray.origin + hit.t * ray.direction;

        // Offset toward the side the next ray is actually travelling into.
        // This avoids self-intersection rings, especially for internal glass reflection.
        float offset_side = dot(direction, hit.normal) < 0.0 ? -1.0 : 1.0;
        ray = Ray(hit_point + hit.normal * offset_side * RAY_EPSILON, direction);
    }

    return light;
}

uint pcg_hash(uint v) {
    v = v * 747796405u + 2891336453u;
    v = ((v >> ((v >> 28u) + 4u)) ^ v) * 277803737u;
    return (v >> 22u) ^ v;
}

uint hash2(uvec2 v) {
    return pcg_hash(v.x + pcg_hash(v.y));
}

void main()
{
    // vec2 viewport = vec2( (resolution.x / resolution.y) * viewport_height, viewport_height );

    // mat4 cam_matrix = get_cam_matrix();

    // vec3 viewport_u =
    //         (cam_matrix * vec4(viewport.x, 0, 0, 1.0) - vec4(camera.position.xyz, 0)).xyz;

    // vec3 viewport_v = (cam_matrix * vec4(0, viewport.y, 0, 1.0) -
    //                     vec4(camera.position.xyz, 0)).xyz;

    // vec3 dx = viewport_u * (1.0 / resolution.x);
    // vec3 dy = viewport_v * (1.0 / resolution.y);

    // vec3 lt = get_left_top(viewport);

    uint seed = hash2(uvec2(gl_FragCoord.xy));
    seed = pcg_hash(seed ^ pcg_hash(frameCount));

    float offset_x = random_between(-.8f, .8f, seed);
    float offset_y = random_between(-.8f, .8f, seed);

    vec3 pos = lt + (gl_FragCoord.x + offset_x) * (dx) + (gl_FragCoord.y + offset_y) * dy;
    
    Ray ray = Ray(
        camera.position.xyz,
        normalize(pos - camera.position.xyz)
    );

    vec4 prev = texelFetch(prevFrame, ivec2(gl_FragCoord.xy), 0);

    vec4 current = vec4(0.0, 0.0, 0.0, 1.0);

    int num_samples = 2;

    for (int i = 0; i < num_samples; i++) {
        current += vec4(trace_ray(ray, 14, seed), 0);
    }

    current /= num_samples;

    float w = 1.0 / float(frameCount + 1);

    FragColor = prev * (1.0 - w) + current * w;
}
