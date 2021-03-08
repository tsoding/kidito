#include <stdio.h>
#include <stdlib.h>

#include "geo.h"

int main()
{
    V4 mesh[TRIS_PER_CUBE][TRI_VERTICES];
    RGBA colors[TRIS_PER_CUBE][TRI_VERTICES];
    V2 uvs[TRIS_PER_CUBE][TRI_VERTICES];

    generate_cube_mesh(mesh, colors, uvs);

    for (size_t tri = 0; tri < TRIS_PER_CUBE; ++tri) {
        for (size_t vert = 0; vert < TRI_VERTICES; ++vert) {
            printf("v %f %f %f %f\n",
                   mesh[tri][vert].cs[X],
                   mesh[tri][vert].cs[Y],
                   mesh[tri][vert].cs[Z],
                   mesh[tri][vert].cs[W]);
            printf("vt %f %f\n",
                   uvs[tri][vert].cs[X],
                   uvs[tri][vert].cs[Y]);
        }
    }

    return 0;
}
