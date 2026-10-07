#ifndef REVERIE_NATIVE_GL_STATE_H
#define REVERIE_NATIVE_GL_STATE_H

#include <GLES2/gl2.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Focused GL-state guard for the state first-party native modules currently
 * mutate. Extend this component when modules begin changing additional shared
 * host state; do not silently assume it covers framebuffer/viewport/scissor or
 * blend/depth/cull parameters that the module never captured.
 */
typedef struct ReverieNativeGlStateV1 {
    GLint program;
    GLint array_buffer;
    GLint active_texture;
    GLint texture0_binding_2d;
    GLboolean depth_test_enabled;
    GLboolean blend_enabled;
    GLboolean cull_face_enabled;
} ReverieNativeGlStateV1;

typedef struct ReverieNativeGlAttribStateV1 {
    GLuint index;
    GLint enabled;
    GLint size;
    GLint stride;
    GLint type;
    GLint normalized;
    GLint buffer_binding;
    void *pointer;
    GLfloat current_value[4];
} ReverieNativeGlAttribStateV1;

static inline void ReverieNativeGlStateCaptureV1(
    ReverieNativeGlStateV1 *state
) {
    if (state == NULL) {
        return;
    }

    glGetIntegerv(
        GL_CURRENT_PROGRAM,
        &state->program
    );
    glGetIntegerv(
        GL_ARRAY_BUFFER_BINDING,
        &state->array_buffer
    );
    glGetIntegerv(
        GL_ACTIVE_TEXTURE,
        &state->active_texture
    );

    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(
        GL_TEXTURE_BINDING_2D,
        &state->texture0_binding_2d
    );
    glActiveTexture(
        (GLenum)state->active_texture
    );

    state->depth_test_enabled =
        glIsEnabled(GL_DEPTH_TEST);
    state->blend_enabled =
        glIsEnabled(GL_BLEND);
    state->cull_face_enabled =
        glIsEnabled(GL_CULL_FACE);
}

static inline void ReverieNativeGlStateRestoreV1(
    const ReverieNativeGlStateV1 *state
) {
    if (state == NULL) {
        return;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(
        GL_TEXTURE_2D,
        (GLuint)state->texture0_binding_2d
    );
    glActiveTexture(
        (GLenum)state->active_texture
    );

    glBindBuffer(
        GL_ARRAY_BUFFER,
        (GLuint)state->array_buffer
    );
    glUseProgram(
        (GLuint)state->program
    );

    if (state->depth_test_enabled == GL_TRUE) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    if (state->blend_enabled == GL_TRUE) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }

    if (state->cull_face_enabled == GL_TRUE) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }
}

static inline void ReverieNativeGlAttribCaptureV1(
    ReverieNativeGlAttribStateV1 *state,
    GLuint index
) {
    if (state == NULL) {
        return;
    }

    state->index = index;
    glGetVertexAttribiv(
        index,
        GL_VERTEX_ATTRIB_ARRAY_ENABLED,
        &state->enabled
    );
    glGetVertexAttribiv(
        index,
        GL_VERTEX_ATTRIB_ARRAY_SIZE,
        &state->size
    );
    glGetVertexAttribiv(
        index,
        GL_VERTEX_ATTRIB_ARRAY_STRIDE,
        &state->stride
    );
    glGetVertexAttribiv(
        index,
        GL_VERTEX_ATTRIB_ARRAY_TYPE,
        &state->type
    );
    glGetVertexAttribiv(
        index,
        GL_VERTEX_ATTRIB_ARRAY_NORMALIZED,
        &state->normalized
    );
    glGetVertexAttribiv(
        index,
        GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,
        &state->buffer_binding
    );
    glGetVertexAttribPointerv(
        index,
        GL_VERTEX_ATTRIB_ARRAY_POINTER,
        &state->pointer
    );
    glGetVertexAttribfv(
        index,
        GL_CURRENT_VERTEX_ATTRIB,
        state->current_value
    );
}

static inline void ReverieNativeGlAttribRestoreV1(
    const ReverieNativeGlAttribStateV1 *state
) {
    if (state == NULL) {
        return;
    }

    glVertexAttrib4fv(
        state->index,
        state->current_value
    );
    glBindBuffer(
        GL_ARRAY_BUFFER,
        (GLuint)state->buffer_binding
    );
    glVertexAttribPointer(
        state->index,
        state->size,
        (GLenum)state->type,
        state->normalized != 0
            ? GL_TRUE
            : GL_FALSE,
        state->stride,
        state->pointer
    );

    if (state->enabled != 0) {
        glEnableVertexAttribArray(
            state->index
        );
    } else {
        glDisableVertexAttribArray(
            state->index
        );
    }
}

#ifdef __cplusplus
}
#endif

#endif
