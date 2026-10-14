#ifndef REVERIE_NATIVE_GL_UTILS_H
#define REVERIE_NATIVE_GL_UTILS_H

#include <stddef.h>
#include <stdio.h>
#include <GLES2/gl2.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ReverieNativeGlAttributeBindingV1 {
    GLuint index;
    const char *name;
} ReverieNativeGlAttributeBindingV1;

/*
 * Optional bounded failure messages for GLES shader compilation and program
 * linking. The original two public helpers remain API-compatible wrappers.
 * Only a failed shader/program asks GLES for an info log; normal draws and
 * successful GL initialization do no extra work or allocations.
 */
static inline void ReverieNativeGlDiagnosticWriteV1(
    char *output, size_t capacity, const char *phase, const char *detail
) {
    if (output == NULL || capacity == 0u) return;
    snprintf(output, capacity, "%s: %s",
        phase != NULL ? phase : "GLES2",
        detail != NULL && detail[0] != '\0' ? detail : "no driver detail");
}

static inline GLuint ReverieNativeCompileShaderDiagnosticV1(
    GLenum type, const char *source, char *diagnostic, size_t capacity
) {
    if (diagnostic != NULL && capacity != 0u) diagnostic[0] = '\0';
    const char *stage = type == GL_VERTEX_SHADER ? "vertex shader"
        : type == GL_FRAGMENT_SHADER ? "fragment shader" : "shader";
    if (source == NULL) {
        ReverieNativeGlDiagnosticWriteV1(diagnostic, capacity, stage, "null source");
        return 0;
    }

    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        ReverieNativeGlDiagnosticWriteV1(diagnostic, capacity,
            stage, "glCreateShader returned zero");
        return 0;
    }
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        if (diagnostic != NULL && capacity != 0u) {
            GLchar info[256] = {};
            glGetShaderInfoLog(shader, sizeof(info) - 1, NULL, info);
            ReverieNativeGlDiagnosticWriteV1(diagnostic, capacity, stage, info);
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static inline GLuint ReverieNativeCompileShader(
    GLenum type, const char *source
) {
    return ReverieNativeCompileShaderDiagnosticV1(type, source, NULL, 0u);
}

static inline GLuint ReverieNativeBuildProgramDiagnosticV1(
    const char *vertex_source, const char *fragment_source,
    const ReverieNativeGlAttributeBindingV1 *bindings,
    size_t binding_count, char *diagnostic, size_t capacity
) {
    if (diagnostic != NULL && capacity != 0u) diagnostic[0] = '\0';
    if (vertex_source == NULL || fragment_source == NULL
        || (binding_count > 0u && bindings == NULL)) {
        ReverieNativeGlDiagnosticWriteV1(diagnostic, capacity,
            "shader program", "invalid source or bindings");
        return 0;
    }
    for (size_t index = 0u; index < binding_count; ++index) {
        if (bindings[index].name == NULL) {
            ReverieNativeGlDiagnosticWriteV1(diagnostic, capacity,
                "shader program", "null attribute binding");
            return 0;
        }
    }

    GLuint vertex = ReverieNativeCompileShaderDiagnosticV1(
        GL_VERTEX_SHADER, vertex_source, diagnostic, capacity);
    if (vertex == 0) return 0;
    GLuint fragment = ReverieNativeCompileShaderDiagnosticV1(
        GL_FRAGMENT_SHADER, fragment_source, diagnostic, capacity);
    if (fragment == 0) {
        glDeleteShader(vertex);
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program == 0) {
        ReverieNativeGlDiagnosticWriteV1(diagnostic, capacity,
            "shader program", "glCreateProgram returned zero");
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return 0;
    }

    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    for (size_t index = 0u; index < binding_count; ++index) {
        glBindAttribLocation(program, bindings[index].index,
            bindings[index].name);
    }
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        if (diagnostic != NULL && capacity != 0u) {
            GLchar info[256] = {};
            glGetProgramInfoLog(program, sizeof(info) - 1, NULL, info);
            ReverieNativeGlDiagnosticWriteV1(
                diagnostic, capacity, "shader link", info);
        }
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

static inline GLuint ReverieNativeBuildProgram(
    const char *vertex_source, const char *fragment_source,
    const ReverieNativeGlAttributeBindingV1 *bindings,
    size_t binding_count
) {
    return ReverieNativeBuildProgramDiagnosticV1(
        vertex_source, fragment_source, bindings, binding_count,
        NULL, 0u);
}

#ifdef __cplusplus
}
#endif

#endif
