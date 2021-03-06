#version 300 es

precision mediump float;

uniform sampler2D pog;
uniform float time;

in vec2 uv;
in vec4 color;
out vec4 frag_color;

void main(void) {
    frag_color = mix(
        texture(pog, uv),
        vec4(0.05, 0.0, 0.0, 1.0),
        gl_FragCoord.z);
}
