#version 460 core

out vec4 fragColor;

uniform sampler2D image;
uniform vec2 resolution;

void main()
{
    vec2 uv = (gl_FragCoord.xy) / resolution;
    fragColor = texture(image, uv);
}