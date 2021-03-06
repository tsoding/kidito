#ifndef RGBA_H_
#define RGBA_H_

#define RGBA_COMPS 4

typedef struct {
    float cs[RGBA_COMPS];
} RGBA;

#define RGBA_Fmt "RGBA(%f,%f,%f,%f)"
#define RGBA_Arg(rgba) rgba.cs[0], rgba.cs[1], rgba.cs[2], rgba.cs[3]

#define RED    (RGBA) {.cs = {1.0f, 0.0f, 0.0f, 1.0f}}
#define GREEN  (RGBA) {.cs = {0.0f, 1.0f, 0.0f, 1.0f}}
#define BLUE   (RGBA) {.cs = {0.0f, 0.0f, 1.0f, 1.0f}}
#define YELLOW (RGBA) {.cs = {1.0f, 1.0f, 0.0f, 1.0f}}
#define PURPLE (RGBA) {.cs = {1.0f, 0.0f, 1.0f, 1.0f}}
#define CYAN   (RGBA) {.cs = {0.0f, 1.0f, 1.0f, 1.0f}}

#endif // RGBA_H_
