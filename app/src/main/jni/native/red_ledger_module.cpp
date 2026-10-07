#include "reverie_native_sdk.h"
#include "red_ledger_simulation.h"
#include "procedural_material_atlas.h"
#include "red_ledger_static_geometry.h"

#include <GLES2/gl2.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <new>
#include <memory>

namespace {

using reverie::redledger::DrinkState;
using reverie::redledger::EventType;
using reverie::redledger::Simulation;
using reverie::redledger::SupplierItem;
using reverie::procedural::Material;
namespace geo = reverie::redledger::geometry;

constexpr float kMaximumWorkReach = 3.5f;
constexpr float kHeldCupDistance = 0.90f;
constexpr char kSaveSlot[] = "state-v2.bin";

enum class WorkTarget : uint8_t {
    None = 0,
    CupStack,
    Tap,
    Patron,
    Payment,
    WashStation,
    Ledger,
    BeerOrder,
    CupOrder,
    ProtectionEnvelope,
    ExitDoor
};

struct TargetBounds {
    WorkTarget target;
    float center[3];
    float half_extent[3];
};

static const TargetBounds kWorkTargets[] = {
    {
        WorkTarget::CupStack,
        {0.45f, -0.20f, -0.70f},
        {0.48f, 0.25f, 0.26f}
    },
    {
        WorkTarget::Tap,
        {-0.85f, -0.02f, -0.98f},
        {0.28f, 0.35f, 0.28f}
    },
    {
        WorkTarget::Patron,
        {-1.30f, -0.35f, -2.05f},
        {0.52f, 0.90f, 0.45f}
    },
    {
        WorkTarget::Payment,
        {-0.98f, -0.18f, -0.72f},
        {0.28f, 0.15f, 0.20f}
    },
    {
        WorkTarget::WashStation,
        {-0.28f, -0.18f, -1.18f},
        {0.55f, 0.22f, 0.30f}
    },
    {
        WorkTarget::Ledger,
        {0.85f, -0.20f, -0.90f},
        {0.30f, 0.12f, 0.24f}
    },
    {
        WorkTarget::BeerOrder,
        {1.20f, -0.18f, -0.72f},
        {0.24f, 0.14f, 0.15f}
    },
    {
        WorkTarget::CupOrder,
        {1.68f, -0.18f, -0.72f},
        {0.24f, 0.14f, 0.15f}
    },
    {
        WorkTarget::ProtectionEnvelope,
        {-1.58f, -0.18f, -0.72f},
        {0.26f, 0.13f, 0.15f}
    },
    {
        WorkTarget::ExitDoor,
        {-2.55f, -0.30f, -2.97f},
        {0.35f, 0.85f, 0.12f}
    }
};

struct ModuleState {
    const ReverieNativeHostV1 *host = nullptr;
    Simulation simulation;

    GLuint program = 0;
    GLuint cube_vbo = 0;
    GLuint static_vbo = 0;
    GLuint atlas_texture = 0;
    GLint position_location = -1;
    GLint uv_location = -1;
    GLint color_attribute_location = -1;
    GLint tile_origin_location = -1;
    GLint sampler_location = -1;
    int active_material = -1;
    GLint matrix_location = -1;
    GLint flicker_location = -1;

    float elapsed_seconds = 0.0f;
    float flicker = 0.90f;
    bool primary_was_down = false;

    bool pointer_active = false;
    float pointer_origin[3] = {};
    float pointer_direction[3] = {
        0.0f,
        0.0f,
        -1.0f
    };

    WorkTarget hovered_target =
        WorkTarget::None;
    float hovered_distance = 0.0f;
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
            "ReverieRedLedger",
            message
        );
    }
}

const char *WorkTargetName(
    WorkTarget target
) {
    switch (target) {
        case WorkTarget::CupStack:
            return "cup stack";
        case WorkTarget::Tap:
            return "tap";
        case WorkTarget::Patron:
            return "patron";
        case WorkTarget::Payment:
            return "payment";
        case WorkTarget::WashStation:
            return "wash station";
        case WorkTarget::Ledger:
            return "ledger";
        case WorkTarget::BeerOrder:
            return "beer order card";
        case WorkTarget::CupOrder:
            return "cup order card";
        case WorkTarget::ProtectionEnvelope:
            return "protection envelope";
        case WorkTarget::ExitDoor:
            return "exit door";
        case WorkTarget::None:
        default:
            return "nothing";
    }
}

bool IsTargetAvailable(
    const ModuleState *state,
    WorkTarget target
) {
    if (state == nullptr) {
        return false;
    }

    const Simulation &simulation =
        state->simulation;
    const DrinkState drink_state =
        simulation.work_drink_state();
    const bool no_payment =
        simulation.pending_payment_cents()
            == 0;
    const bool patron_waiting =
        simulation.current_patron()
            != nullptr;

    switch (target) {
        case WorkTarget::CupStack:
            return (
                drink_state
                    == DrinkState::None
                && no_payment
                && patron_waiting
                && simulation.clean_cups() > 0
            )
                || (
                    drink_state
                        == DrinkState::EmptyCup
                    && no_payment
                );

        case WorkTarget::Tap:
            return drink_state
                    == DrinkState::EmptyCup
                && no_payment
                && simulation.beer_units() > 0;

        case WorkTarget::Patron:
            return patron_waiting
                && no_payment
                && drink_state
                    == DrinkState::FilledCup;

        case WorkTarget::Payment:
            return simulation.pending_payment_cents()
                > 0;

        case WorkTarget::WashStation:
            return no_payment
                && drink_state
                    == DrinkState::None
                && simulation.dirty_cups() > 0;

        case WorkTarget::Ledger:
            return no_payment
                && drink_state
                    == DrinkState::None
                && simulation.patrons_remaining()
                    == 0;

        case WorkTarget::BeerOrder:
        case WorkTarget::CupOrder:
            return no_payment
                && drink_state
                    == DrinkState::None
                && simulation.current_event()
                    != EventType::SupplyInterruption;

        case WorkTarget::ProtectionEnvelope:
            return no_payment
                && drink_state
                    == DrinkState::None
                && simulation.current_event()
                    == EventType::ProtectionDemand
                && !simulation.protection_paid();

        case WorkTarget::ExitDoor:
            return patron_waiting
                && no_payment
                && drink_state
                    == DrinkState::None;

        case WorkTarget::None:
        default:
            return false;
    }
}

float RayAabbDistance(
    const float *origin,
    const float *direction,
    const TargetBounds &bounds
) {
    float near_distance = 0.0f;
    float far_distance =
        kMaximumWorkReach;

    for (int axis = 0;
         axis < 3;
         ++axis) {
        const float minimum =
            bounds.center[axis]
                - bounds.half_extent[axis];
        const float maximum =
            bounds.center[axis]
                + bounds.half_extent[axis];
        const float component =
            direction[axis];

        if (std::abs(component)
            < 0.000001f) {
            if (origin[axis] < minimum
                || origin[axis] > maximum) {
                return -1.0f;
            }
            continue;
        }

        float first =
            (minimum - origin[axis])
                / component;
        float second =
            (maximum - origin[axis])
                / component;

        if (first > second) {
            std::swap(
                first,
                second
            );
        }

        near_distance =
            std::max(
                near_distance,
                first
            );
        far_distance =
            std::min(
                far_distance,
                second
            );

        if (near_distance
            > far_distance) {
            return -1.0f;
        }
    }

    if (far_distance < 0.0f
        || near_distance
            > kMaximumWorkReach) {
        return -1.0f;
    }

    return std::max(
        0.0f,
        near_distance
    );
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
        || input->pointer_kind
            == REVERIE_NATIVE_POINTER_NONE) {
        return WorkTarget::None;
    }

    WorkTarget best =
        WorkTarget::None;
    float best_distance =
        kMaximumWorkReach + 1.0f;

    for (const TargetBounds &bounds
         : kWorkTargets) {
        if (!IsTargetAvailable(
                state,
                bounds.target
            )) {
            continue;
        }

        const float distance =
            RayAabbDistance(
                input->pointer_origin,
                input->pointer_direction,
                bounds
            );

        if (distance >= 0.0f
            && distance < best_distance) {
            best = bounds.target;
            best_distance = distance;
        }
    }

    if (best != WorkTarget::None
        && out_distance != nullptr) {
        *out_distance =
            best_distance;
    }

    return best;
}

bool SaveState(ModuleState *state) {
    if (state == nullptr
        || !ReverieNativeHostSupportsSaveV1(
            state->host
        )) {
        return false;
    }

    uint8_t bytes[
        Simulation::kSerializedSize
    ] = {};
    size_t size = 0u;

    if (!state->simulation.Serialize(
            bytes,
            sizeof(bytes),
            &size
        )) {
        Log(
            state,
            REVERIE_NATIVE_LOG_ERROR,
            "Could not serialize Red Ledger state."
        );
        return false;
    }

    const int32_t result =
        state->host->write_save(
            kSaveSlot,
            bytes,
            static_cast<uint32_t>(
                size
            )
        );

    if (result
        != REVERIE_NATIVE_SAVE_OK) {
        Log(
            state,
            REVERIE_NATIVE_LOG_WARN,
            "Host rejected Red Ledger save write."
        );
        return false;
    }

    return true;
}

void LoadState(ModuleState *state) {
    if (state == nullptr
        || !ReverieNativeHostSupportsSaveV1(
            state->host
        )) {
        return;
    }

    uint8_t bytes[
        Simulation::kSerializedSize
    ] = {};
    uint32_t size = 0u;

    const int32_t result =
        state->host->read_save(
            kSaveSlot,
            bytes,
            static_cast<uint32_t>(
                sizeof(bytes)
            ),
            &size
        );

    if (result
        == REVERIE_NATIVE_SAVE_NOT_FOUND) {
        Log(
            state,
            REVERIE_NATIVE_LOG_INFO,
            "No Red Ledger v2 save exists; using the opening state."
        );
        return;
    }

    if (result
            != REVERIE_NATIVE_SAVE_OK
        || size != sizeof(bytes)
        || !state->simulation.Deserialize(
            bytes,
            size
        )) {
        Log(
            state,
            REVERIE_NATIVE_LOG_WARN,
            "Red Ledger v2 save is invalid; preserving a fresh opening state."
        );
        return;
    }

    Log(
        state,
        REVERIE_NATIVE_LOG_INFO,
        "Red Ledger v2 save restored."
    );
}

bool ActivateWorkTarget(
    ModuleState *state,
    WorkTarget target
) {
    if (state == nullptr
        || !IsTargetAvailable(
            state,
            target
        )) {
        return false;
    }

    Simulation &simulation =
        state->simulation;
    bool changed = false;

    switch (target) {
        case WorkTarget::CupStack:
            if (simulation.work_drink_state()
                    == DrinkState::None) {
                changed =
                    simulation.TakeCleanCup();
            } else {
                changed =
                    simulation.ReturnHeldCup();
            }
            break;

        case WorkTarget::Tap:
            changed =
                simulation.FillHeldCup();
            break;

        case WorkTarget::Patron:
            changed =
                simulation.ServeHeldCup();
            break;

        case WorkTarget::Payment:
            changed =
                simulation.CollectPayment();
            break;

        case WorkTarget::WashStation:
            changed =
                simulation.WashOneCup();
            break;

        case WorkTarget::Ledger: {
            const int32_t closing_day =
                simulation.day();
            simulation.CloseDay();
            changed =
                simulation.day()
                    != closing_day;
            break;
        }

        case WorkTarget::BeerOrder:
            changed =
                simulation.BuySupply(
                    SupplierItem::BeerCrate
                );
            break;

        case WorkTarget::CupOrder:
            changed =
                simulation.BuySupply(
                    SupplierItem::CupSet
                );
            break;

        case WorkTarget::ProtectionEnvelope:
            changed =
                simulation.PayProtection();
            break;

        case WorkTarget::ExitDoor:
            changed =
                simulation.TurnAwayCurrentPatron();
            break;

        case WorkTarget::None:
        default:
            break;
    }

    if (!changed) {
        Log(
            state,
            REVERIE_NATIVE_LOG_WARN,
            "Targeted bar interaction could not complete."
        );
        return false;
    }

    Log(
        state,
        REVERIE_NATIVE_LOG_DEBUG,
        WorkTargetName(target)
    );
    SaveState(state);
    return true;
}

GLuint CompileShader(
    GLenum type,
    const char *source
) {
    GLuint shader =
        glCreateShader(type);

    if (shader == 0) {
        return 0;
    }

    glShaderSource(
        shader,
        1,
        &source,
        nullptr
    );
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

GLuint BuildProgram() {
    static const char *kVertexShader =
        "uniform mat4 u_Mvp;\n"
        "attribute vec3 a_Position;\n"
        "attribute vec2 a_Uv;\n"
        "attribute vec3 a_Color;\n"
        "attribute vec2 a_TileOrigin;\n"
        "varying vec2 v_Uv;\n"
        "varying vec3 v_Color;\n"
        "void main() {\n"
        "  v_Uv = a_TileOrigin + a_Uv * 0.4921875;\n"
        "  v_Color = a_Color;\n"
        "  gl_Position = u_Mvp * vec4(a_Position, 1.0);\n"
        "}\n";

    static const char *kFragmentShader =
        "precision mediump float;\n"
        "uniform float u_Flicker;\n"
        "uniform sampler2D u_Atlas;\n"
        "varying vec2 v_Uv;\n"
        "varying vec3 v_Color;\n"
        "void main() {\n"
        "  vec3 material = texture2D(u_Atlas, v_Uv).rgb;\n"
        "  gl_FragColor = vec4(material * v_Color * u_Flicker, 1.0);\n"
        "}\n";

    GLuint vertex =
        CompileShader(
            GL_VERTEX_SHADER,
            kVertexShader
        );

    if (vertex == 0) {
        return 0;
    }

    GLuint fragment =
        CompileShader(
            GL_FRAGMENT_SHADER,
            kFragmentShader
        );

    if (fragment == 0) {
        glDeleteShader(vertex);
        return 0;
    }

    GLuint program =
        glCreateProgram();

    if (program == 0) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return 0;
    }

    glAttachShader(
        program,
        vertex
    );
    glAttachShader(
        program,
        fragment
    );
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
    glBindAttribLocation(program, 2, "a_Color");
    glBindAttribLocation(program, 3, "a_TileOrigin");
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

void MultiplyMatrix(
    float *out,
    const float *a,
    const float *b
) {
    float result[16] = {};

    for (int column = 0;
         column < 4;
         ++column) {
        for (int row = 0;
             row < 4;
             ++row) {
            result[column * 4 + row] =
                a[0 * 4 + row]
                    * b[column * 4 + 0]
                + a[1 * 4 + row]
                    * b[column * 4 + 1]
                + a[2 * 4 + row]
                    * b[column * 4 + 2]
                + a[3 * 4 + row]
                    * b[column * 4 + 3];
        }
    }

    std::memcpy(
        out,
        result,
        sizeof(result)
    );
}

void MakeTransform(
    float *out,
    float x,
    float y,
    float z,
    float sx,
    float sy,
    float sz
) {
    const float result[16] = {
        sx, 0.0f, 0.0f, 0.0f,
        0.0f, sy, 0.0f, 0.0f,
        0.0f, 0.0f, sz, 0.0f,
        x, y, z, 1.0f
    };

    std::memcpy(
        out,
        result,
        sizeof(result)
    );
}

// UVs are baked once per context, never recomputed per eye or draw call.
void BuildCubeVertices(float *out) {
    for (int index = 0; index < 36; ++index) {
        const int face = index / 6;
        const float *position = &geo::kUnitCubeVertices[index * 3];
        const float u = (face < 2 || face >= 4)
            ? position[0] + 0.5f
            : position[2] + 0.5f;
        const float v = (face >= 4)
            ? position[2] + 0.5f
            : position[1] + 0.5f;
        for (int axis = 0; axis < 3; ++axis) {
            out[index * 5 + axis] = position[axis];
        }
        out[index * 5 + 3] = u;
        out[index * 5 + 4] = v;
    }
}

void DestroyGl(
    ModuleState *state
) {
    if (state->atlas_texture != 0) {
        glDeleteTextures(1, &state->atlas_texture);
        state->atlas_texture = 0;
    }
    if (state->static_vbo != 0) {
        glDeleteBuffers(1, &state->static_vbo);
        state->static_vbo = 0;
    }
    if (state->cube_vbo != 0) {
        glDeleteBuffers(
            1,
            &state->cube_vbo
        );
        state->cube_vbo = 0;
    }

    if (state->program != 0) {
        glDeleteProgram(state->program);
        state->program = 0;
    }

    state->position_location = -1;
    state->uv_location = -1;
    state->color_attribute_location = -1;
    state->tile_origin_location = -1;
    state->sampler_location = -1;
    state->active_material = -1;
    state->matrix_location = -1;
    state->flicker_location = -1;
}

void AbandonGl(
    ModuleState *state
) {
    state->cube_vbo = 0;
    state->static_vbo = 0;
    state->atlas_texture = 0;
    state->program = 0;
    state->position_location = -1;
    state->uv_location = -1;
    state->color_attribute_location = -1;
    state->tile_origin_location = -1;
    state->sampler_location = -1;
    state->active_material = -1;
    state->matrix_location = -1;
    state->flicker_location = -1;
}

int32_t InitializeGl(
    ModuleState *state
) {
    AbandonGl(state);

    state->program =
        BuildProgram();

    if (state->program == 0) {
        Log(
            state,
            REVERIE_NATIVE_LOG_ERROR,
            "Shader program creation failed."
        );
        return 0;
    }

    state->position_location =
        glGetAttribLocation(
            state->program,
            "a_Position"
        );
    state->uv_location =
        glGetAttribLocation(state->program, "a_Uv");
    state->color_attribute_location =
        glGetAttribLocation(state->program, "a_Color");
    state->tile_origin_location =
        glGetAttribLocation(state->program, "a_TileOrigin");
    state->sampler_location =
        glGetUniformLocation(state->program, "u_Atlas");
    state->matrix_location =
        glGetUniformLocation(
            state->program,
            "u_Mvp"
        );
    state->flicker_location =
        glGetUniformLocation(
            state->program,
            "u_Flicker"
        );

    if (state->position_location < 0
        || state->uv_location < 0
        || state->color_attribute_location < 0
        || state->tile_origin_location < 0
        || state->sampler_location < 0
        || state->matrix_location < 0
        || state->flicker_location < 0) {
        Log(
            state,
            REVERIE_NATIVE_LOG_ERROR,
            "Required shader locations are unavailable."
        );
        DestroyGl(state);
        return 0;
    }

    glGenBuffers(
        1,
        &state->cube_vbo
    );

    if (state->cube_vbo == 0) {
        Log(
            state,
            REVERIE_NATIVE_LOG_ERROR,
            "Cube vertex buffer creation failed."
        );
        DestroyGl(state);
        return 0;
    }

    glBindBuffer(
        GL_ARRAY_BUFFER,
        state->cube_vbo
    );
    float cube_vertices[36 * 5];
    BuildCubeVertices(cube_vertices);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(cube_vertices),
        cube_vertices,
        GL_STATIC_DRAW
    );
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // One context-lifetime world-space batch replaces 11 static draw calls.
    std::unique_ptr<float[]> room_vertices(
        new (std::nothrow) float[geo::kStaticVertexFloats]
    );
    if (!room_vertices || !geo::BuildStaticRoomVertices(
            room_vertices.get(), geo::kStaticVertexFloats)) {
        Log(state, REVERIE_NATIVE_LOG_ERROR,
            "Static room batch generation failed.");
        DestroyGl(state);
        return 0;
    }
    glGenBuffers(1, &state->static_vbo);
    if (state->static_vbo == 0) {
        Log(state, REVERIE_NATIVE_LOG_ERROR,
            "Static room VBO allocation failed.");
        DestroyGl(state);
        return 0;
    }
    glBindBuffer(GL_ARRAY_BUFFER, state->static_vbo);
    glBufferData(GL_ARRAY_BUFFER, geo::kStaticVertexBytes,
        room_vertices.get(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    room_vertices.reset();

    // Generate only on GL-context creation/recreation. The temporary CPU
    // buffer is freed immediately after upload; no runtime bitmap asset.
    // RGB565 is 32 KiB instead of 64 KiB RGBA8, and needs no
    // intermediate bitmap or conversion allocation.
    std::unique_ptr<uint16_t[]> pixels(
        new (std::nothrow) uint16_t[reverie::procedural::kAtlasPixelCount]
    );
    if (!pixels || !reverie::procedural::GenerateMaterialAtlasRgb565(
            reverie::procedural::kRedLedgerMaterialSeed,
            pixels.get(),
            reverie::procedural::kAtlasRgb565Bytes)) {
        Log(state, REVERIE_NATIVE_LOG_ERROR,
            "Procedural atlas allocation/generation failed.");
        DestroyGl(state);
        return 0;
    }
    GLint previous_active_texture = 0;
    GLint previous_texture = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previous_active_texture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture);
    glGenTextures(1, &state->atlas_texture);
    if (state->atlas_texture != 0) {
        glBindTexture(GL_TEXTURE_2D, state->atlas_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGB,
            reverie::procedural::kAtlasWidth,
            reverie::procedural::kAtlasHeight,
            0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pixels.get()
        );
    }
    glBindTexture(GL_TEXTURE_2D,
        static_cast<GLuint>(previous_texture));
    glActiveTexture(static_cast<GLenum>(previous_active_texture));
    pixels.reset();

    if (state->atlas_texture == 0 || glGetError()
        != GL_NO_ERROR) {
        Log(
            state,
            REVERIE_NATIVE_LOG_ERROR,
            "OpenGL ES initialization reported an error."
        );
        DestroyGl(state);
        return 0;
    }

    Log(
        state,
        REVERIE_NATIVE_LOG_INFO,
        "Red Ledger GL: 32 KiB RGB565 atlas and one static room VBO."
    );
    return 1;
}

void DrawCube(
    ModuleState *state,
    const float *view_projection,
    float flicker,
    float x,
    float y,
    float z,
    float sx,
    float sy,
    float sz,
    float r,
    float g,
    float b,
    Material material = Material::Stone
) {
    float model[16];
    float mvp[16];

    MakeTransform(
        model,
        x,
        y,
        z,
        sx,
        sy,
        sz
    );
    MultiplyMatrix(
        mvp,
        view_projection,
        model
    );

    glUniformMatrix4fv(
        state->matrix_location,
        1,
        GL_FALSE,
        mvp
    );
    glVertexAttrib3f(
        static_cast<GLuint>(state->color_attribute_location),
        r, g, b
    );
    glUniform1f(
        state->flicker_location,
        flicker
    );
    const int material_index = static_cast<int>(material);
    if (state->active_material != material_index) {
        state->active_material = material_index;
        glVertexAttrib2f(
            static_cast<GLuint>(state->tile_origin_location),
            0.00390625f + (material_index & 1) * 0.5f,
            0.00390625f + (material_index >> 1) * 0.5f
        );
    }
    glDrawArrays(
        GL_TRIANGLES,
        0,
        36
    );
}

void DrawCup(
    ModuleState *state,
    const float *view_projection,
    float flicker,
    float x,
    float y,
    float z,
    bool filled,
    bool highlighted
) {
    float r = filled ? 0.72f : 0.52f;
    float g = filled ? 0.43f : 0.54f;
    float b = filled ? 0.16f : 0.50f;

    if (highlighted) {
        r = std::min(
            1.0f,
            r + 0.24f
        );
        g = std::min(
            1.0f,
            g + 0.24f
        );
        b = std::min(
            1.0f,
            b + 0.18f
        );
    }

    DrawCube(
        state,
        view_projection,
        flicker,
        x,
        y,
        z,
        0.13f,
        0.20f,
        0.13f,
        r,
        g,
        b,
        Material::Metal
    );
}

// Immutable world-space room: one GLES2 draw per eye, no per-frame upload.
void DrawStaticRoom(ModuleState *state, const float *vp, float flicker) {
    const GLsizei stride = static_cast<GLsizei>(
        geo::kStaticVertexStride * sizeof(float)
    );
    glBindBuffer(GL_ARRAY_BUFFER, state->static_vbo);
    glVertexAttribPointer(
        static_cast<GLuint>(state->position_location),
        3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void *>(0)
    );
    glVertexAttribPointer(
        static_cast<GLuint>(state->uv_location),
        2, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void *>(3u * sizeof(float))
    );
    glEnableVertexAttribArray(
        static_cast<GLuint>(state->color_attribute_location)
    );
    glVertexAttribPointer(
        static_cast<GLuint>(state->color_attribute_location),
        3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void *>(5u * sizeof(float))
    );
    glEnableVertexAttribArray(
        static_cast<GLuint>(state->tile_origin_location)
    );
    glVertexAttribPointer(
        static_cast<GLuint>(state->tile_origin_location),
        2, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<const void *>(8u * sizeof(float))
    );
    glUniformMatrix4fv(state->matrix_location, 1, GL_FALSE, vp);
    glUniform1f(state->flicker_location, flicker);
    glDrawArrays(GL_TRIANGLES, 0,
        static_cast<GLsizei>(geo::kStaticVertexCount));

    // Dynamic props still use the unit-cube VBO and constant attributes.
    glDisableVertexAttribArray(
        static_cast<GLuint>(state->color_attribute_location)
    );
    glDisableVertexAttribArray(
        static_cast<GLuint>(state->tile_origin_location)
    );
    glBindBuffer(GL_ARRAY_BUFFER, state->cube_vbo);
    glVertexAttribPointer(
        static_cast<GLuint>(state->position_location),
        3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
        reinterpret_cast<const void *>(0)
    );
    glVertexAttribPointer(
        static_cast<GLuint>(state->uv_location),
        2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
        reinterpret_cast<const void *>(3u * sizeof(float))
    );
    state->active_material = -1;
}

void DrawRoom(
    ModuleState *state,
    const float *view_projection,
    float flicker
) {
    DrawStaticRoom(state, view_projection, flicker);
    const bool tap_hover =
        state->hovered_target
            == WorkTarget::Tap;

    DrawCube(
        state,
        view_projection,
        flicker,
        -0.85f,-0.15f,-0.98f,
        0.18f,0.40f,0.18f,
        tap_hover ? 0.76f : 0.36f,
        tap_hover ? 0.62f : 0.34f,
        tap_hover ? 0.24f : 0.30f,
        Material::Metal
    );
    DrawCube(
        state,
        view_projection,
        flicker,
        -0.85f,0.10f,-0.98f,
        0.34f,0.12f,0.22f,
        tap_hover ? 0.82f : 0.25f,
        tap_hover ? 0.70f : 0.24f,
        tap_hover ? 0.28f : 0.22f,
        Material::Metal
    );

    const bool wash_hover =
        state->hovered_target
            == WorkTarget::WashStation;

    DrawCube(
        state,
        view_projection,
        flicker,
        -0.28f,-0.20f,-1.18f,
        0.92f,0.12f,0.48f,
        wash_hover ? 0.30f : 0.18f,
        wash_hover ? 0.58f : 0.30f,
        wash_hover ? 0.72f : 0.34f,
        Material::Metal
    );

    const bool ledger_hover =
        state->hovered_target
            == WorkTarget::Ledger;

    DrawCube(
        state,
        view_projection,
        flicker,
        0.85f,-0.22f,-0.90f,
        0.42f,0.06f,0.30f,
        ledger_hover ? 0.78f : 0.18f,
        ledger_hover ? 0.62f : 0.12f,
        ledger_hover ? 0.24f : 0.08f,
        Material::Paper
    );

    const bool beer_hover =
        state->hovered_target
            == WorkTarget::BeerOrder;
    DrawCube(
        state,
        view_projection,
        flicker,
        1.20f,-0.18f,-0.72f,
        0.42f,0.05f,0.24f,
        beer_hover ? 0.78f : 0.34f,
        beer_hover ? 0.66f : 0.28f,
        beer_hover ? 0.25f : 0.12f,
        Material::Paper
    );

    const bool order_cup_hover =
        state->hovered_target
            == WorkTarget::CupOrder;
    DrawCube(
        state,
        view_projection,
        flicker,
        1.68f,-0.18f,-0.72f,
        0.42f,0.05f,0.24f,
        order_cup_hover ? 0.76f : 0.30f,
        order_cup_hover ? 0.76f : 0.30f,
        order_cup_hover ? 0.70f : 0.26f,
        Material::Paper
    );

    if (state->simulation.current_event()
            == EventType::ProtectionDemand
        && !state->simulation
            .protection_paid()) {
        const bool protection_hover =
            state->hovered_target
                == WorkTarget::ProtectionEnvelope;

        DrawCube(
            state,
            view_projection,
            flicker,
            -1.58f,-0.18f,-0.72f,
            0.44f,0.05f,0.24f,
            protection_hover ? 0.86f : 0.50f,
            protection_hover ? 0.30f : 0.14f,
            protection_hover ? 0.24f : 0.12f,
            Material::Paper
        );
    }

    const bool cup_stack_hover =
        state->hovered_target
            == WorkTarget::CupStack;

    DrawCube(
        state,
        view_projection,
        flicker,
        0.45f,-0.31f,-0.70f,
        0.82f,0.06f,0.34f,
        cup_stack_hover ? 0.58f : 0.24f,
        cup_stack_hover ? 0.56f : 0.23f,
        cup_stack_hover ? 0.48f : 0.20f,
        Material::Wood
    );

    const int clean =
        std::min(
            3,
            state->simulation.clean_cups()
        );

    for (int index = 0;
         index < clean;
         ++index) {
        DrawCup(
            state,
            view_projection,
            flicker,
            0.25f
                + static_cast<float>(
                    index
                ) * 0.22f,
            -0.20f,
            -0.70f,
            false,
            cup_stack_hover
        );
    }

    const int dirty =
        std::min(
            3,
            state->simulation.dirty_cups()
        );

    for (int index = 0;
         index < dirty;
         ++index) {
        DrawCup(
            state,
            view_projection,
            flicker,
            -0.05f
                - static_cast<float>(
                    index
                ) * 0.18f,
            -0.20f,
            -1.18f,
            false,
            wash_hover
        );
    }

    if (state->simulation.current_patron()
        != nullptr) {
        const bool patron_hover =
            state->hovered_target
                == WorkTarget::Patron;
        const float body_r =
            patron_hover
                ? 0.42f
                : 0.24f;
        const float body_g =
            patron_hover
                ? 0.52f
                : 0.31f;
        const float body_b =
            patron_hover
                ? 0.55f
                : 0.34f;

        DrawCube(
            state,
            view_projection,
            flicker,
            -1.30f,-0.52f,-2.05f,
            0.52f,1.05f,0.36f,
            body_r,body_g,body_b
        );
        DrawCube(
            state,
            view_projection,
            flicker,
            -1.30f,0.18f,-2.05f,
            0.34f,0.34f,0.34f,
            patron_hover ? 0.70f : 0.52f,
            patron_hover ? 0.58f : 0.43f,
            patron_hover ? 0.44f : 0.34f
        );
    }

    if (state->simulation.pending_payment_cents()
        > 0) {
        const bool payment_hover =
            state->hovered_target
                == WorkTarget::Payment;

        DrawCube(
            state,
            view_projection,
            flicker,
            -0.98f,-0.17f,-0.72f,
            0.12f,0.04f,0.12f,
            payment_hover ? 0.95f : 0.72f,
            payment_hover ? 0.82f : 0.62f,
            payment_hover ? 0.28f : 0.16f,
            Material::Metal
        );
        DrawCube(
            state,
            view_projection,
            flicker,
            -0.82f,-0.17f,-0.72f,
            0.10f,0.04f,0.10f,
            payment_hover ? 0.94f : 0.68f,
            payment_hover ? 0.80f : 0.58f,
            payment_hover ? 0.26f : 0.14f,
            Material::Metal
        );
    }

    if (state->simulation.current_patron()
        != nullptr
        && state->simulation.pending_payment_cents()
            == 0
        && state->simulation.work_drink_state()
            == DrinkState::None) {
        const bool exit_hover =
            state->hovered_target
                == WorkTarget::ExitDoor;

        DrawCube(
            state,
            view_projection,
            flicker,
            -2.55f,-0.30f,-2.97f,
            0.58f,1.50f,0.05f,
            exit_hover ? 0.68f : 0.33f,
            exit_hover ? 0.28f : 0.18f,
            exit_hover ? 0.22f : 0.16f,
            Material::Wood
        );
    }

    if (state->pointer_active
        && state->simulation.work_drink_state()
            != DrinkState::None) {
        const float cup_x =
            state->pointer_origin[0]
            + state->pointer_direction[0]
                * kHeldCupDistance;
        const float cup_y =
            state->pointer_origin[1]
            + state->pointer_direction[1]
                * kHeldCupDistance;
        const float cup_z =
            state->pointer_origin[2]
            + state->pointer_direction[2]
                * kHeldCupDistance;

        DrawCup(
            state,
            view_projection,
            1.0f,
            cup_x,
            cup_y,
            cup_z,
            state->simulation.work_drink_state()
                == DrinkState::FilledCup,
            true
        );
    }

    if (state->pointer_active
        && state->hovered_target
            != WorkTarget::None
        && state->hovered_distance
            > 0.0f) {
        const float contact_x =
            state->pointer_origin[0]
            + state->pointer_direction[0]
                * state->hovered_distance;
        const float contact_y =
            state->pointer_origin[1]
            + state->pointer_direction[1]
                * state->hovered_distance;
        const float contact_z =
            state->pointer_origin[2]
            + state->pointer_direction[2]
                * state->hovered_distance;

        DrawCube(
            state,
            view_projection,
            1.0f,
            contact_x,
            contact_y,
            contact_z,
            0.055f,
            0.055f,
            0.055f,
            0.95f,
            0.90f,
            0.28f
        );
    }

    float marker_r = 0.28f;
    float marker_g = 0.36f;
    float marker_b = 0.30f;

    switch (
        state->simulation.current_event()
    ) {
        case EventType::ProtectionDemand:
            marker_r = 0.48f;
            marker_g = 0.18f;
            marker_b = 0.16f;
            break;

        case EventType::Inspection:
            marker_r = 0.24f;
            marker_g = 0.32f;
            marker_b = 0.52f;
            break;

        case EventType::SupplyInterruption:
            marker_r = 0.50f;
            marker_g = 0.38f;
            marker_b = 0.12f;
            break;

        case EventType::None:
        default:
            break;
    }

    DrawCube(
        state,
        view_projection,
        flicker,
        1.80f,0.45f,-2.98f,
        0.75f,0.46f,0.04f,
        marker_r,marker_g,marker_b,
        Material::Paper
    );

}

void *Create(
    const ReverieNativeHostV1 *host
) {
    if (!ReverieNativeHostSupportsSaveV1(
            host
        )) {
        return nullptr;
    }

    ModuleState *state =
        new (std::nothrow)
            ModuleState();

    if (state == nullptr) {
        return nullptr;
    }

    state->host = host;
    LoadState(state);

    Log(
        state,
        REVERIE_NATIVE_LOG_INFO,
        "Red Ledger module created with manual cup/service/payment loop."
    );
    return state;
}

void Destroy(
    void *instance
) {
    ModuleState *state =
        static_cast<ModuleState *>(
            instance
        );

    if (state == nullptr) {
        return;
    }

    if (state->program != 0
        || state->cube_vbo != 0
        || state->static_vbo != 0
        || state->atlas_texture != 0) {
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
        "Red Ledger module destroyed."
    );
    delete state;
}

int32_t OnGlContextCreated(
    void *instance
) {
    ModuleState *state =
        static_cast<ModuleState *>(
            instance
        );

    return state != nullptr
        ? InitializeGl(state)
        : 0;
}

void ReleaseGlContext(
    void *instance
) {
    ModuleState *state =
        static_cast<ModuleState *>(
            instance
        );

    if (state == nullptr) {
        return;
    }

    DestroyGl(state);
    Log(
        state,
        REVERIE_NATIVE_LOG_INFO,
        "Red Ledger GL resources released."
    );
}

void Resume(
    void *instance
) {
    Log(
        static_cast<ModuleState *>(
            instance
        ),
        REVERIE_NATIVE_LOG_INFO,
        "Red Ledger module resumed."
    );
}

void Pause(
    void *instance
) {
    Log(
        static_cast<ModuleState *>(
            instance
        ),
        REVERIE_NATIVE_LOG_INFO,
        "Red Ledger module paused."
    );
}

void Update(
    void *instance,
    const ReverieNativeInputV1 *input
) {
    ModuleState *state =
        static_cast<ModuleState *>(
            instance
        );

    if (state == nullptr
        || !ReverieNativeInputHasPointerV1(
            input
        )) {
        return;
    }

    const float dt =
        std::max(
            0.0f,
            std::min(
                input->delta_seconds,
                0.1f
            )
        );
    state->elapsed_seconds += dt;
    // Calculate lighting once per simulation update, not separately for
    // the two eyes. Both stereo views receive identical illumination.
    const float wave =
        std::sin(state->elapsed_seconds * 7.0f)
        + 0.35f * std::sin(state->elapsed_seconds * 19.0f);
    state->flicker = std::max(0.72f,
        std::min(1.0f, 0.90f + wave * 0.06f));

    ReverieNativeInputV1 effective_input =
        *input;

    if (effective_input.pointer_kind
            != REVERIE_NATIVE_POINTER_NONE) {
        const float direction_length =
            std::sqrt(
                effective_input.pointer_direction[0]
                    * effective_input.pointer_direction[0]
                + effective_input.pointer_direction[1]
                    * effective_input.pointer_direction[1]
                + effective_input.pointer_direction[2]
                    * effective_input.pointer_direction[2]
            );

        if (direction_length
            > 0.000001f) {
            for (int axis = 0;
                 axis < 3;
                 ++axis) {
                effective_input.pointer_direction[axis] /=
                    direction_length;
            }
        } else {
            effective_input.pointer_kind =
                REVERIE_NATIVE_POINTER_NONE;
        }
    }

    state->pointer_active =
        effective_input.pointer_kind
            != REVERIE_NATIVE_POINTER_NONE;

    if (state->pointer_active) {
        for (int axis = 0;
             axis < 3;
             ++axis) {
            state->pointer_origin[axis] =
                effective_input.pointer_origin[axis];
            state->pointer_direction[axis] =
                effective_input.pointer_direction[axis];
        }
    }

    state->hovered_target =
        FindWorkTarget(
            state,
            &effective_input,
            &state->hovered_distance
        );

    const bool primary =
        input->primary_down != 0u;

    if (primary
        && !state->primary_was_down) {
        if (state->hovered_target
            == WorkTarget::None) {
            Log(
                state,
                REVERIE_NATIVE_LOG_WARN,
                "Primary action had no reachable bar target."
            );
        } else {
            ActivateWorkTarget(
                state,
                state->hovered_target
            );
        }
    }

    state->primary_was_down =
        primary;
}

int32_t RenderEye(
    void *instance,
    const ReverieNativeEyeV1 *eye
) {
    ModuleState *state =
        static_cast<ModuleState *>(
            instance
        );

    if (state == nullptr
        || !ReverieNativeEyeHasMatricesV1(
            eye
        )
        || state->program == 0
        || state->cube_vbo == 0
        || state->static_vbo == 0
        || state->atlas_texture == 0) {
        return 0;
    }

    GLint previous_program = 0;
    GLint previous_array_buffer = 0;
    GLint previous_active_texture = 0;
    GLint previous_texture = 0;

    glGetIntegerv(
        GL_CURRENT_PROGRAM,
        &previous_program
    );
    glGetIntegerv(
        GL_ARRAY_BUFFER_BINDING,
        &previous_array_buffer
    );
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previous_active_texture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture);

    const GLboolean depth_enabled =
        glIsEnabled(
            GL_DEPTH_TEST
        );
    const GLboolean blend_enabled =
        glIsEnabled(
            GL_BLEND
        );
    const GLboolean cull_enabled =
        glIsEnabled(
            GL_CULL_FACE
        );

    float view_projection[16];
    MultiplyMatrix(
        view_projection,
        eye->projection,
        eye->view
    );

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    glUseProgram(
        state->program
    );
    glBindTexture(GL_TEXTURE_2D, state->atlas_texture);
    glUniform1i(state->sampler_location, 0);
    state->active_material = -1;
    glBindBuffer(
        GL_ARRAY_BUFFER,
        state->cube_vbo
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
        reinterpret_cast<
            const void *
        >(0)
    );
    glEnableVertexAttribArray(
        static_cast<GLuint>(state->uv_location)
    );
    glVertexAttribPointer(
        static_cast<GLuint>(state->uv_location),
        2, GL_FLOAT, GL_FALSE,
        5 * sizeof(float),
        reinterpret_cast<const void *>(3 * sizeof(float))
    );

    const float flicker = state->flicker;

    DrawRoom(
        state,
        view_projection,
        flicker
    );

    glDisableVertexAttribArray(
        static_cast<GLuint>(
            state->position_location
        )
    );
    glDisableVertexAttribArray(
        static_cast<GLuint>(state->uv_location)
    );
    glBindTexture(GL_TEXTURE_2D,
        static_cast<GLuint>(previous_texture));
    glActiveTexture(static_cast<GLenum>(previous_active_texture));
    glBindBuffer(
        GL_ARRAY_BUFFER,
        static_cast<GLuint>(
            previous_array_buffer
        )
    );
    glUseProgram(
        static_cast<GLuint>(
            previous_program
        )
    );

    if (depth_enabled
        == GL_TRUE) {
        glEnable(
            GL_DEPTH_TEST
        );
    } else {
        glDisable(
            GL_DEPTH_TEST
        );
    }

    if (blend_enabled
        == GL_TRUE) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }

    if (cull_enabled
        == GL_TRUE) {
        glEnable(
            GL_CULL_FACE
        );
    } else {
        glDisable(
            GL_CULL_FACE
        );
    }

    return glGetError()
            == GL_NO_ERROR
        ? 1
        : 0;
}

const ReverieNativeModuleApiV1 kApi = {
    sizeof(
        ReverieNativeModuleApiV1
    ),
    REVERIE_NATIVE_MODULE_ABI_VERSION,
    {
        sizeof(
            ReverieNativeModuleDescriptorV1
        ),
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
const ReverieNativeModuleApiV1 *
reverie_native_module_entry_v1(void) {
    return &kApi;
}
