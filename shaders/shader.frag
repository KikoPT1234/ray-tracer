#version 460 core

struct Camera {
    vec4 position;
    vec4 direction;
};

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct Material {
    vec4 color_smoothness;
    vec4 emission_color_strength;
};

struct Sphere {
    vec4 position_radius;
    Material material;
};

struct HitInfo {
    bool did_hit;
    Sphere sphere;
    vec3 point;
    vec3 normal;
    double t;
};

uniform Camera camera;

layout (std430, binding = 0) buffer Spheres {
    Sphere spheres[];
};

uniform int sphereCount;
uniform vec2 resolution;
uniform float viewport_height;

out vec4 FragColor;

float random(inout uint seed) {
    seed = seed * 747796405 + 2891336453;
    uint result = ((seed >> ((seed >> 28) + 4)) ^ seed) + 277803737;
    result = (result >> 22) ^ result;
    return result / 4294967295.0;
}

float random_between(float start, float end, inout uint seed) {
    return (end - start) * random(seed) + start;
}

vec3 random_unit_vector(inout uint seed) {
    vec3 candidate;
    while (true) {
        candidate = vec3(random_between(-1, 1, seed), random_between(-1, 1, seed),
                     random_between(-1, 1, seed));

        // std::printf("(%f %f %f)\n", candidate.x, candidate.y, candidate.z);
        double len_sq = dot(candidate, candidate);

        if (len_sq < 1e-160 || len_sq > 1.0)
            continue;

        candidate = normalize(candidate);
        break;
    }
    return candidate;
}

mat4 get_cam_matrix() {
    vec3 world_up = vec3(0.0, 1.0, 0.0);
    vec3 forward = -camera.direction.xyz;
    vec3 right = normalize(cross(world_up, forward));
    vec3 up = cross(forward, right);

    mat4 mat;

    mat[0] = vec4(right, 0.0);
    mat[1] = vec4(up, 0.0);
    mat[2] = vec4(forward, 0.0);
    mat[3] = vec4(camera.position.xyz, 1.0f);

    return mat;
}

float get_focal_length() {
    float FOV = 90.0;
    return (viewport_height / 2.0) / tan(radians(FOV / 2.0));
}

vec3 get_left_top(vec2 viewport) {
    float fz = get_focal_length();
    mat4 cam_matrix = get_cam_matrix();
    vec3 outv = (cam_matrix * vec4(-viewport.x / 2.0,
                                    -viewport.y / 2.0, -fz, 1.0)).xyz;
    return outv;
}


void sphere_intersection(Sphere sphere, Ray ray, inout HitInfo info) {
    vec3 op = sphere.position_radius.xyz - ray.origin.xyz;
    vec3 rd = ray.direction.xyz;
    float radius = sphere.position_radius.w;
    float a = dot(rd, rd);
    float h = dot(rd, op);
    float c = dot(op, op) - radius * radius;

    float discriminant = h * h - a * c;
    if (discriminant < 0) {
        info.did_hit = false;
        return;
    }

    float x;
    if (discriminant == 0)
        x = h / a;
    else
        x = (h - sqrt(discriminant)) / a;

    if (x <= 1e-10)
        info.did_hit = false;
    else {
        info.did_hit = true;
        info.sphere = sphere;
        info.t = x;
        info.point = ray.origin + ray.direction * x;
        info.normal = (info.point - sphere.position_radius.xyz) / radius;
    }
}

HitInfo intersect_ray(Ray ray) {
    HitInfo info;
    info.did_hit = false;
    info.t = 1.0 / 0.0;

    float epsilon = 1e-8;

    for (int i = 0; i < sphereCount; i++) {
        Sphere sphere = spheres[i];

        HitInfo temp;
        temp.did_hit = false;
        sphere_intersection(sphere, ray, temp);

        if (temp.did_hit) {
            if (temp.t < info.t && temp.t > epsilon) {
                info = temp;
            }
        }        
    }

    return info;
}

vec3 lerp(vec3 a, vec3 b, float h) {
    return a * (1-h) + b * h;
}

vec3 trace_ray(Ray ray, int bounces, inout uint seed) {
    vec3 color = vec3(1, 1, 1);
    vec3 light = vec3(0, 0, 0);

    for (int i = 0; i <= bounces; i++) {
        HitInfo hit = intersect_ray(ray);
        if (hit.did_hit) {
            Material material = hit.sphere.material;
            vec3 emission_color = material.emission_color_strength.xyz;
            float emission_strength = material.emission_color_strength.w;
            vec3 emitted_light = emission_color * emission_strength;
            light += emitted_light * color;
            vec3 material_color = material.color_smoothness.xyz;
            float smoothness = material.color_smoothness.w;
            color *= material_color;
            vec3 diffuse_direction = hit.normal + random_unit_vector(seed);
            vec3 specular_direction =
                reflect(ray.direction, hit.normal);
            vec3 direction =
                lerp(diffuse_direction, specular_direction, smoothness);

            ray = Ray(hit.point + hit.normal * 1e-8, direction);
        } else {
            vec3 unit_direction = normalize(ray.direction);
            float a = 0.5 * (unit_direction.y + 1.0);
            vec3 environment_light = ((1.0 - a) * vec3(1.0, 1.0, 1.0) +
                                        a * vec3(0.1, 0.4, 1.0));
            float sky_intensity = 1;
            // dvec3 environment_light{0, 0, 0};
            light += color * environment_light * sky_intensity;
            break;
        }
    }

    return light;
}

uint hash(uvec2 p) {
    p = 1664525u * (p ^ (p >> 15u));
    p += 1013904223u;
    p ^= (p >> 16u);
    return p.x ^ p.y;
}

void main()
{
    vec2 viewport = vec2( (resolution.x / resolution.y) * viewport_height, viewport_height );

    mat4 cam_matrix = get_cam_matrix();

    vec3 viewport_u =
            (cam_matrix * vec4(viewport.x, 0, 0, 1.0) - vec4(camera.position.xyz, 0)).xyz;

    vec3 viewport_v = (cam_matrix * vec4(0, viewport.y, 0, 1.0) -
                        vec4(camera.position.xyz, 0)).xyz;

    vec3 dx = viewport_u * (1.0 / resolution.x);
    vec3 dy = viewport_v * (1.0 / resolution.y);

    vec3 lt = get_left_top(viewport);

    vec3 pos = lt + (gl_FragCoord.x) * (dx) + (gl_FragCoord.y) * dy;
    
    Ray ray = Ray(
        camera.position.xyz,
        pos - camera.position.xyz
    );

    uint seed = hash(uvec2(gl_FragCoord.xy));

    vec3 sum = vec3(0, 0, 0);

    for (int i = 0; i < 150; i++) {
        sum += trace_ray(ray, 2, seed);
    }

    FragColor = vec4(sum / 150.0, 1);
}
