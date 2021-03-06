#version 300 es

precision mediump float;

uniform sampler2D pog;
uniform float time;

in vec2 uv;
in vec4 color;
out vec4 frag_color;

void main(void) {
    frag_color = color;
}

