#ifndef REVERIE_NATIVE_GL_UTILS_H
#define REVERIE_NATIVE_GL_UTILS_H

#include <stddef.h>
#include <GLES2/gl2.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ReverieNativeGlAttributeBindingV1 {
    GLuint index;
    const char *name;
} ReverieNativeGlAttributeBindingV1;

/*
 * Compile one GLES shader and fail closed. Program-specific attribute binding,
 * link policy and diagnostic wording remain with the module.
 */
static inline GLuint ReverieNativeCompileShader(
    GLenum type,
    const char *source
) {
    if (source == NULL) {
        return 0;
    }

    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        return 0;
    }

    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint status = GL_FALSE;
    glGetShaderiv(
        shader,
        GL_COMPILE_STATUS,
        &status
    );

    if (status != GL_TRUE) {
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static inline GLuint ReverieNativeBuildProgram(
    const char *vertex_source,
    const char *fragment_source,
    const ReverieNativeGlAttributeBindingV1 *bindings,
    size_t binding_count
) {
    if (vertex_source == NULL
        || fragment_source == NULL
        || (binding_count > 0u
            && bindings == NULL)) {
        return 0;
    }

    for (size_t index = 0u;
         index < binding_count;
         ++index) {
        if (bindings[index].name == NULL) {
            return 0;
        }
    }

    GLuint vertex =
        ReverieNativeCompileShader(
            GL_VERTEX_SHADER,
            vertex_source
        );
    if (vertex == 0) {
        return 0;
    }

    GLuint fragment =
        ReverieNativeCompileShader(
            GL_FRAGMENT_SHADER,
            fragment_source
        );
    if (fragment == 0) {
        glDeleteShader(vertex);
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program == 0) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return 0;
    }

    glAttachShader(program, vertex);
    glAttachShader(program, fragment);

    for (size_t index = 0u;
         index < binding_count;
         ++index) {
        glBindAttribLocation(
            program,
            bindings[index].index,
            bindings[index].name
        );
    }

    glLinkProgram(program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint linked = GL_FALSE;
    glGetProgramiv(
        program,
        GL_LINK_STATUS,
        &linked
    );
    if (linked != GL_TRUE) {
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

#ifdef __cplusplus
}
#endif

#endif
