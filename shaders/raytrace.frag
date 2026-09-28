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
    vec4 type;
};

struct Sphere {
    vec4 position_radius;
    Material material;
};

struct Triangle {
    vec4 v1;
    vec4 v2;
    vec4 v3;

    vec4 n1;
    vec4 n2;
    vec4 n3;
    Material material;
};

struct HitInfo {
    bool did_hit;
    Sphere sphere;
    Triangle triangle;
    vec3 point;
    vec3 normal;
    float t;
    bool isTriangle;
};

uniform Camera camera;

layout (std430, binding = 0) buffer Spheres {
    Sphere spheres[];
};

layout (std430, binding = 1) buffer Triangles {
    Triangle triangles[];
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
    return result / 4294967295.0;
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
    // Perturb perfect reflection/refraction to make rough glass or glossy blur.
    return normalize(direction + random_unit_vector(seed) * roughness * roughness);
}

void triangle_intersection(Triangle triangle, Ray ray, inout HitInfo info) {
    vec3 ab = (triangle.v2 - triangle.v1).xyz;
    vec3 ac = (triangle.v3 - triangle.v1).xyz;
    vec3 ao = ray.origin - triangle.v1.xyz;

    vec3 pvec = cross(ray.direction, ac);
    float determinant = dot(ab, pvec);
    bool two_sided = triangle.material.type.x == 1;

    if ((!two_sided && determinant < RAY_EPSILON) ||
        (two_sided && abs(determinant) < RAY_EPSILON)) {
        info.did_hit = false;
        return;
    }

    float invDet = 1.0 / determinant;

    // Calculate dst to triangle & barycentric coordinates of intersection.
    // Glass is two-sided, so negative determinants are allowed and handled by invDet.
    float u = dot(ao, pvec) * invDet;
    if (u < -BARY_EPSILON || u > 1.0 + BARY_EPSILON) {
        info.did_hit = false;
        return;
    }

    vec3 qvec = cross(ao, ab);
    float v = dot(ray.direction, qvec) * invDet;
    if (v < -BARY_EPSILON || v + u > 1.0 + BARY_EPSILON) {
        info.did_hit = false;
        return;
    }

    float dst = dot(ac, qvec) * invDet;
    if (dst < RAY_EPSILON) {
        info.did_hit = false;
        return;
    }

    u = clamp(u, 0.0, 1.0);
    v = clamp(v, 0.0, 1.0);
    float w = clamp(1.0 - u - v, 0.0, 1.0);

    // Initialize hit info
    info.did_hit = true;
    info.isTriangle = true;
    info.triangle = triangle;
    info.point = ray.origin + ray.direction * dst;
    info.normal = normalize(triangle.n1.xyz * w + triangle.n2.xyz * u + triangle.n3.xyz * v);
    
    info.t = dst;
}

void sphere_intersection(Sphere sphere, Ray ray, inout HitInfo info) {
    vec3 op = sphere.position_radius.xyz - ray.origin.xyz;
    vec3 rd = ray.direction.xyz;
    float radius = sphere.position_radius.w;
    float h = dot(rd, op);
    float c = dot(op, op) - radius * radius;

    float discriminant = h * h - c;
    if (discriminant < 0) {
        info.did_hit = false;
        return;
    }

    float sqrt_discriminant = sqrt(discriminant);
    float x = (h - sqrt_discriminant);

    // If the near root is behind/too close, try the far root so rays inside spheres can exit.
    if (x <= RAY_EPSILON)
        x = (h + sqrt_discriminant);

    if (x <= RAY_EPSILON)
        info.did_hit = false;
    else {
        info.did_hit = true;
        info.sphere = sphere;
        info.t = x;
        info.point = ray.origin + ray.direction * x;
        info.normal = (info.point - sphere.position_radius.xyz) / radius;
        info.isTriangle = false;
    }
}

HitInfo intersect_ray(Ray ray) {
    HitInfo info;
    info.did_hit = false;
    info.t = 1.0 / 0.0;

    for (int i = 0; i < sphereCount; i++) {
        Sphere sphere = spheres[i];

        HitInfo temp;
        temp.did_hit = false;
        sphere_intersection(sphere, ray, temp);

        if (temp.did_hit) {
            if (temp.t < info.t && temp.t > RAY_EPSILON) {
                info = temp;
            }
        }        
    }

    for (int i = 0; i < triangleCount; i++) {
        Triangle triangle = triangles[i];

        HitInfo temp;
        temp.did_hit = false;
        triangle_intersection(triangle, ray, temp);

        if (temp.did_hit) {
            if (temp.t < info.t && temp.t > RAY_EPSILON) {
                info = temp;
            }
        }  
    }

    return info;
}

vec3 lerp(vec3 a, vec3 b, float h) {
    return a * (1.0f-h) + b * h;
}

Material get_material(HitInfo hit) {
    if (hit.isTriangle) return hit.triangle.material;
    return hit.sphere.material;
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

    vec3 reflect_direction = calculate_reflection(ray, material, normal, seed);
    // If Snell's law would require sin(theta) > 1, this is total internal reflection.
    bool cannot_refract = sine2_outgoing > 1.0;
    vec3 direction = reflect_direction;

    if (!cannot_refract) {
        float cos_outgoing = sqrt(max(0.0, 1.0 - sine2_outgoing));
        // Schlick approximation: probability that this bounce reflects instead of refracts.
        float r0 = (1.0 - refractive_index) / (1.0 + refractive_index);
        r0 *= r0;
        float x = 1.0 - (eta <= 1 ? hit_cos : cos_outgoing);
        float x2 = x * x;
        float reflectance = r0 + (1.0 - r0) * x2 * x2 * x;

        vec3 refract_direction =
            eta * ray.direction + (eta * hit_cos - cos_outgoing) * normal;

        bool refracted = random(seed) >= reflectance;
        direction = refracted ? refract_direction : reflect_direction;
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
    if (material.type.x == 1) {
        direction = calculate_refraction(ray, hit, material, color, seed);
    } else {
        direction = calculate_reflection(ray, material, hit.normal, seed);
    }

    return roughen_direction(direction, roughness, seed);
}

vec3 environment_light(vec3 direction) {
    vec3 unit_direction = normalize(direction);
    float a = 0.5 * (unit_direction.y + 1.0);
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
        HitInfo hit = intersect_ray(ray);
        if (!hit.did_hit) {
            light += color * environment_light(ray.direction);
            break;
        }

        Material material = get_material(hit);

        vec3 emission_color = material.emission_color_strength.xyz;
        float emission_strength = material.emission_color_strength.w;
        vec3 emitted_light = emission_color * emission_strength;
        light += emitted_light * color;
        color *= material.color_smoothness.xyz;

        vec3 direction = scatter(ray, hit, material, color, seed);

        // Random early exit if ray colour is nearly 0 (can't contribute much to final result)
        // float p = max(color.r, max(color.g, color.b));
        // if (random(seed) >= p) {
        //     break;
        // }

        // color *= 1.0 / p;

        // Offset toward the side the next ray is actually travelling into.
        // This avoids self-intersection rings, especially for internal glass reflection.
        float offset_side = dot(direction, hit.normal) < 0.0 ? -1.0 : 1.0;
        ray = Ray(hit.point + hit.normal * offset_side * RAY_EPSILON, direction);
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

    float offset_x = random_between(-.5f, .5f, seed);
    float offset_y = random_between(-.5f, .5f, seed);

    vec3 pos = lt + (gl_FragCoord.x + offset_x) * (dx) + (gl_FragCoord.y + offset_y) * dy;
    
    Ray ray = Ray(
        camera.position.xyz,
        normalize(pos - camera.position.xyz)
    );

    vec2 uv = gl_FragCoord.xy / resolution;

    vec4 prev = texture(prevFrame, uv);
    vec4 current = vec4(trace_ray(ray, 7, seed), 1);

    float w = 1.0 / float(frameCount + 1);

    FragColor = prev * (1.0 - w) + current * w;
}
