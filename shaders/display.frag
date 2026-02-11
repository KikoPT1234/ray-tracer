#version 460 core

out vec4 fragColor;

uniform sampler2D image;
uniform vec2 resolution;

vec3 aces(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 gamma_correct(vec3 v) {
    float x = pow(v.x, 1.0 / 2.2);
    float y = pow(v.y, 1.0 / 2.2);
    float z = pow(v.z, 1.0 / 2.2);

    return vec3(x, y, z);
}

void main()
{
    vec2 uv = (gl_FragCoord.xy) / resolution;
    vec3 color = texture(image, uv).xyz;

    fragColor = vec4(gamma_correct(aces(color)), 1.0);
}