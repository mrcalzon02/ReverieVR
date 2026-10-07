#include "reverie_native_sdk.h"
#include "reverie_native_gl_state.h"
#include "reverie_native_gl_utils.h"
#include "reverie_native_math.h"
#include "gentexture.hpp"

#include <GLES2/gl2.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <new>
#include <string>
#include <vector>

namespace {

constexpr int kTextureSize = 256;

struct ModuleState {
    const ReverieNativeHostV1 *host = nullptr;

    GLuint program = 0;
    GLuint vbo = 0;
    GLuint texture = 0;

    GLint position_location = -1;
    GLint uv_location = -1;
    GLint matrix_location = -1;
    GLint tint_location = -1;
    GLint texture_location = -1;

    bool primary_was_down = false;
    bool alternate_tint = false;

    std::vector<uint8_t> rgba;
};

void Log(
    const ModuleState *state,
    int32_t level,
    const char *message
) {
    if (state != nullptr
        && ReverieNativeHostSupportsLogV1(
            state->host
        )) {
        state->host->log(
            level,
            "ReverieTestChamber",
            message
        );
    }
}

GLuint BuildProgram() {
    static const char *kVertexShader =
        "uniform mat4 u_ViewProjection;\n"
        "attribute vec3 a_Position;\n"
        "attribute vec2 a_Uv;\n"
        "varying vec2 v_Uv;\n"
        "void main() {\n"
        "  gl_Position = u_ViewProjection * vec4(a_Position, 1.0);\n"
        "  v_Uv = a_Uv;\n"
        "}\n";

    static const char *kFragmentShader =
        "precision mediump float;\n"
        "uniform sampler2D u_Texture;\n"
        "uniform vec3 u_Tint;\n"
        "varying vec2 v_Uv;\n"
        "void main() {\n"
        "  vec4 texel = texture2D(u_Texture, v_Uv);\n"
        "  gl_FragColor = vec4(texel.rgb * u_Tint, 1.0);\n"
        "}\n";

    GLuint vertex =
        ReverieNativeCompileShader(
            GL_VERTEX_SHADER,
            kVertexShader
        );
    if (vertex == 0) {
        return 0;
    }

    GLuint fragment =
        ReverieNativeCompileShader(
            GL_FRAGMENT_SHADER,
            kFragmentShader
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
    glBindAttribLocation(
        program,
        0,
        "a_Position"
    );
    glBindAttribLocation(
        program,
        1,
        "a_Uv"
    );
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

void GenerateProceduralTexture(
    ModuleState *state
) {
    InitTexgen();

    GenTexture gradient(kTextureSize, 1);
    for (int index = 0; index < kTextureSize; ++index) {
        float t =
            static_cast<float>(index)
            / static_cast<float>(
                kTextureSize - 1
            );

        uint8_t r =
            static_cast<uint8_t>(
                24.0f + t * 78.0f
            );
        uint8_t g =
            static_cast<uint8_t>(
                64.0f + t * 136.0f
            );
        uint8_t b =
            static_cast<uint8_t>(
                78.0f + t * 106.0f
            );

        gradient.Data[index].Init(
            r,
            g,
            b,
            255
        );
    }

    GenTexture generated(
        kTextureSize,
        kTextureSize
    );
    generated.Noise(
        gradient,
        4,
        4,
        5,
        0.57f,
        0x51A7,
        GenTexture::NoiseNormalize
            | GenTexture::NoiseBandlimit
    );

    state->rgba.resize(
        kTextureSize
            * kTextureSize
            * 4
    );

    for (int index = 0;
         index < generated.NPixels;
         ++index) {
        const Pixel &pixel =
            generated.Data[index];
        size_t offset =
            static_cast<size_t>(index)
            * 4u;

        state->rgba[offset + 0] =
            static_cast<uint8_t>(
                pixel.r >> 8
            );
        state->rgba[offset + 1] =
            static_cast<uint8_t>(
                pixel.g >> 8
            );
        state->rgba[offset + 2] =
            static_cast<uint8_t>(
                pixel.b >> 8
            );
        state->rgba[offset + 3] = 255u;
    }
}

static const float kRoomVertices[] = {
    // Floor
    -3.0f,-1.5f,-3.0f, 0.0f,0.0f,
     3.0f,-1.5f,-3.0f, 4.0f,0.0f,
     3.0f,-1.5f, 3.0f, 4.0f,4.0f,
    -3.0f,-1.5f,-3.0f, 0.0f,0.0f,
     3.0f,-1.5f, 3.0f, 4.0f,4.0f,
    -3.0f,-1.5f, 3.0f, 0.0f,4.0f,

    // Ceiling
    -3.0f, 1.5f, 3.0f, 0.0f,0.0f,
     3.0f, 1.5f, 3.0f, 4.0f,0.0f,
     3.0f, 1.5f,-3.0f, 4.0f,4.0f,
    -3.0f, 1.5f, 3.0f, 0.0f,0.0f,
     3.0f, 1.5f,-3.0f, 4.0f,4.0f,
    -3.0f, 1.5f,-3.0f, 0.0f,4.0f,

    // Back wall
    -3.0f,-1.5f,-3.0f, 0.0f,0.0f,
    -3.0f, 1.5f,-3.0f, 0.0f,2.0f,
     3.0f, 1.5f,-3.0f, 4.0f,2.0f,
    -3.0f,-1.5f,-3.0f, 0.0f,0.0f,
     3.0f, 1.5f,-3.0f, 4.0f,2.0f,
     3.0f,-1.5f,-3.0f, 4.0f,0.0f,

    // Front wall
     3.0f,-1.5f, 3.0f, 0.0f,0.0f,
     3.0f, 1.5f, 3.0f, 0.0f,2.0f,
    -3.0f, 1.5f, 3.0f, 4.0f,2.0f,
     3.0f,-1.5f, 3.0f, 0.0f,0.0f,
    -3.0f, 1.5f, 3.0f, 4.0f,2.0f,
    -3.0f,-1.5f, 3.0f, 4.0f,0.0f,

    // Left wall
    -3.0f,-1.5f, 3.0f, 0.0f,0.0f,
    -3.0f, 1.5f, 3.0f, 0.0f,2.0f,
    -3.0f, 1.5f,-3.0f, 4.0f,2.0f,
    -3.0f,-1.5f, 3.0f, 0.0f,0.0f,
    -3.0f, 1.5f,-3.0f, 4.0f,2.0f,
    -3.0f,-1.5f,-3.0f, 4.0f,0.0f,

    // Right wall
     3.0f,-1.5f,-3.0f, 0.0f,0.0f,
     3.0f, 1.5f,-3.0f, 0.0f,2.0f,
     3.0f, 1.5f, 3.0f, 4.0f,2.0f,
     3.0f,-1.5f,-3.0f, 0.0f,0.0f,
     3.0f, 1.5f, 3.0f, 4.0f,2.0f,
     3.0f,-1.5f, 3.0f, 4.0f,0.0f
};

void DestroyGl(ModuleState *state) {
    if (state->texture != 0) {
        glDeleteTextures(
            1,
            &state->texture
        );
        state->texture = 0;
    }

    if (state->vbo != 0) {
        glDeleteBuffers(
            1,
            &state->vbo
        );
        state->vbo = 0;
    }

    if (state->program != 0) {
        glDeleteProgram(state->program);
        state->program = 0;
    }

    state->position_location = -1;
    state->uv_location = -1;
    state->matrix_location = -1;
    state->tint_location = -1;
    state->texture_location = -1;
}

void AbandonGl(ModuleState *state) {
    state->program = 0;
    state->vbo = 0;
    state->texture = 0;
    state->position_location = -1;
    state->uv_location = -1;
    state->matrix_location = -1;
    state->tint_location = -1;
    state->texture_location = -1;
}

int32_t InitializeGl(ModuleState *state) {
    ReverieNativeGlStateV1 init_gl_state = {};
    ReverieNativeGlStateCaptureV1(
        &init_gl_state
    );
    auto finish = [&init_gl_state](
        int32_t result
    ) -> int32_t {
        ReverieNativeGlStateRestoreV1(
            &init_gl_state
        );
        return result;
    };

    AbandonGl(state);

    state->program = BuildProgram();
    if (state->program == 0) {
        Log(
            state,
            REVERIE_NATIVE_LOG_ERROR,
            "Shader program creation failed."
        );
        return finish(0);
    }

    state->position_location =
        glGetAttribLocation(
            state->program,
            "a_Position"
        );
    state->uv_location =
        glGetAttribLocation(
            state->program,
            "a_Uv"
        );
    state->matrix_location =
        glGetUniformLocation(
            state->program,
            "u_ViewProjection"
        );
    state->tint_location =
        glGetUniformLocation(
            state->program,
            "u_Tint"
        );
    state->texture_location =
        glGetUniformLocation(
            state->program,
            "u_Texture"
        );

    if (state->position_location < 0
        || state->uv_location < 0
        || state->matrix_location < 0
        || state->tint_location < 0
        || state->texture_location < 0) {
        Log(
            state,
            REVERIE_NATIVE_LOG_ERROR,
            "Required shader locations are unavailable."
        );
        DestroyGl(state);
        return finish(0);
    }

    glGenBuffers(1, &state->vbo);
    if (state->vbo == 0) {
        Log(
            state,
            REVERIE_NATIVE_LOG_ERROR,
            "Room vertex buffer creation failed."
        );
        DestroyGl(state);
        return finish(0);
    }

    glBindBuffer(
        GL_ARRAY_BUFFER,
        state->vbo
    );
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(kRoomVertices),
        kRoomVertices,
        GL_STATIC_DRAW
    );

    glGenTextures(1, &state->texture);
    if (state->texture == 0) {
        Log(
            state,
            REVERIE_NATIVE_LOG_ERROR,
            "Procedural texture creation failed."
        );
        DestroyGl(state);
        return finish(0);
    }

    glBindTexture(
        GL_TEXTURE_2D,
        state->texture
    );
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR
    );
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_REPEAT
    );
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_REPEAT
    );
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        kTextureSize,
        kTextureSize,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        state->rgba.data()
    );

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        Log(
            state,
            REVERIE_NATIVE_LOG_ERROR,
            "OpenGL ES initialization reported an error."
        );
        DestroyGl(state);
        return finish(0);
    }

    Log(
        state,
        REVERIE_NATIVE_LOG_INFO,
        "GL resources created from OpenKTG procedural texture."
    );
    return finish(1);
}

void *Create(
    const ReverieNativeHostV1 *host
) {
    if (!ReverieNativeHostSupportsLogV1(
            host
        )) {
        return nullptr;
    }

    ModuleState *state =
        new (std::nothrow) ModuleState();
    if (state == nullptr) {
        return nullptr;
    }

    state->host = host;

    try {
        GenerateProceduralTexture(state);
    } catch (...) {
        Log(
            state,
            REVERIE_NATIVE_LOG_ERROR,
            "OpenKTG texture generation threw an exception."
        );
        delete state;
        return nullptr;
    }

    Log(
        state,
        REVERIE_NATIVE_LOG_INFO,
        "Module created; procedural texture recipe expanded on CPU."
    );
    return state;
}

void Destroy(void *instance) {
    ModuleState *state =
        static_cast<ModuleState *>(instance);
    if (state == nullptr) {
        return;
    }

    if (state->program != 0
        || state->vbo != 0
        || state->texture != 0) {
        Log(
            state,
            REVERIE_NATIVE_LOG_WARN,
            "Module destroyed after GL context release was skipped."
        );
        AbandonGl(state);
    }

    Log(
        state,
        REVERIE_NATIVE_LOG_INFO,
        "Module destroyed."
    );
    delete state;
}

int32_t OnGlContextCreated(void *instance) {
    ModuleState *state =
        static_cast<ModuleState *>(instance);
    if (state == nullptr) {
        return 0;
    }
    return InitializeGl(state);
}

void ReleaseGlContext(void *instance) {
    ModuleState *state =
        static_cast<ModuleState *>(instance);
    if (state == nullptr) {
        return;
    }

    DestroyGl(state);
    Log(
        state,
        REVERIE_NATIVE_LOG_INFO,
        "GL resources released."
    );
}

void Resume(void *instance) {
    ModuleState *state =
        static_cast<ModuleState *>(instance);
    Log(
        state,
        REVERIE_NATIVE_LOG_INFO,
        "Module resumed."
    );
}

void Pause(void *instance) {
    ModuleState *state =
        static_cast<ModuleState *>(instance);
    Log(
        state,
        REVERIE_NATIVE_LOG_INFO,
        "Module paused."
    );
}

void Update(
    void *instance,
    const ReverieNativeInputV1 *input
) {
    ModuleState *state =
        static_cast<ModuleState *>(instance);
    if (state == nullptr
        || !ReverieNativeInputHasBaseV1(
            input
        )) {
        return;
    }

    bool primary =
        input->primary_down != 0u;
    if (primary
        && !state->primary_was_down) {
        state->alternate_tint =
            !state->alternate_tint;
        Log(
            state,
            REVERIE_NATIVE_LOG_DEBUG,
            "Primary action toggled chamber tint."
        );
    }
    state->primary_was_down = primary;
}

int32_t RenderEye(
    void *instance,
    const ReverieNativeEyeV1 *eye
) {
    ModuleState *state =
        static_cast<ModuleState *>(instance);
    if (state == nullptr
        || !ReverieNativeEyeHasMatricesV1(
            eye
        )
        || state->program == 0
        || state->vbo == 0
        || state->texture == 0) {
        return 0;
    }

    ReverieNativeGlStateV1 gl_state = {};
    ReverieNativeGlAttribStateV1 position_state = {};
    ReverieNativeGlAttribStateV1 uv_state = {};

    ReverieNativeGlStateCaptureV1(
        &gl_state
    );
    ReverieNativeGlAttribCaptureV1(
        &position_state,
        static_cast<GLuint>(
            state->position_location
        )
    );
    ReverieNativeGlAttribCaptureV1(
        &uv_state,
        static_cast<GLuint>(
            state->uv_location
        )
    );

    float view_projection[16];

    // Shell-owned camera movement is already shared across both eyes.
    ReverieNativeMat4Multiply(
        view_projection,
        eye->projection,
        eye->view
    );

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    glUseProgram(state->program);
    glBindBuffer(
        GL_ARRAY_BUFFER,
        state->vbo
    );

    glEnableVertexAttribArray(
        static_cast<GLuint>(
            state->position_location
        )
    );
    glVertexAttribPointer(
        static_cast<GLuint>(
            state->position_location
        ),
        3,
        GL_FLOAT,
        GL_FALSE,
        5 * sizeof(float),
        reinterpret_cast<const void *>(0)
    );

    glEnableVertexAttribArray(
        static_cast<GLuint>(
            state->uv_location
        )
    );
    glVertexAttribPointer(
        static_cast<GLuint>(
            state->uv_location
        ),
        2,
        GL_FLOAT,
        GL_FALSE,
        5 * sizeof(float),
        reinterpret_cast<const void *>(
            3 * sizeof(float)
        )
    );

    glUniformMatrix4fv(
        state->matrix_location,
        1,
        GL_FALSE,
        view_projection
    );

    if (state->alternate_tint) {
        glUniform3f(
            state->tint_location,
            1.0f,
            0.72f,
            0.55f
        );
    } else {
        glUniform3f(
            state->tint_location,
            0.82f,
            1.0f,
            0.92f
        );
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(
        GL_TEXTURE_2D,
        state->texture
    );
    glUniform1i(
        state->texture_location,
        0
    );

    glDrawArrays(
        GL_TRIANGLES,
        0,
        36
    );

    ReverieNativeGlAttribRestoreV1(
        &uv_state
    );
    ReverieNativeGlAttribRestoreV1(
        &position_state
    );
    ReverieNativeGlStateRestoreV1(
        &gl_state
    );

    return glGetError() == GL_NO_ERROR
        ? 1
        : 0;
}

const ReverieNativeModuleCapabilitiesV1 kCapabilities = {
    sizeof(ReverieNativeModuleCapabilitiesV1),
    REVERIE_NATIVE_CAPABILITY_SHELL_LOCOMOTION,
    1.55f,
    1.55f
};

const ReverieNativeModuleApiV1 kApi = {
    sizeof(ReverieNativeModuleApiV1),
    REVERIE_NATIVE_MODULE_ABI_VERSION,
    {
        sizeof(
            ReverieNativeModuleDescriptorV1
        ),
        REVERIE_NATIVE_MODULE_ABI_VERSION,
        "procedural-test-chamber",
        "Procedural Test Chamber",
        2u,
        0u
    },
    Create,
    Destroy,
    OnGlContextCreated,
    ReleaseGlContext,
    Resume,
    Pause,
    Update,
    RenderEye,
    &kCapabilities
};

}  // namespace

extern "C"
__attribute__((visibility("default")))
const ReverieNativeModuleApiV1 *
reverie_native_module_entry_v1(void) {
    return &kApi;
}
