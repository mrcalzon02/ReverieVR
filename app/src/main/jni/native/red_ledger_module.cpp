#include "reverie_native_module.h"
#include "red_ledger_simulation.h"

#include <GLES2/gl2.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <new>

namespace {

using reverie::redledger::EventType;
using reverie::redledger::Simulation;

constexpr float kMoveSpeed = 0.35f;
constexpr float kMoveLimit = 0.35f;
constexpr float kMaximumWorkReach = 3.5f;
constexpr char kSaveSlot[] = "state-v1.bin";

enum class WorkTarget : uint8_t {
    None = 0,
    Tap,
    WashStation,
    Ledger,
    BeerOrder,
    CupOrder,
    ProtectionEnvelope
};

struct TargetBounds {
    WorkTarget target;
    float center[3];
    float half_extent[3];
};

static const TargetBounds kWorkTargets[] = {
    {WorkTarget::Tap, {-0.85f, -0.02f, -0.98f}, {0.28f, 0.35f, 0.28f}},
    {WorkTarget::WashStation, {-0.28f, -0.18f, -1.18f}, {0.55f, 0.22f, 0.30f}},
    {WorkTarget::Ledger, {0.85f, -0.20f, -0.90f}, {0.30f, 0.12f, 0.24f}},
    {WorkTarget::BeerOrder, {1.20f, -0.18f, -0.72f}, {0.24f, 0.14f, 0.15f}},
    {WorkTarget::CupOrder, {1.68f, -0.18f, -0.72f}, {0.24f, 0.14f, 0.15f}},
    {WorkTarget::ProtectionEnvelope, {-1.35f, -0.18f, -0.72f}, {0.26f, 0.13f, 0.15f}}
};

struct ModuleState {
    const ReverieNativeHostV1 *host = nullptr;
    Simulation simulation;

    GLuint program = 0;
    GLuint cube_vbo = 0;
    GLint position_location = -1;
    GLint matrix_location = -1;
    GLint color_location = -1;
    GLint flicker_location = -1;

    float player_x = 0.0f;
    float player_z = 0.0f;
    float elapsed_seconds = 0.0f;
    bool primary_was_down = false;
    WorkTarget hovered_target = WorkTarget::None;
    float hovered_distance = 0.0f;
};

void Log(const ModuleState *state, int32_t level, const char *message) {
    if (state != nullptr && state->host != nullptr && state->host->log != nullptr) {
        state->host->log(level, "ReverieRedLedger", message);
    }
}

const char *WorkTargetName(WorkTarget target) {
    switch (target) {
        case WorkTarget::Tap: return "tap";
        case WorkTarget::WashStation: return "wash station";
        case WorkTarget::Ledger: return "ledger";
        case WorkTarget::BeerOrder: return "beer order card";
        case WorkTarget::CupOrder: return "cup order card";
        case WorkTarget::ProtectionEnvelope: return "protection envelope";
        case WorkTarget::None:
        default: return "nothing";
    }
}

bool IsTargetAvailable(const ModuleState *state, WorkTarget target) {
    if (state == nullptr) {
        return false;
    }
    if (target == WorkTarget::ProtectionEnvelope) {
        return state->simulation.current_event() == EventType::ProtectionDemand
            && !state->simulation.protection_paid();
    }
    return target != WorkTarget::None;
}

float RayAabbDistance(
    const float *origin,
    const float *direction,
    const TargetBounds &bounds
) {
    float near_distance = 0.0f;
    float far_distance = kMaximumWorkReach;

    for (int axis = 0; axis < 3; ++axis) {
        const float minimum = bounds.center[axis] - bounds.half_extent[axis];
        const float maximum = bounds.center[axis] + bounds.half_extent[axis];
        const float component = direction[axis];

        if (std::abs(component) < 0.000001f) {
            if (origin[axis] < minimum || origin[axis] > maximum) {
                return -1.0f;
            }
            continue;
        }

        float first = (minimum - origin[axis]) / component;
        float second = (maximum - origin[axis]) / component;
        if (first > second) {
            std::swap(first, second);
        }

        near_distance = std::max(near_distance, first);
        far_distance = std::min(far_distance, second);
        if (near_distance > far_distance) {
            return -1.0f;
        }
    }

    if (far_distance < 0.0f || near_distance > kMaximumWorkReach) {
        return -1.0f;
    }
    return std::max(0.0f, near_distance);
}

WorkTarget FindWorkTarget(
    const ModuleState *state,
    const ReverieNativeInputV1 *input,
    float *out_distance
) {
    if (out_distance != nullptr) {
        *out_distance = 0.0f;
    }
    if (state == nullptr
        || input == nullptr
        || input->pointer_kind == REVERIE_NATIVE_POINTER_NONE) {
        return WorkTarget::None;
    }

    WorkTarget best = WorkTarget::None;
    float best_distance = kMaximumWorkReach + 1.0f;

    for (const TargetBounds &bounds : kWorkTargets) {
        if (!IsTargetAvailable(state, bounds.target)) {
            continue;
        }

        const float distance = RayAabbDistance(
            input->pointer_origin,
            input->pointer_direction,
            bounds
        );
        if (distance >= 0.0f && distance < best_distance) {
            best = bounds.target;
            best_distance = distance;
        }
    }

    if (best != WorkTarget::None && out_distance != nullptr) {
        *out_distance = best_distance;
    }
    return best;
}

bool SaveState(ModuleState *state) {
    if (state == nullptr
        || state->host == nullptr
        || state->host->write_save == nullptr) {
        return false;
    }

    uint8_t bytes[Simulation::kSerializedSize] = {};
    size_t size = 0u;
    if (!state->simulation.Serialize(bytes, sizeof(bytes), &size)) {
        Log(state, REVERIE_NATIVE_LOG_ERROR, "Could not serialize Red Ledger state.");
        return false;
    }

    const int32_t result = state->host->write_save(
        kSaveSlot,
        bytes,
        static_cast<uint32_t>(size)
    );
    if (result != REVERIE_NATIVE_SAVE_OK) {
        Log(state, REVERIE_NATIVE_LOG_WARN, "Host rejected Red Ledger save write.");
        return false;
    }
    return true;
}

void LoadState(ModuleState *state) {
    if (state == nullptr
        || state->host == nullptr
        || state->host->read_save == nullptr) {
        return;
    }

    uint8_t bytes[Simulation::kSerializedSize] = {};
    uint32_t size = 0u;
    const int32_t result = state->host->read_save(
        kSaveSlot,
        bytes,
        static_cast<uint32_t>(sizeof(bytes)),
        &size
    );

    if (result == REVERIE_NATIVE_SAVE_NOT_FOUND) {
        Log(state, REVERIE_NATIVE_LOG_INFO, "No Red Ledger save exists; using the opening state.");
        return;
    }

    if (result != REVERIE_NATIVE_SAVE_OK
        || size != sizeof(bytes)
        || !state->simulation.Deserialize(bytes, size)) {
        Log(state, REVERIE_NATIVE_LOG_WARN, "Red Ledger save is invalid; preserving a fresh opening state.");
        return;
    }

    Log(state, REVERIE_NATIVE_LOG_INFO, "Red Ledger save restored.");
}

bool ActivateWorkTarget(ModuleState *state, WorkTarget target) {
    if (state == nullptr || !IsTargetAvailable(state, target)) {
        return false;
    }

    bool changed = false;
    switch (target) {
        case WorkTarget::Tap:
            changed = state->simulation.ServeNextPatron();
            break;
        case WorkTarget::WashStation:
            changed = state->simulation.WashOneCup();
            break;
        case WorkTarget::BeerOrder:
            changed = state->simulation.BuySupply(
                reverie::redledger::SupplierItem::BeerCrate
            );
            break;
        case WorkTarget::CupOrder:
            changed = state->simulation.BuySupply(
                reverie::redledger::SupplierItem::CupSet
            );
            break;
        case WorkTarget::ProtectionEnvelope:
            changed = state->simulation.PayProtection();
            break;
        case WorkTarget::Ledger:
            if (state->simulation.patrons_remaining() == 0) {
                state->simulation.CloseDay();
                changed = true;
            }
            break;
        case WorkTarget::None:
        default:
            break;
    }

    if (!changed) {
        Log(state, REVERIE_NATIVE_LOG_WARN, "Targeted bar interaction could not complete.");
        return false;
    }

    Log(state, REVERIE_NATIVE_LOG_DEBUG, WorkTargetName(target));
    SaveState(state);
    return true;
}

GLuint CompileShader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        return 0;
    }
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint BuildProgram() {
    static const char *kVertexShader =
        "uniform mat4 u_Mvp;\n"
        "attribute vec3 a_Position;\n"
        "void main() {\n"
        "  gl_Position = u_Mvp * vec4(a_Position, 1.0);\n"
        "}\n";

    static const char *kFragmentShader =
        "precision mediump float;\n"
        "uniform vec3 u_Color;\n"
        "uniform float u_Flicker;\n"
        "void main() {\n"
        "  gl_FragColor = vec4(u_Color * u_Flicker, 1.0);\n"
        "}\n";

    GLuint vertex = CompileShader(GL_VERTEX_SHADER, kVertexShader);
    if (vertex == 0) {
        return 0;
    }
    GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, kFragmentShader);
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
    glBindAttribLocation(program, 0, "a_Position");
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

void MultiplyMatrix(float *out, const float *a, const float *b) {
    float result[16] = {};
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            result[column * 4 + row] =
                a[0 * 4 + row] * b[column * 4 + 0]
                + a[1 * 4 + row] * b[column * 4 + 1]
                + a[2 * 4 + row] * b[column * 4 + 2]
                + a[3 * 4 + row] * b[column * 4 + 3];
        }
    }
    std::memcpy(out, result, sizeof(result));
}

void MakeTransform(float *out, float x, float y, float z, float sx, float sy, float sz) {
    const float result[16] = {
        sx, 0.0f, 0.0f, 0.0f,
        0.0f, sy, 0.0f, 0.0f,
        0.0f, 0.0f, sz, 0.0f,
        x, y, z, 1.0f
    };
    std::memcpy(out, result, sizeof(result));
}

void MakeTranslation(float *out, float x, float y, float z) {
    MakeTransform(out, x, y, z, 1.0f, 1.0f, 1.0f);
}

static const float kCubeVertices[] = {
    -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
    -0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
     0.5f,-0.5f,-0.5f, -0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
     0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
    -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
    -0.5f,-0.5f,-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f,
     0.5f,-0.5f, 0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
     0.5f,-0.5f, 0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f,  0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,
    -0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
    -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,
    -0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f
};

void DestroyGl(ModuleState *state) {
    if (state->cube_vbo != 0) {
        glDeleteBuffers(1, &state->cube_vbo);
        state->cube_vbo = 0;
    }
    if (state->program != 0) {
        glDeleteProgram(state->program);
        state->program = 0;
    }
    state->position_location = -1;
    state->matrix_location = -1;
    state->color_location = -1;
    state->flicker_location = -1;
}

void AbandonGl(ModuleState *state) {
    state->cube_vbo = 0;
    state->program = 0;
    state->position_location = -1;
    state->matrix_location = -1;
    state->color_location = -1;
    state->flicker_location = -1;
}

int32_t InitializeGl(ModuleState *state) {
    AbandonGl(state);
    state->program = BuildProgram();
    if (state->program == 0) {
        Log(state, REVERIE_NATIVE_LOG_ERROR, "Shader program creation failed.");
        return 0;
    }
    state->position_location = glGetAttribLocation(state->program, "a_Position");
    state->matrix_location = glGetUniformLocation(state->program, "u_Mvp");
    state->color_location = glGetUniformLocation(state->program, "u_Color");
    state->flicker_location = glGetUniformLocation(state->program, "u_Flicker");
    if (state->position_location < 0 || state->matrix_location < 0
        || state->color_location < 0 || state->flicker_location < 0) {
        Log(state, REVERIE_NATIVE_LOG_ERROR, "Required shader locations are unavailable.");
        DestroyGl(state);
        return 0;
    }

    glGenBuffers(1, &state->cube_vbo);
    if (state->cube_vbo == 0) {
        Log(state, REVERIE_NATIVE_LOG_ERROR, "Cube vertex buffer creation failed.");
        DestroyGl(state);
        return 0;
    }
    glBindBuffer(GL_ARRAY_BUFFER, state->cube_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVertices), kCubeVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (glGetError() != GL_NO_ERROR) {
        Log(state, REVERIE_NATIVE_LOG_ERROR, "OpenGL ES initialization reported an error.");
        DestroyGl(state);
        return 0;
    }
    Log(state, REVERIE_NATIVE_LOG_INFO, "Red Ledger low-poly bar GL resources created.");
    return 1;
}

void DrawCube(
    ModuleState *state,
    const float *view_projection,
    float flicker,
    float x, float y, float z,
    float sx, float sy, float sz,
    float r, float g, float b
) {
    float model[16];
    float mvp[16];
    MakeTransform(model, x, y, z, sx, sy, sz);
    MultiplyMatrix(mvp, view_projection, model);
    glUniformMatrix4fv(state->matrix_location, 1, GL_FALSE, mvp);
    glUniform3f(state->color_location, r, g, b);
    glUniform1f(state->flicker_location, flicker);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void DrawRoom(ModuleState *state, const float *view_projection, float flicker) {
    DrawCube(state, view_projection, flicker, 0.0f,-1.55f,-0.6f, 6.0f,0.10f,5.0f, 0.20f,0.20f,0.18f);
    DrawCube(state, view_projection, flicker, 0.0f, 0.0f,-3.05f, 6.0f,3.1f,0.10f, 0.29f,0.28f,0.24f);
    DrawCube(state, view_projection, flicker,-3.05f, 0.0f,-0.6f, 0.10f,3.1f,5.0f, 0.27f,0.27f,0.24f);
    DrawCube(state, view_projection, flicker, 3.05f, 0.0f,-0.6f, 0.10f,3.1f,5.0f, 0.27f,0.27f,0.24f);
    DrawCube(state, view_projection, flicker, 0.0f, 1.55f,-0.6f, 6.0f,0.10f,5.0f, 0.18f,0.18f,0.17f);

    DrawCube(state, view_projection, flicker, 0.0f,-0.83f,-0.95f, 3.7f,0.85f,0.65f, 0.30f,0.20f,0.12f);
    DrawCube(state, view_projection, flicker, 0.0f,-0.36f,-0.95f, 3.9f,0.12f,0.75f, 0.39f,0.27f,0.15f);

    const bool tap_hover = state->hovered_target == WorkTarget::Tap;
    DrawCube(state, view_projection, flicker,-0.85f,-0.15f,-0.98f, 0.18f,0.40f,0.18f,
        tap_hover ? 0.76f : 0.36f,
        tap_hover ? 0.62f : 0.34f,
        tap_hover ? 0.24f : 0.30f);
    DrawCube(state, view_projection, flicker,-0.85f, 0.10f,-0.98f, 0.34f,0.12f,0.22f,
        tap_hover ? 0.82f : 0.25f,
        tap_hover ? 0.70f : 0.24f,
        tap_hover ? 0.28f : 0.22f);

    const bool wash_hover = state->hovered_target == WorkTarget::WashStation;
    DrawCube(state, view_projection, flicker,-0.28f,-0.20f,-1.18f, 0.92f,0.12f,0.48f,
        wash_hover ? 0.30f : 0.18f,
        wash_hover ? 0.58f : 0.30f,
        wash_hover ? 0.72f : 0.34f);

    DrawCube(state, view_projection, flicker, 1.35f,-0.95f,-1.80f, 0.72f,0.16f,0.72f, 0.22f,0.16f,0.12f);
    DrawCube(state, view_projection, flicker, 1.35f,-1.30f,-1.80f, 0.12f,0.70f,0.12f, 0.17f,0.13f,0.10f);

    DrawCube(state, view_projection, flicker, 1.90f,-1.38f,-2.38f, 1.35f,0.18f,0.70f, 0.31f,0.28f,0.23f);
    const bool ledger_hover = state->hovered_target == WorkTarget::Ledger;
    DrawCube(state, view_projection, flicker, 0.85f,-0.22f,-0.90f, 0.42f,0.06f,0.30f,
        ledger_hover ? 0.78f : 0.18f,
        ledger_hover ? 0.62f : 0.12f,
        ledger_hover ? 0.24f : 0.08f);

    const bool beer_hover = state->hovered_target == WorkTarget::BeerOrder;
    DrawCube(state, view_projection, flicker, 1.20f,-0.18f,-0.72f, 0.42f,0.05f,0.24f,
        beer_hover ? 0.78f : 0.34f,
        beer_hover ? 0.66f : 0.28f,
        beer_hover ? 0.25f : 0.12f);

    const bool cup_hover = state->hovered_target == WorkTarget::CupOrder;
    DrawCube(state, view_projection, flicker, 1.68f,-0.18f,-0.72f, 0.42f,0.05f,0.24f,
        cup_hover ? 0.76f : 0.30f,
        cup_hover ? 0.76f : 0.30f,
        cup_hover ? 0.70f : 0.26f);

    if (state->simulation.current_event() == EventType::ProtectionDemand
        && !state->simulation.protection_paid()) {
        const bool protection_hover =
            state->hovered_target == WorkTarget::ProtectionEnvelope;
        DrawCube(state, view_projection, flicker,-1.35f,-0.18f,-0.72f, 0.44f,0.05f,0.24f,
            protection_hover ? 0.86f : 0.50f,
            protection_hover ? 0.30f : 0.14f,
            protection_hover ? 0.24f : 0.12f);
    }

    const int clean = std::min(3, state->simulation.clean_cups());
    for (int i = 0; i < clean; ++i) {
        DrawCube(state, view_projection, flicker,
            0.25f + static_cast<float>(i) * 0.22f, -0.20f, -0.70f,
            0.13f,0.20f,0.13f, 0.56f,0.58f,0.52f);
    }
    const int dirty = std::min(3, state->simulation.dirty_cups());
    for (int i = 0; i < dirty; ++i) {
        DrawCube(state, view_projection, flicker,
            -0.05f - static_cast<float>(i) * 0.18f, -0.20f, -1.18f,
            0.13f,0.20f,0.13f, 0.34f,0.30f,0.24f);
    }

    if (state->simulation.current_patron() != nullptr) {
        DrawCube(state, view_projection, flicker, -1.30f,-0.52f,-2.05f, 0.52f,1.05f,0.36f, 0.24f,0.31f,0.34f);
        DrawCube(state, view_projection, flicker, -1.30f, 0.18f,-2.05f, 0.34f,0.34f,0.34f, 0.52f,0.43f,0.34f);
    }

    float marker_r = 0.28f;
    float marker_g = 0.36f;
    float marker_b = 0.30f;
    switch (state->simulation.current_event()) {
        case EventType::ProtectionDemand:
            marker_r = 0.48f; marker_g = 0.18f; marker_b = 0.16f;
            break;
        case EventType::Inspection:
            marker_r = 0.24f; marker_g = 0.32f; marker_b = 0.52f;
            break;
        case EventType::SupplyInterruption:
            marker_r = 0.50f; marker_g = 0.38f; marker_b = 0.12f;
            break;
        case EventType::None:
        default:
            break;
    }
    DrawCube(state, view_projection, flicker, 1.80f,0.45f,-2.98f, 0.75f,0.46f,0.04f, marker_r,marker_g,marker_b);

    DrawCube(state, view_projection, 1.0f, 0.0f,1.28f,-0.70f, 0.55f,0.10f,0.34f, 0.54f,0.45f,0.28f);
}

void *Create(const ReverieNativeHostV1 *host) {
    if (host == nullptr || host->struct_size < sizeof(ReverieNativeHostV1)
        || host->abi_version != REVERIE_NATIVE_MODULE_ABI_VERSION) {
        return nullptr;
    }
    ModuleState *state = new (std::nothrow) ModuleState();
    if (state == nullptr) {
        return nullptr;
    }
    state->host = host;
    LoadState(state);
    Log(state, REVERIE_NATIVE_LOG_INFO, "Red Ledger module created with controller-ray work surface.");
    return state;
}

void Destroy(void *instance) {
    ModuleState *state = static_cast<ModuleState *>(instance);
    if (state == nullptr) {
        return;
    }
    if (state->program != 0 || state->cube_vbo != 0) {
        Log(state, REVERIE_NATIVE_LOG_WARN, "Module destroyed after GL context release was skipped.");
        AbandonGl(state);
    }
    Log(state, REVERIE_NATIVE_LOG_INFO, "Red Ledger module destroyed.");
    delete state;
}

int32_t OnGlContextCreated(void *instance) {
    ModuleState *state = static_cast<ModuleState *>(instance);
    return state != nullptr ? InitializeGl(state) : 0;
}

void ReleaseGlContext(void *instance) {
    ModuleState *state = static_cast<ModuleState *>(instance);
    if (state == nullptr) {
        return;
    }
    DestroyGl(state);
    Log(state, REVERIE_NATIVE_LOG_INFO, "Red Ledger GL resources released.");
}

void Resume(void *instance) {
    Log(static_cast<ModuleState *>(instance), REVERIE_NATIVE_LOG_INFO, "Red Ledger module resumed.");
}

void Pause(void *instance) {
    Log(static_cast<ModuleState *>(instance), REVERIE_NATIVE_LOG_INFO, "Red Ledger module paused.");
}

void Update(void *instance, const ReverieNativeInputV1 *input) {
    ModuleState *state = static_cast<ModuleState *>(instance);
    if (state == nullptr || input == nullptr
        || input->struct_size < sizeof(ReverieNativeInputV1)) {
        return;
    }

    const float dt = std::max(0.0f, std::min(input->delta_seconds, 0.1f));
    state->elapsed_seconds += dt;
    state->player_x = std::max(-kMoveLimit, std::min(kMoveLimit,
        state->player_x + input->move_x * kMoveSpeed * dt));
    state->player_z = std::max(-kMoveLimit, std::min(kMoveLimit,
        state->player_z + input->move_y * kMoveSpeed * dt));

    state->hovered_target = FindWorkTarget(
        state,
        input,
        &state->hovered_distance
    );

    const bool primary = input->primary_down != 0u;
    if (primary && !state->primary_was_down) {
        if (state->hovered_target == WorkTarget::None) {
            Log(state, REVERIE_NATIVE_LOG_WARN,
                "Primary action had no reachable bar target.");
        } else {
            ActivateWorkTarget(state, state->hovered_target);
        }
    }
    state->primary_was_down = primary;
}

int32_t RenderEye(void *instance, const ReverieNativeEyeV1 *eye) {
    ModuleState *state = static_cast<ModuleState *>(instance);
    if (state == nullptr || eye == nullptr || eye->struct_size < sizeof(ReverieNativeEyeV1)
        || state->program == 0 || state->cube_vbo == 0) {
        return 0;
    }

    GLint previous_program = 0;
    GLint previous_array_buffer = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previous_program);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previous_array_buffer);
    const GLboolean depth_enabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean blend_enabled = glIsEnabled(GL_BLEND);
    const GLboolean cull_enabled = glIsEnabled(GL_CULL_FACE);

    float translation[16];
    float moved_view[16];
    float view_projection[16];
    MakeTranslation(translation, -state->player_x, 0.0f, -state->player_z);
    MultiplyMatrix(moved_view, eye->view, translation);
    MultiplyMatrix(view_projection, eye->projection, moved_view);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glUseProgram(state->program);
    glBindBuffer(GL_ARRAY_BUFFER, state->cube_vbo);
    glEnableVertexAttribArray(static_cast<GLuint>(state->position_location));
    glVertexAttribPointer(static_cast<GLuint>(state->position_location), 3, GL_FLOAT, GL_FALSE,
        3 * sizeof(float), reinterpret_cast<const void *>(0));

    const float wave = std::sin(state->elapsed_seconds * 7.0f)
        + 0.35f * std::sin(state->elapsed_seconds * 19.0f);
    const float flicker = std::max(0.72f, std::min(1.0f, 0.90f + wave * 0.06f));
    DrawRoom(state, view_projection, flicker);

    glDisableVertexAttribArray(static_cast<GLuint>(state->position_location));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(previous_array_buffer));
    glUseProgram(static_cast<GLuint>(previous_program));

    if (depth_enabled == GL_TRUE) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (blend_enabled == GL_TRUE) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (cull_enabled == GL_TRUE) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);

    return glGetError() == GL_NO_ERROR ? 1 : 0;
}

const ReverieNativeModuleApiV1 kApi = {
    sizeof(ReverieNativeModuleApiV1),
    REVERIE_NATIVE_MODULE_ABI_VERSION,
    {
        sizeof(ReverieNativeModuleDescriptorV1),
        REVERIE_NATIVE_MODULE_ABI_VERSION,
        "between-deliveries-red-ledger",
        "Between Deliveries: The Red Ledger VR",
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
    RenderEye
};

}  // namespace

extern "C"
__attribute__((visibility("default")))
const ReverieNativeModuleApiV1 *reverie_native_module_entry_v1(void) {
    return &kApi;
}
