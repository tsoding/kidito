#version 300 es

precision mediump float;

uniform sampler2D pog;

in vec2 uv;
out vec4 color;

void main(void) {
    color = texture(pog, uv);
};
