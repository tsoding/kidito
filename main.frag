#version 300 es

precision mediump float;

uniform sampler2D pog;
uniform float time;

in vec2 uv;
out vec4 color;

void main(void) {
    // color = vec4(1.0, 0.0, 0.0, 1.0);
    // color = vec4(uv.x, uv.y, 0.0, 1.0);
    color = texture(pog, uv);
}

