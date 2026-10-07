#ifndef REVERIE_NATIVE_GL_UTILS_H
#define REVERIE_NATIVE_GL_UTILS_H

#include <stddef.h>
#include <GLES2/gl2.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif
