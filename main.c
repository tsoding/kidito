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

#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

void panic_errno(const char *fmt, ...)
{
    fprintf(stderr, "ERROR: ");

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, ": %s\n", strerror(errno));

    exit(1);
}

char *slurp_file(const char *file_path)
{
#define SLURP_FILE_PANIC panic_errno("Could not read file `%s`", file_path)
    FILE *f = fopen(file_path, "r");
    if (f == NULL) SLURP_FILE_PANIC;
    if (fseek(f, 0, SEEK_END) < 0) SLURP_FILE_PANIC;

    long size = ftell(f);
    if (size < 0) SLURP_FILE_PANIC;

    char *buffer = malloc(size + 1);
    if (buffer == NULL) SLURP_FILE_PANIC;

    if (fseek(f, 0, SEEK_SET) < 0) SLURP_FILE_PANIC;

    fread(buffer, 1, size, f);
    if (ferror(f) < 0) SLURP_FILE_PANIC;

    buffer[size] = '\0';

    if (fclose(f) < 0) SLURP_FILE_PANIC;

    return buffer;
#undef SLURP_FILE_PANIC
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
    bool err = compile_shader_source(source, shader_type, shader);
    free(source);
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

GLuint program = 0;
GLuint failed_program = 0;

const char *failed_vert_source =
    "#version 130\n"
    "\n"
    "void main(void)\n"
    "{\n"
    "    int gray = gl_VertexID ^ (gl_VertexID >> 1);\n"
    "\n"
    "    gl_Position = vec4(\n"
    "        2 * (gray / 2) - 1,\n"
    "        2 * (gray % 2) - 1,\n"
    "        0.0,\n"
    "        1.0);\n"
    "};\n";

const char *failed_frag_source =
    "#version 130\n"
    "\n"
    "out vec4 color;\n"
    "\n"
    "void main(void) {\n"
    "    color = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "};\n";


void init_failed_program(void)
{
    GLuint vert = 0;
    if (!compile_shader_source(failed_vert_source, GL_VERTEX_SHADER, &vert)) {
        exit(1);
    }

    GLuint frag = 0;
    if (!compile_shader_source(failed_frag_source, GL_FRAGMENT_SHADER, &frag)) {
        exit(1);
    }

    if (!link_program(vert, frag, &failed_program)) {
        exit(1);
    }
}

void reload_shaders(void)
{
    glDeleteProgram(program);

    GLuint vert = 0;
    if (!compile_shader_file("./main.vert", GL_VERTEX_SHADER, &vert)) {
        glUseProgram(failed_program);
        return;
    }

    GLuint frag = 0;
    if (!compile_shader_file("./main.frag", GL_FRAGMENT_SHADER, &frag)) {
        glUseProgram(failed_program);
        return;
    }

    if (!link_program(vert, frag, &program)) {
        glUseProgram(failed_program);
        return;
    }

    glUseProgram(program);

    printf("Successfully Reload the Shaders\n");
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void) window;
    (void) scancode;
    (void) action;
    (void) mods;

    if (key == GLFW_KEY_F5 && action == GLFW_PRESS) {
        reload_shaders();
    }
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
    (void) window;
    glViewport(
        width / 2 - SCREEN_WIDTH / 2,
        height / 2 - SCREEN_HEIGHT / 2,
        SCREEN_WIDTH,
        SCREEN_HEIGHT);
}

GLuint buffer_from_mesh(Tri *mesh, size_t mesh_count)
{
    GLuint buffer_id;
    glGenBuffers(1, &buffer_id);
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(Tri) * mesh_count,
                 mesh,
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

    GLFWwindow * const window = glfwCreateWindow(
                                    SCREEN_WIDTH,
                                    SCREEN_HEIGHT,
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

#define TEXTURE_FILE_PATH "pog.png"
    int w, h;
    uint32_t *pixels = (uint32_t*) stbi_load(TEXTURE_FILE_PATH, &w, &h, NULL, 4);
    if (pixels == NULL) {
        fprintf(stderr, "ERROR: could not load file `"TEXTURE_FILE_PATH"`: %s\n",
                strerror(errno));
        exit(1);
    }

    GLuint texture_id = 0;
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


    init_failed_program();
    reload_shaders();

    Tri cube_mesh[TRIS_PER_CUBE] = {0};
    generate_cube_mesh(cube_mesh);
    buffer_from_mesh(cube_mesh, TRIS_PER_CUBE);

    GLuint u_matrix = glGetUniformLocation(program, "matrix");

    {
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

    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, window_size_callback);

    float angle = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        angle += 0.01f;
        Mat4x4 matrix = rotation_mat4x4_y(angle);
        glUniformMatrix4fv(u_matrix, 1, GL_TRUE, (void*) &matrix);

        glClear(GL_COLOR_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLES, 0, TRIS_PER_CUBE * TRI_VERTICES);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}
