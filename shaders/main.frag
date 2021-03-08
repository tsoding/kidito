#version 300 es

precision mediump float;

uniform sampler2D pog;
uniform float time;

in vec2 uv;
in vec4 vertex;
out vec4 frag_color;

#define FOG_MIN 1.0
#define FOG_MAX 50.0

float fog_factor(float d)
{
    if (d <= FOG_MIN) return 0.0;
    if (d >= FOG_MAX) return 1.0;
    return 1.0 - (FOG_MAX - d) / (FOG_MAX - FOG_MIN);
}

void main(void) {
    frag_color = mix(
        texture(pog, uv),
        vec4(0.0, 0.0, 0.0, 1.0),
        fog_factor(length(vertex)));
}
