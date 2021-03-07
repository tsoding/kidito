#ifndef GEO_H_
#define GEO_H_

#include "./rgba.h"

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

void generate_cube_face_mesh(size_t a, size_t b,
                             size_t c, float cv,
                             size_t d, float dv,
                             V4 mesh[TRIS_PER_FACE][TRI_VERTICES]);
void generate_cube_mesh(V4 mesh[TRIS_PER_CUBE][TRI_VERTICES],
                        RGBA colors[TRIS_PER_CUBE][TRI_VERTICES],
                        V2 uvs[TRIS_PER_CUBE][TRI_VERTICES]);

typedef struct {
    float vs[4][4];
} Mat4x4;

Mat4x4 rotation_mat4x4_y(float angle);
Mat4x4 rotation_mat4x4_z(float angle);


#endif // GEO_H_
