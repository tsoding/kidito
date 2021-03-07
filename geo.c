#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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

void generate_cube_face_mesh(size_t a, size_t b,
                             size_t c, float cv,
                             size_t d, float dv,
                             V4 mesh[TRIS_PER_FACE][TRI_VERTICES])
{
    for (size_t i = 0; i < TRIS_PER_FACE; ++i) {
        for (size_t j = 0; j < TRI_VERTICES; ++j) {
            size_t k = i + j;
            mesh[i][j].cs[a] = (float) (k & 1);
            mesh[i][j].cs[b] = (float) (k >> 1);
            mesh[i][j].cs[c] = cv;
            mesh[i][j].cs[d] = dv;
        }
    }
}

void generate_cube_mesh(V4 mesh[TRIS_PER_CUBE][TRI_VERTICES],
                        RGBA colors[TRIS_PER_CUBE][TRI_VERTICES],
                        V2 uvs[TRIS_PER_CUBE][TRI_VERTICES])
{
    size_t count = 0;

    static const RGBA face_colors[CUBE_FACES] = {
        RED,
        GREEN,
        BLUE,
        YELLOW,
        PURPLE,
        CYAN
    };

    static const size_t face_pairs[CUBE_FACE_PAIRS][V4_COMPS] = {
        {X, Y, Z, W},
        {Z, Y, X, W},
        {X, Z, Y, W}
    };

    for (size_t i = 0; i < CUBE_FACE_PAIRS; ++i) {
        for (size_t j = 0; j < PAIR_COMPS; ++j) {
            generate_cube_face_mesh(
                face_pairs[i][0],
                face_pairs[i][1],
                face_pairs[i][2], (float) j,
                face_pairs[i][3], 1.0f,
                &mesh[count]);

            for (size_t k = 0; k < TRIS_PER_FACE; ++k) {
                for (size_t q = 0; q < TRI_VERTICES; ++q) {
                    colors[count + k][q] = face_colors[2 * i + j];

                    size_t t = k + q;
                    uvs[count + k][q].cs[X] = (float) (t & 1);
                    uvs[count + k][q].cs[Y] = (float) (t >> 1);
                }
            }

            count += 2;
        }
    }

    assert(count == TRIS_PER_CUBE);
}

// https://en.wikipedia.org/wiki/Rotation_matrix
Mat4x4 rotation_mat4x4_y(float angle)
{
    return (Mat4x4) {
        .vs = {
            { cosf(angle), 0.0f, sinf(angle), 0.0f},
            {        0.0f, 1.0f,       0.0f, 0.0f},
            {-sinf(angle), 0.0f, cosf(angle), 0.0f},
            {        0.0f, 0.0f,       0.0f, 1.0f},
        }
    };
}

Mat4x4 rotation_mat4x4_z(float angle)
{
    return (Mat4x4) {
        .vs = {
            {cosf(angle), -sinf(angle), 0.0f, 0.0f},
            {sinf(angle),  cosf(angle), 0.0f, 0.0f},
            {       0.0f,         0.0f, 1.0f, 0.0f},
            {       0.0f,         0.0f, 0.0f, 1.0f},
        }
    };
}
