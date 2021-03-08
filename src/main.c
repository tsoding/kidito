#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#define GLEW_STATIC
#include <GL/glew.h>

#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

#include "./geo.h"
#include "./sv.h"

#define MEMORY_CAPACITY (1 * 1000 * 1000)
char memory[MEMORY_CAPACITY] = {0};
size_t memory_size = 0;

void *memory_malloc(size_t size)
{
    if (memory_size + size >= MEMORY_CAPACITY) {
        errno = ENOMEM;
        return NULL;
    }

    void *result = memory + memory_size;
    memory_size += size;
    return result;
}

void *memory_realloc(void *old_memory, size_t old_size, size_t new_size)
{
    void *new_memory = memory_malloc(new_size);
    memcpy(new_memory, old_memory, old_size);
    return new_memory;
}

#define STBI_MALLOC(size) memory_malloc(size)
#define STBI_FREE(ignored) do {(void)ignored;} while(0)
#define STBI_REALLOC_SIZED(ptr,oldsz,newsz) memory_realloc(ptr,oldsz,newsz)

#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

#define MANUAL_TIME_STEP 0.05f
#define HOT_RELOAD_ERROR_COLOR 1.0f, 0.0f, 0.0f, 1.0f
#define BACKGROUND_COLOR 0.0f, 0.0f, 0.0f, 0.0f

char *cstr_from_sv(String_View sv)
{
    char *result = memory_malloc(sv.count + 1);
    memcpy(result, sv.data, sv.count);
    result[sv.count] = '\0';
    return result;
}

char *slurp_file(const char *file_path)
{
    FILE *f = NULL;
    char *buffer = NULL;

    f = fopen(file_path, "r");
    if (f == NULL) goto end;
    if (fseek(f, 0, SEEK_END) < 0) goto end;

    long size = ftell(f);
    if (size < 0) goto end;

    buffer = memory_malloc(size + 1);
    if (buffer == NULL) goto end;

    if (fseek(f, 0, SEEK_SET) < 0) goto end;

    fread(buffer, 1, size, f);
    if (ferror(f) < 0) goto end;

    buffer[size] = '\0';

end:
    if (f) fclose(f);
    return buffer;
}

bool compile_shader_source(const GLchar *source, GLenum shader_type, GLuint *shader)
{
    *shader = glCreateShader(shader_type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);

    GLint compiled = 0;
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        GLchar message[1024];
        GLsizei message_size = 0;
        glGetShaderInfoLog(*shader, sizeof(message), &message_size, message);
        fprintf(stderr, "%.*s\n", message_size, message);
        return false;
    }

    return true;
}

bool compile_shader_file(const char *file_path, GLenum shader_type, GLuint *shader)
{
    char *source = slurp_file(file_path);
    if (source == NULL) {
        fprintf(stderr, "ERROR: could not slurp the file %s: %s\n",
                file_path, strerror(errno));
        return false;
    }

    bool err = compile_shader_source(source, shader_type, shader);
    return err;
}

bool link_program(GLuint vert_shader, GLuint frag_shader, GLuint *program)
{
    *program = glCreateProgram();

    glAttachShader(*program, vert_shader);
    glAttachShader(*program, frag_shader);
    glLinkProgram(*program);

    GLint linked = 0;
    glGetProgramiv(*program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLsizei message_size = 0;
        GLchar message[1024];

        glGetProgramInfoLog(*program, sizeof(message), &message_size, message);
        fprintf(stderr, "Program Linking: %.*s\n", message_size, message);
    }

    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);

    return program;
}

// Global variables (fragile people with CS degree look away)
bool program_failed = false;
GLuint program = 0;

double time = 0.0;
GLint time_location = 0;
bool pause = false;
GLint resolution_location = 0;

GLuint texture_id = 0;

void reload_shaders(void)
{
    const char *scene_conf_file_path = "./scene.conf";
    const char *vertex_shader_file_path = "./main.vert";
    const char *fragment_shader_file_path = "./main.frag";
    const char *texture_file_path = "yep.png";

    glClearColor(BACKGROUND_COLOR);
    program_failed = false;

    String_View scene_conf_content = sv_from_cstr(slurp_file(scene_conf_file_path));
    if (scene_conf_content.data == NULL) {
        glClearColor(HOT_RELOAD_ERROR_COLOR);
        program_failed = true;
        return;
    }

    for (size_t line_number = 0; scene_conf_content.count > 0; line_number++) {
        String_View line = sv_chop_by_delim(&scene_conf_content, '\n');
        line = sv_trim(sv_chop_by_delim(&line, '#'));

        if (line.count > 0) {
            String_View key = sv_trim(sv_chop_by_delim(&line, '='));
            String_View value = sv_trim(line);
            if (sv_eq(key, SV("frag_shader"))) {
                fragment_shader_file_path = cstr_from_sv(value);
            } else if (sv_eq(key, SV("vert_shader"))) {
                vertex_shader_file_path = cstr_from_sv(value);
            } else if (sv_eq(key, SV("texture"))) {
                texture_file_path = cstr_from_sv(value);
            } else {
                printf("%s:%zu: WARNING: unknown key `"SV_Fmt"`\n",
                       scene_conf_file_path, line_number,
                       SV_Arg(key));
            }
        }
    }

    glDeleteProgram(program);

    GLuint vert = 0;
    if (!compile_shader_file(vertex_shader_file_path, GL_VERTEX_SHADER, &vert)) {
        glClearColor(HOT_RELOAD_ERROR_COLOR);
        program_failed = true;
        return;
    }

    GLuint frag = 0;
    if (!compile_shader_file(fragment_shader_file_path, GL_FRAGMENT_SHADER, &frag)) {
        glClearColor(HOT_RELOAD_ERROR_COLOR);
        program_failed = true;
        return;
    }

    if (!link_program(vert, frag, &program)) {
        glClearColor(HOT_RELOAD_ERROR_COLOR);
        program_failed = true;
        return;
    }

    glUseProgram(program);
    time_location = glGetUniformLocation(program, "time");
    resolution_location = glGetUniformLocation(program, "resolution");

    glDeleteTextures(1, &texture_id);

    int w, h;
    uint32_t *pixels = (uint32_t*) stbi_load(texture_file_path, &w, &h, NULL, 4);
    if (pixels == NULL) {
        printf("ERROR: could not load file %s: %s\n", texture_file_path, strerror(errno));
        glClearColor(HOT_RELOAD_ERROR_COLOR);
        program_failed = true;
        return;
    }

    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 w,
                 h,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    printf("Successfully reloaded Shaders and Textures\n");
    printf("Memory %zu/%zu\n", memory_size, (size_t) MEMORY_CAPACITY);
    memory_size = 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void) window;
    (void) scancode;
    (void) action;
    (void) mods;

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_F5) {
            reload_shaders();
        } else if (key == GLFW_KEY_SPACE) {
            pause = !pause;
        }

        if (pause) {
            if (key == GLFW_KEY_LEFT) {
                time -= MANUAL_TIME_STEP;
            } else if (key == GLFW_KEY_RIGHT) {
                time += MANUAL_TIME_STEP;
            }
        }
    }
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
    (void) window;
    glViewport(0, 0, width, height);
}

GLuint array_buffer_from_data(void *data, size_t data_size)
{
    GLuint buffer_id;
    glGenBuffers(1, &buffer_id);
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
    glBufferData(GL_ARRAY_BUFFER,
                 data_size,
                 data,
                 GL_STATIC_DRAW);
    return buffer_id;
}

void MessageCallback(GLenum source,
                     GLenum type,
                     GLuint id,
                     GLenum severity,
                     GLsizei length,
                     const GLchar* message,
                     const void* userParam)
{
    (void) source;
    (void) id;
    (void) length;
    (void) userParam;
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            type, severity, message);
}

int main()
{
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not initialize GLFW\n");
        exit(1);
    }

    GLFWwindow * const window =
        glfwCreateWindow(
            800,
            600,
            "atomato",
            NULL,
            NULL);
    if (window == NULL) {
        fprintf(stderr, "ERROR: could not create a window.\n");
        glfwTerminate();
        exit(1);
    }

    glfwMakeContextCurrent(window);

    if (GLEW_OK != glewInit()) {
        fprintf(stderr, "Could not initialize GLEW!\n");
        exit(1);
    }


    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    reload_shaders();

    // TODO: hot reloadable mesh from an .obj file
    V4 cube_mesh[TRIS_PER_CUBE][TRI_VERTICES] = {0};
    RGBA cube_colors[TRIS_PER_CUBE][TRI_VERTICES] = {0};
    V2 cube_uvs[TRIS_PER_CUBE][TRI_VERTICES] = {0};
    generate_cube_mesh(cube_mesh, cube_colors, cube_uvs);

    {
        array_buffer_from_data(cube_mesh, sizeof(cube_mesh));

        const GLint position = 1;
        glEnableVertexAttribArray(position);

        glVertexAttribPointer(
            position,           // index
            V4_COMPS,           // numComponents
            GL_FLOAT,           // type
            0,                  // normalized
            0,                  // stride
            0                   // offset
        );
    }

    {
        array_buffer_from_data(cube_colors, sizeof(cube_colors));

        const GLint position = 2;
        glEnableVertexAttribArray(position);

        glVertexAttribPointer(
            position,           // index
            RGBA_COMPS,         // numComponents
            GL_FLOAT,           // type
            0,                  // normalized
            0,                  // stride
            0                   // offset
        );
    }

    {
        array_buffer_from_data(cube_uvs, sizeof(cube_uvs));

        const GLint position = 3;
        glEnableVertexAttribArray(position);

        glVertexAttribPointer(
            position,           // index
            V2_COMPS,           // numComponents
            GL_FLOAT,           // type
            0,                  // normalized
            0,                  // stride
            0                   // offset
        );
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, window_size_callback);
    double prev_time = 0.0;
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!program_failed) {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            glUniform2f(resolution_location, width, height);
            glUniform1f(time_location, time);
            glDrawArrays(GL_TRIANGLES, 0, TRIS_PER_CUBE * TRI_VERTICES);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
        double cur_time = glfwGetTime();
        if (!pause) {
            time += cur_time - prev_time;
        }
        prev_time = cur_time;
    }

    return 0;
}
