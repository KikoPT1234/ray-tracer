#version 460 core

struct Camera {
    vec4 position;
    vec4 direction;
};

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct Sphere {
    vec4 position_radius;
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

    HitInfo info;
    info.t = 1.0 / 0.0;

    float epsilon = 1e-8;

    for (int i = 0; i < sphereCount; i++) {
        Sphere sphere = spheres[i];

        HitInfo temp;
        sphere_intersection(sphere, ray, temp);

        if (temp.did_hit) {
            if (temp.t < info.t && temp.t > epsilon) {
                info = temp;
            }
        }        
    }

    if (info.did_hit)
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    else
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
