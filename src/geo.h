#ifndef GEO_H_
#define GEO_H_

#include "./rgba.h"

#define MY_PI 3.14159265359f

#define X 0
#define Y 1
#define Z 2
#define W 3

#define V2_COMPS 2

typedef struct {
    float cs[V2_COMPS];
} V2;

#define V2_Fmt "V2(%f, %f)"
#define V2_Arg(v2) v2.cs[X], v2.cs[Y]

#define V3_COMPS 3
#define V4_COMPS 4
#define PAIR_COMPS 2

typedef struct {
    float cs[V4_COMPS];
} V4;

#define V4_Fmt "V4(%f, %f, %f, %f)"
#define V4_Arg(v4) v4.cs[X], v4.cs[Y], v4.cs[Z], v4.cs[W]

V4 v4_add(V4 a, V4 b);
V4 v4_scale(V4 a, float s);

#define CUBE_FACE_PAIRS 3
#define CUBE_FACES (CUBE_FACE_PAIRS * PAIR_COMPS)

#define TRI_VERTICES 3
#define TRIS_PER_FACE 2
#define TRIS_PER_CUBE (CUBE_FACES * TRIS_PER_FACE)

void generate_cube_mesh(V4 mesh[TRIS_PER_CUBE][TRI_VERTICES],
                        RGBA colors[TRIS_PER_CUBE][TRI_VERTICES],
                        V2 uvs[TRIS_PER_CUBE][TRI_VERTICES],
                        V4 normals[TRIS_PER_CUBE][TRI_VERTICES]);

typedef struct {
    float vs[V4_COMPS][V4_COMPS];
} Mat4;

V4 mat4_mult_v4(Mat4 mat, V4 vec);
Mat4 mat4_mult_mat4(Mat4 m1, Mat4 m2);

Mat4 mat4_id(void);
Mat4 mat4_translate(float x, float y, float z);
Mat4 mat4_scale(float x, float y, float z);
Mat4 mat4_rotate_y(float angle);
Mat4 mat4_rotate_z(float angle);
Mat4 mat4_perspective(float fovy, float aspect, float near, float far);

#endif // GEO_H_
