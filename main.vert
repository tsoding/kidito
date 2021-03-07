#version 300 es

precision mediump float;

uniform mat4 matrix;
uniform float time;

layout(location = 1) in vec4 vertex_position;
layout(location = 2) in vec4 vertex_color;
layout(location = 3) in vec2 vertex_uv;

out vec2 uv;
out vec4 color;

void main(void)
{
    gl_Position = matrix * vertex_position;
    color = vertex_color;
    uv = vertex_uv;
}
 
