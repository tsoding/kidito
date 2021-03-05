#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "./geo.h"

V4 v4_add(V4 a, V4 b)
{
    for (size_t i = 0; i < TRI_VERTICES; ++i) {
        a.cs[i] += b.cs[i];
    }
    return a;
}

V4 v4_scale(V4 a, float s)
{
    for (size_t i = 0; i < TRI_VERTICES; ++i) {
        a.cs[i] *= s;
    }
    return a;
}

void tri_translate(Tri *tri, V4 dir)
{
    for (size_t i = 0; i < TRI_VERTICES; ++i) {
        tri->vs[i] = v4_add(tri->vs[i], dir);
    }

}
void cube_face(size_t a, size_t b, size_t c, float cv,
               Tri mesh[TRIS_PER_FACE])
{
    for (size_t i = 0; i < TRIS_PER_FACE; ++i) {
        for (size_t j = 0; j < TRI_VERTICES; ++j) {
            size_t k = i + j;
            mesh[i].vs[j].cs[a] = (float) (k & 1);
            mesh[i].vs[j].cs[b] = (float) (k >> 1);
            mesh[i].vs[j].cs[c] = cv;
        }
    }
}

void cube(Tri mesh[TRIS_PER_CUBE])
{
    size_t count = 0;

    Tri face_mesh[TRIS_PER_FACE] = {0};


    size_t face_pairs[CUBE_FACE_PAIRS][V3_COMPS] = {
        {X, Y, Z},
        {Z, Y, X},
        {X, Z, Y}
    };

    for (size_t i = 0; i < CUBE_FACE_PAIRS; ++i) {
        for (size_t j = 0; j < PAIR_COMPS; ++j) {
            cube_face(face_pairs[i][0],
                      face_pairs[i][1],
                      face_pairs[i][2],
                      (float) j,
                      face_mesh);

            for (size_t k = 0; k < TRIS_PER_FACE; ++k) {
                mesh[count++] = face_mesh[k];
            }
        }
    }

    assert(count == TRIS_PER_CUBE);
}
