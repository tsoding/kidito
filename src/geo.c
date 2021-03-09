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

void generate_cube_mesh(V4 mesh[TRIS_PER_CUBE][TRI_VERTICES],
                        RGBA colors[TRIS_PER_CUBE][TRI_VERTICES],
                        V2 uvs[TRIS_PER_CUBE][TRI_VERTICES],
                        V4 normals[TRIS_PER_CUBE][TRI_VERTICES])
{
    size_t count = 0;

    static const RGBA face_colors[CUBE_FACE_PAIRS][PAIR_COMPS] = {
        {RED, GREEN},
        {BLUE, YELLOW},
        {PURPLE, CYAN},
    };

    static const size_t face_pairs[CUBE_FACE_PAIRS][V3_COMPS] = {
        {X, Y, Z},
        {Z, Y, X},
        {X, Z, Y}
    };

    for (size_t face_pair_index = 0; face_pair_index < CUBE_FACE_PAIRS; ++face_pair_index) {
        for (size_t pair_comp_index = 0; pair_comp_index < PAIR_COMPS; ++pair_comp_index) {
            for (size_t tri = 0; tri < TRIS_PER_FACE; ++tri) {
                for (size_t vert = 0; vert < TRI_VERTICES; ++vert) {
                    const size_t strip_index = tri + vert;
                    const size_t A = face_pairs[face_pair_index][0];
                    const size_t B = face_pairs[face_pair_index][1];
                    const size_t C = face_pairs[face_pair_index][2];

                    // Mesh
                    {

                        mesh[count + tri][vert].cs[A] = (float) (strip_index & 1);
                        mesh[count + tri][vert].cs[B] = (float) (strip_index >> 1);
                        mesh[count + tri][vert].cs[C] = (float) pair_comp_index;;
                        mesh[count + tri][vert].cs[W] = 1.0f;
                    }

                    // Color
                    {
                        colors[count + tri][vert] =
                            face_colors[face_pair_index][pair_comp_index];
                    }

                    // UVs
                    {
                        uvs[count + tri][vert].cs[X] = (float) (strip_index & 1);
                        uvs[count + tri][vert].cs[Y] = (float) (strip_index >> 1);
                    }

                    // Normals
                    {
                        normals[count + tri][vert].cs[A] = 0.0f;
                        normals[count + tri][vert].cs[B] = 0.0f;
                        normals[count + tri][vert].cs[Z] = (float) (2 * (int) pair_comp_index - 1);
                        normals[count + tri][vert].cs[W] = 1.0f;
                    }
                }
            }

            count += TRIS_PER_FACE;
        }
    }

    assert(count == TRIS_PER_CUBE);
}

Mat4 mat4_id(void)
{
    return (Mat4) {
        .vs = {
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f},
        },
    };
}

V4 mat4_mult_v4(Mat4 mat, V4 vec)
{
    V4 result = {0};
    for (int row = 0; row < V4_COMPS; ++row) {
        for (int col = 0; col < V4_COMPS; ++col) {
            result.cs[row] += mat.vs[row][col] * vec.cs[col];
        }
    }
    return result;
}

Mat4 mat4_mult_mat4(Mat4 m1, Mat4 m2)
{
    Mat4 result = {0};

    for (int row = 0; row < V4_COMPS; ++row) {
        for (int col = 0; col < V4_COMPS; ++col) {
            for (int t = 0; t < V4_COMPS; ++t) {
                result.vs[row][col] += m1.vs[row][t] * m2.vs[t][col];
            }
        }
    }

    return result;
}

Mat4 mat4_translate(float x, float y, float z)
{
    return (Mat4) {
        .vs = {
            {1.0f, 0.0f, 0.0f, x},
            {0.0f, 1.0f, 0.0f, y},
            {0.0f, 0.0f, 1.0f, z},
            {0.0f, 0.0f, 0.0f, 1.0f},
        }
    };
}

Mat4 mat4_scale(float x, float y, float z)
{
    return (Mat4) {
        .vs = {
            {   x, 0.0f, 0.0f, 0.0f},
            {0.0f,    y, 0.0f, 0.0f},
            {0.0f, 0.0f,    z, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f},
        }
    };
}

// https://en.wikipedia.org/wiki/Rotation_matrix
Mat4 mat4_rotate_y(float angle)
{
    return (Mat4) {
        .vs = {
            { cosf(angle), 0.0f, sinf(angle), 0.0f},
            {        0.0f, 1.0f,       0.0f, 0.0f},
            {-sinf(angle), 0.0f, cosf(angle), 0.0f},
            {        0.0f, 0.0f,       0.0f, 1.0f},
        }
    };
}

Mat4 mat4_rotate_z(float angle)
{
    return (Mat4) {
        .vs = {
            {cosf(angle), -sinf(angle), 0.0f, 0.0f},
            {sinf(angle),  cosf(angle), 0.0f, 0.0f},
            {       0.0f,         0.0f, 1.0f, 0.0f},
            {       0.0f,         0.0f, 0.0f, 1.0f},
        }
    };
}

// NOTE: stolen from glm::perspectiveRH_NO()
Mat4 mat4_perspective(float fovy, float aspect, float near, float far)
{
    const float tan_half_fovy = tanf(fovy * 0.5f);

    Mat4 result = {0};
    result.vs[0][0] = 1.0f / (aspect * tan_half_fovy);
    result.vs[1][1] = 1.0f / tan_half_fovy;
    result.vs[2][2] = far / (near - far);
    result.vs[2][3] = -1.0f;
    result.vs[3][2] = -(far * near) / (far - near);
    return result;
}
