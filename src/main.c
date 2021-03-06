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
#include "./region.h"

Region hot_reload_memory;

#define STBI_MALLOC(size) region_malloc(&hot_reload_memory, size)
#define STBI_FREE(ignored) do {(void)ignored;} while(0)
#define STBI_REALLOC_SIZED(ptr, oldsz, newsz) \
    region_realloc(&hot_reload_memory, ptr, oldsz, newsz)

#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

#define VERTEX_CAPACITY 1000

#define MANUAL_TIME_STEP 0.05f
#define HOT_RELOAD_ERROR_COLOR 0.5f, 0.0f, 0.0f, 1.0f
#define BACKGROUND_COLOR 0.0f, 0.0f, 0.0f, 0.0f

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

void reload_scene(void)
{
    const char *const scene_conf_file_path = "./scene.conf";
    const char *vertex_shader_file_path = NULL;
    size_t vertex_shader_def_line = 0;
    const char *fragment_shader_file_path = NULL;
    size_t fragment_shader_def_line = 0;
    const char *texture_file_path = NULL;
    size_t texture_def_line = 0;

    glClearColor(HOT_RELOAD_ERROR_COLOR);
    program_failed = true;

    // reload scene.conf begin
    {
        String_View scene_conf_content = sv_from_cstr(region_slurp_file(&hot_reload_memory, scene_conf_file_path));
        if (scene_conf_content.data == NULL) {
            return;
        }

        for (size_t line_number = 0; scene_conf_content.count > 0; line_number++) {
            String_View line = sv_chop_by_delim(&scene_conf_content, '\n');
            line = sv_trim(sv_chop_by_delim(&line, '#'));

            if (line.count > 0) {
                String_View key = sv_trim(sv_chop_by_delim(&line, '='));
                String_View value = sv_trim(line);
                if (sv_eq(key, SV("frag_shader"))) {
                    fragment_shader_file_path = region_cstr_from_sv(&hot_reload_memory, value);
                    fragment_shader_def_line = line_number;
                } else if (sv_eq(key, SV("vert_shader"))) {
                    vertex_shader_file_path = region_cstr_from_sv(&hot_reload_memory, value);
                    vertex_shader_def_line = line_number;
                } else if (sv_eq(key, SV("texture"))) {
                    texture_file_path = region_cstr_from_sv(&hot_reload_memory, value);
                    texture_def_line = line_number;
                } else {
                    printf("%s:%zu: WARNING: unknown key `"SV_Fmt"`\n",
                           scene_conf_file_path, line_number,
                           SV_Arg(key));
                }
            }
        }

        if (vertex_shader_file_path == NULL) {
            fprintf(stderr, "ERROR: `vert_shader` is not specified in %s\n",
                    scene_conf_file_path);
            return;
        }

        if (fragment_shader_file_path == NULL) {
            fprintf(stderr, "ERROR: `frag_shader` is not specified in %s\n",
                    scene_conf_file_path);
            return;
        }

        if (texture_file_path == NULL) {
            fprintf(stderr, "ERROR: `texture` is not specified in %s\n",
                    scene_conf_file_path);
            return;
        }
    }
    // reload scene.conf end

    // reload shader program begin
    {
        glDeleteProgram(program);

        char *vert_source = region_slurp_file(&hot_reload_memory, vertex_shader_file_path);
        if (vert_source == NULL) {
            fprintf(stderr, "%s:%zu: ERROR: Could not read file `%s`: %s\n",
                    scene_conf_file_path, vertex_shader_def_line, vertex_shader_file_path, strerror(errno));
            return;
        }

        GLuint vert = 0;
        if (!compile_shader_source(vert_source, GL_VERTEX_SHADER, &vert)) {
            fprintf(stderr, "%s:%zu: ERROR: Failed to compile vertex shader `%s`\n",
                    scene_conf_file_path, vertex_shader_def_line, vertex_shader_file_path);
            return;
        }

        char *frag_source = region_slurp_file(&hot_reload_memory, fragment_shader_file_path);
        if (frag_source == NULL) {
            fprintf(stderr, "%s:%zu: ERROR: Could not read file `%s`: %s\n",
                    scene_conf_file_path, fragment_shader_def_line, fragment_shader_file_path, strerror(errno));
            return;
        }

        GLuint frag = 0;
        if (!compile_shader_source(frag_source, GL_FRAGMENT_SHADER, &frag)) {
            fprintf(stderr, "%s:%zu: ERROR: Failed to compile fragment shader `%s`\n",
                    scene_conf_file_path, fragment_shader_def_line, fragment_shader_file_path);
            return;
        }

        if (!link_program(vert, frag, &program)) {
            fprintf(stderr, "ERROR: failed to link shader program\n");
            return;
        }

        glUseProgram(program);
        time_location = glGetUniformLocation(program, "time");
        resolution_location = glGetUniformLocation(program, "resolution");
    }
    // reload shader program end

    // reload texture begin
    {
        glDeleteTextures(1, &texture_id);

        int w, h;
        uint32_t *pixels = (uint32_t*) stbi_load(texture_file_path, &w, &h, NULL, 4);
        if (pixels == NULL) {
            fprintf(stderr, "%s:%zu: ERROR: could not load file %s: %s\n",
                    scene_conf_file_path, texture_def_line, texture_file_path, strerror(errno));
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
    }
    // reload texture end

    glClearColor(BACKGROUND_COLOR);
    program_failed = false;

    printf("Successfully reloaded scene\n");
    printf("Memory %zu/%zu bytes\n", hot_reload_memory.size, (size_t) REGION_CAPACITY);
    region_clean(&hot_reload_memory);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void) window;
    (void) scancode;
    (void) action;
    (void) mods;

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_F5) {
            reload_scene();
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
            "kidito",
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

    reload_scene();


    V4 mesh[TRIS_PER_CUBE][TRI_VERTICES] = {0};
    RGBA colors[TRIS_PER_CUBE][TRI_VERTICES] = {0};
    V2 uvs[TRIS_PER_CUBE][TRI_VERTICES] = {0};
    V4 normals[TRIS_PER_CUBE][TRI_VERTICES] = {0};

    generate_cube_mesh(mesh, colors, uvs, normals);

    {
        GLuint position_buffer_id;
        glGenBuffers(1, &position_buffer_id);
        glBindBuffer(GL_ARRAY_BUFFER, position_buffer_id);
        glBufferData(GL_ARRAY_BUFFER,
                     sizeof(mesh),
                     mesh,
                     GL_STATIC_DRAW);
        GLuint position_index = 0;
        glEnableVertexAttribArray(position_index);
        glVertexAttribPointer(position_index,
                              V4_COMPS,
                              GL_FLOAT,
                              GL_FALSE,
                              0,
                              NULL);
    }

    {
        GLuint uv_buffer_id;
        glGenBuffers(1, &uv_buffer_id);
        glBindBuffer(GL_ARRAY_BUFFER, uv_buffer_id);
        glBufferData(GL_ARRAY_BUFFER,
                     sizeof(uvs),
                     uvs,
                     GL_STATIC_DRAW);
        GLuint uv_index = 1;
        glEnableVertexAttribArray(uv_index);
        glVertexAttribPointer(uv_index,
                              V2_COMPS,
                              GL_FLOAT,
                              GL_FALSE,
                              0,
                              NULL);
    }

    {
        GLuint normal_buffer_id;
        glGenBuffers(1, &normal_buffer_id);
        glBindBuffer(GL_ARRAY_BUFFER, normal_buffer_id);
        glBufferData(GL_ARRAY_BUFFER,
                     sizeof(normals),
                     normals,
                     GL_STATIC_DRAW);
        GLuint normal_index = 2;
        glEnableVertexAttribArray(normal_index);
        glVertexAttribPointer(normal_index,
                              V4_COMPS,
                              GL_FLOAT,
                              GL_FALSE,
                              0,
                              NULL);
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
