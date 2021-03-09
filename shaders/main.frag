#version 300 es

precision mediump float;

uniform sampler2D pog;
uniform float time;

in vec2 uv;
in vec4 vertex;
in vec4 normal;
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
    vec3 light_source = vec3(0.0, 0.0, 00.0);
    float a = abs(dot(normalize(light_source - vertex.xyz), normal.xyz));
    vec4 t = texture(pog, uv);

    frag_color = mix(
        vec4(t.xyz * vec3(1.0, 1.0, 1.0) * a, 1.0),
        vec4(0.0, 0.0, 0.0, 1.0),
        fog_factor(length(vertex)));
}
