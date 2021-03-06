#version 300 es

precision mediump float;

uniform mat4 matrix;
uniform float time;

layout(location = 1) in vec4 vertex_position;
layout(location = 2) in vec4 vertex_color;
layout(location = 3) in vec2 vertex_uv;

out vec2 uv;
out vec4 color;

#define FIXUP_SCALE 0.5
#define MESH_FIXUP_LOCATION vec4(-0.5, -0.5, -0.5, 0.0)
#define MESH_FIXUP_SCALE vec4(FIXUP_SCALE, FIXUP_SCALE, FIXUP_SCALE, 1.0)

#define ROTATION_X
#define ROTATION_Y
#define ROTATION_Z

void main(void)
{
    vec4 pos = (vertex_position + MESH_FIXUP_LOCATION) * MESH_FIXUP_SCALE;

    // Rotation

#ifdef ROTATION_X
    {
        float pz = pos.z * cos(time) - pos.y * sin(time);
        float py = pos.z * sin(time) + pos.y * cos(time);
        pos.z = pz;
        pos.y = py;
    }
#endif // ROTATION_X

#ifdef ROTATION_Y
    {
        float px = pos.x * cos(time) - pos.z * sin(time);
        float pz = pos.x * sin(time) + pos.z * cos(time);
        pos.x = px;
        pos.z = pz;
    }
#endif // ROTATION_Y

#ifdef ROTATION_Z
    {
        float px = pos.x * cos(time) - pos.y * sin(time);
        float py = pos.x * sin(time) + pos.y * cos(time);
        pos.x = px;
        pos.y = py;
    }
#endif // ROTATION_Z

    float near = 0.25;
 
    // Translate
    pos.z += 0.5;

    // Projection
    pos.x = (pos.x * near) / pos.z;
    pos.y = (pos.y * near) / pos.z;

    uv = vertex_uv;
    // uv.x = (vertex_uv.x * near) / pos.z;
    // uv.y = (vertex_uv.y * near) / pos.z;


    // Output
    gl_Position = pos;
    color = vertex_color;
}
 
