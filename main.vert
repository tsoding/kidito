#version 300 es

precision mediump float;

uniform mat4 matrix;

layout(location = 1) in vec4 position;

out vec2 uv;

#define MESH_FIXUP vec4(-0.5, -0.5, -0.5, 0.0)

void main(void)
{
    vec4 position = (matrix * ((position + MESH_FIXUP) / vec4(2.0, 2.0, 2.0, 1.0))) + vec4(0.0, 0.0, 1.0, 0.0);
    gl_Position = vec4(position.x / position.z, position.y / position.z, position.z, position.w);

    vec4 tuv = (matrix * position);
    uv = tuv.xy;
}
