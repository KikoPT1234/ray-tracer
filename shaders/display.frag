#version 460 core

out vec4 fragColor;

uniform sampler2D image;
uniform vec2 resolution;
// Multiplier on the linear image before tone mapping.
uniform float exposure;

// Khronos PBR Neutral tone mapping. Leaves colours untouched until they get
// bright, then compresses the peak channel and blends toward white, so
// saturated colours keep their hue instead of washing out.
vec3 pbr_neutral(vec3 color) {
    const float start_compression = 0.8 - 0.04;
    const float desaturation = 0.15;

    float x = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;

    float peak = max(color.r, max(color.g, color.b));
    if (peak < start_compression) return color;

    float d = 1.0 - start_compression;
    float new_peak = 1.0 - d * d / (peak + d - start_compression);
    color *= new_peak / peak;

    float g = 1.0 - 1.0 / (desaturation * (peak - new_peak) + 1.0);
    return mix(color, vec3(new_peak), g);
}

// Exact sRGB encoding (linear segment near black, 2.4 power above it).
vec3 linear_to_srgb(vec3 c) {
    return mix(12.92 * c, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055,
               step(0.0031308, c));
}

void main()
{
    vec2 uv = (gl_FragCoord.xy) / resolution;
    vec3 color = texture(image, uv).xyz;

    fragColor = vec4(linear_to_srgb(pbr_neutral(color * exposure)), 1.0);
}
