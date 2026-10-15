#include "reverie_native_sdk.h"
#include "reverie_native_sound_queue.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <cmath>
#include <cstdio>

namespace {

int failures = 0;
int captured_log_count = 0;
uint32_t captured_feedback_flags = 0u;
uint32_t captured_sound_cue = 0u;
unsigned captured_sound_calls = 0u;

void Check(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

void DummyLog(
    int32_t,
    const char *,
    const char *
) {
    ++captured_log_count;
}

int32_t DummyReadSave(
    const char *,
    void *,
    uint32_t,
    uint32_t *
) {
    return REVERIE_NATIVE_SAVE_NOT_FOUND;
}

int32_t DummyWriteSave(
    const char *,
    const void *,
    uint32_t
) {
    return REVERIE_NATIVE_SAVE_OK;
}

void DummyFeedback(
    uint32_t flags
) {
    captured_feedback_flags |= flags;
}
void DummySound(uint32_t cue_id) {
    captured_sound_cue=cue_id;
    ++captured_sound_calls;
}

}  // namespace

static_assert(
    REVERIE_NATIVE_DESCRIPTOR_V1_SIZE
        == sizeof(ReverieNativeModuleDescriptorV1),
    "descriptor v1 size must stay frozen"
);

static_assert(
    REVERIE_NATIVE_MODULE_API_V1_MIN_SIZE
        == offsetof(
            ReverieNativeModuleApiV1,
            capabilities
        ),
    "optional API tail must begin exactly after mandatory callbacks"
);

static_assert(
    REVERIE_NATIVE_MODULE_API_V1_CAPABILITIES_MIN_SIZE
        <= sizeof(ReverieNativeModuleApiV1),
    "capability-tail prefix must fit the current API table"
);

int main() {
    ReverieNativeHostV1 host = {};
    host.struct_size =
        REVERIE_NATIVE_HOST_V1_LOG_MIN_SIZE;
    host.abi_version =
        REVERIE_NATIVE_MODULE_ABI_VERSION;
    host.log = DummyLog;
    host.read_save = DummyReadSave;
    host.write_save = DummyWriteSave;
    host.request_feedback = DummyFeedback;
    host.request_sound = DummySound;

    Check(
        ReverieNativeHostSupportsLogV1(&host) != 0,
        "log-prefix host should expose logging"
    );
    Check(
        ReverieNativeLogV1(
            &host,
            REVERIE_NATIVE_LOG_INFO,
            nullptr,
            nullptr
        ) != 0
            && captured_log_count == 1,
        "log helper should work on minimum log-prefix host"
    );
    Check(
        ReverieNativeHostSupportsSaveV1(&host) == 0,
        "log-prefix host must not expose save tail"
    );

    host.struct_size =
        REVERIE_NATIVE_HOST_V1_SAVE_MIN_SIZE;
    Check(
        ReverieNativeHostSupportsSaveV1(&host) != 0,
        "save-prefix host should expose save callbacks"
    );
    Check(
        ReverieNativeHostSupportsFeedbackV1(&host) == 0,
        "save-prefix host must not expose feedback tail"
    );

    host.struct_size =
        REVERIE_NATIVE_HOST_V1_FEEDBACK_MIN_SIZE;
    Check(
        ReverieNativeHostSupportsFeedbackV1(&host) != 0,
        "feedback-prefix host should expose cue callback"
    );
    Check(
        ReverieNativeRequestFeedbackV1(
            &host,
            REVERIE_NATIVE_FEEDBACK_ACTIVATION
                | REVERIE_NATIVE_FEEDBACK_FAILURE
                | 0x80000000u
        ) != 0,
        "known feedback flags should be submitted"
    );
    Check(
        captured_feedback_flags
            == (
                REVERIE_NATIVE_FEEDBACK_ACTIVATION
                | REVERIE_NATIVE_FEEDBACK_FAILURE
            ),
        "feedback helper must strip unknown flag bits"
    );
    captured_feedback_flags = 0u;
    Check(
        ReverieNativeRequestFeedbackV1(
            &host, REVERIE_NATIVE_FEEDBACK_WEAPON_FIRE
                | REVERIE_NATIVE_FEEDBACK_RELOAD_START
                | REVERIE_NATIVE_FEEDBACK_RELOAD_READY
                | REVERIE_NATIVE_FEEDBACK_ARTILLERY
                | REVERIE_NATIVE_FEEDBACK_AIRSTRIKE
        ) != 0 &&
        captured_feedback_flags ==
            (REVERIE_NATIVE_FEEDBACK_WEAPON_FIRE
             | REVERIE_NATIVE_FEEDBACK_RELOAD_START
             | REVERIE_NATIVE_FEEDBACK_RELOAD_READY
             | REVERIE_NATIVE_FEEDBACK_ARTILLERY
             | REVERIE_NATIVE_FEEDBACK_AIRSTRIKE),
        "fixed gameplay sound cues must survive ABI sanitization"
    );

    // New host fields are strictly append-only, so older v1 hosts can
    // still offer their original feedback and save callbacks safely.
    Check(REVERIE_NATIVE_HOST_V1_SOUND_MIN_SIZE
            > REVERIE_NATIVE_HOST_V1_FEEDBACK_MIN_SIZE,
          "ordered sound-event tail must follow feedback");
    Check(ReverieNativeHostSupportsSoundV1(&host) == 0,
          "feedback-only host must not expose ordered audio");
    Check(ReverieNativeRequestSoundV1(&host,
              REVERIE_NATIVE_SOUND_WEAPON_MACHINE) == 0,
          "old host cannot accept ordered sound cues");
    host.struct_size = REVERIE_NATIVE_HOST_V1_SOUND_MIN_SIZE;
    Check(ReverieNativeHostSupportsSoundV1(&host) != 0,
          "current host should expose ordered sound service");
    for(uint32_t cue=REVERIE_NATIVE_SOUND_WEAPON_MACHINE;
        cue<=REVERIE_NATIVE_SOUND_MAX_V1;++cue) {
        Check(ReverieNativeRequestSoundV1(&host,cue) != 0,
              "valid ordered cue should dispatch");
    }
    Check(captured_sound_calls==5u &&
          captured_sound_cue==REVERIE_NATIVE_SOUND_WEAPON_CANNON,
          "one callback per distinct weapon report");
    Check(ReverieNativeRequestSoundV1(&host,0u)==0 &&
          ReverieNativeRequestSoundV1(&host,9999u)==0 &&
          captured_sound_calls==5u,
          "invalid cue ids rejected without reaching host");
    host.request_sound=nullptr;
    Check(ReverieNativeRequestSoundV1(&host,
          REVERIE_NATIVE_SOUND_WEAPON_MACHINE)==0,
          "missing sound callback must fail closed");
    host.request_sound=DummySound;

    // The actual host uses this exact bounded ordered queue.
    ReverieNativeSoundQueueV1 ordered;
    Check(!ordered.Push(0u) && !ordered.Push(9999u) &&
          ordered.count==0u,"invalid sound IDs never enter queue");
    for(uint32_t index=0u;index<8u;++index)
        Check(ordered.Push(1u+(index%5u)),"first eight ordered cues accepted");
    Check(!ordered.Push(1u),"ninth queued sound must fail boundedly");
    uint64_t packet=ordered.Drain();
    for(uint32_t index=0u;index<8u;++index) {
        const uint32_t cue=static_cast<uint32_t>(packet & 255u);
        Check(cue==1u+(index%5u),"order and multiplicity preserved");
        packet>>=8u;
    }
    Check(packet==0u && ordered.count==0u && ordered.packed==0u,
          "drain must clear queue");
    ordered.Push(2u);
    ordered.Clear();
    Check(ordered.Drain()==0u,"pause clears unplayed requests");

    ReverieNativeModuleDescriptorV1 descriptor = {};
    descriptor.struct_size =
        REVERIE_NATIVE_DESCRIPTOR_V1_SIZE;
    descriptor.abi_version =
        REVERIE_NATIVE_MODULE_ABI_VERSION;
    descriptor.required_gles_major = 2u;
    descriptor.required_gles_minor = 0u;

    Check(
        ReverieNativeDescriptorHasMandatoryV1(
            &descriptor
        ) != 0,
        "exact v1 descriptor size should be admitted"
    );
    descriptor.struct_size += 1u;
    Check(
        ReverieNativeDescriptorHasMandatoryV1(
            &descriptor
        ) == 0,
        "embedded v1 descriptor must reject layout growth"
    );
    descriptor.struct_size =
        REVERIE_NATIVE_DESCRIPTOR_V1_SIZE;

    Check(
        ReverieNativeGlesRequirementSupportedV1(
            &descriptor,
            2u,
            0u
        ) != 0,
        "GLES 2.0 requirement should fit GLES 2.0 host"
    );
    descriptor.required_gles_minor = 1u;
    Check(
        ReverieNativeGlesRequirementSupportedV1(
            &descriptor,
            2u,
            0u
        ) == 0,
        "GLES 2.1 requirement must not fit GLES 2.0 host"
    );
    descriptor.required_gles_major = 3u;
    descriptor.required_gles_minor = 0u;
    Check(
        ReverieNativeGlesRequirementSupportedV1(
            &descriptor,
            2u,
            0u
        ) == 0,
        "GLES 3.0 requirement must not fit GLES 2.0 host"
    );
    descriptor.required_gles_major = 0u;
    descriptor.required_gles_minor = 0u;
    Check(
        ReverieNativeGlesRequirementSupportedV1(&descriptor, 2u, 0u) == 0,
        "zero GLES major must not be accepted as a valid requirement"
    );
    descriptor.required_gles_major = 2u;
    Check(
        ReverieNativeGlesRequirementSupportedV1(&descriptor, 0u, 0u) == 0,
        "zero GLES host major must not be accepted as valid support"
    );
    descriptor.required_gles_major = 1u;
    descriptor.required_gles_minor = 1u;
    Check(
        ReverieNativeGlesRequirementSupportedV1(
            &descriptor,
            2u,
            0u
        ) != 0,
        "lower GLES major requirement should fit newer host"
    );

    ReverieNativeInputV1 input = {};
    input.struct_size =
        REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE;
    Check(
        ReverieNativeInputHasBaseV1(&input) != 0,
        "base input prefix should be readable"
    );
    input.delta_seconds = std::numeric_limits<float>::quiet_NaN();
    input.move_x = std::numeric_limits<float>::quiet_NaN();
    input.move_y = std::numeric_limits<float>::infinity();
    input.primary_down = 7u;
    input.secondary_down = 9u;
    Check(
        ReverieNativeSanitizeBaseInputV1(&input) != 0
        && input.delta_seconds == 0.0f
        && input.move_x == 0.0f
        && input.move_y == 0.0f
        && input.primary_down == 1u
        && input.secondary_down == 1u,
        "base sanitizer must neutralize non-finite samples and normalize buttons"
    );
    input.delta_seconds = 0.5f;
    input.move_x = 2.0f;
    input.move_y = -2.0f;
    Check(
        ReverieNativeSanitizeBaseInputV1(&input) != 0
        && input.delta_seconds == 0.1f
        && input.move_x == 1.0f
        && input.move_y == -1.0f,
        "base sanitizer must clamp finite axes and frame delta"
    );
    input.delta_seconds = -0.25f;
    input.move_x = -std::numeric_limits<float>::infinity();
    Check(
        ReverieNativeSanitizeBaseInputV1(&input) != 0
        && input.delta_seconds == 0.0f
        && input.move_x == 0.0f,
        "base sanitizer must reject negative time and infinite axis"
    );
    input.struct_size = REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE - 1u;
    input.move_x = std::numeric_limits<float>::quiet_NaN();
    Check(
        ReverieNativeSanitizeBaseInputV1(&input) == 0
        && std::isnan(input.move_x),
        "base sanitizer must not mutate truncated input prefix"
    );
    input.struct_size = REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE;
    Check(
        ReverieNativeSanitizeBaseInputV1(nullptr) == 0,
        "base sanitizer must reject null input"
    );
    Check(
        ReverieNativeInputHasPointerV1(&input) == 0,
        "base input prefix must not expose pointer tail"
    );
    input.struct_size =
        REVERIE_NATIVE_INPUT_V1_POINTER_MIN_SIZE;
    Check(
        ReverieNativeInputHasPointerV1(&input) != 0,
        "pointer input prefix should expose pointer fields"
    );

    input = {};
    input.struct_size = REVERIE_NATIVE_INPUT_V1_POINTER_MIN_SIZE;
    input.pointer_kind = REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    input.pointer_direction[0] = 3.0f;
    input.pointer_direction[1] = 4.0f;
    Check(
        ReverieNativeSanitizePointerV1(&input) != 0
        && std::abs(input.pointer_direction[0] - 0.6f) < 0.00001f,
        "pointer sanitizer must normalize ordinary finite rays"
    );
    input.pointer_kind = REVERIE_NATIVE_POINTER_VIRTUAL_CONTROLLER;
    input.pointer_direction[0] = 1.0e30f;
    input.pointer_direction[1] = 1.0e30f;
    Check(
        ReverieNativeSanitizePointerV1(&input) != 0
        && std::isfinite(input.pointer_direction[0])
        && std::abs(input.pointer_direction[0] - 0.70710678f) < 0.00001f,
        "pointer sanitizer must normalize huge finite rays without overflow"
    );
    input.pointer_kind = REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    input.pointer_direction[0] = std::numeric_limits<float>::quiet_NaN();
    Check(
        ReverieNativeSanitizePointerV1(&input) == 0
        && input.pointer_kind == REVERIE_NATIVE_POINTER_NONE,
        "pointer sanitizer must reject NaN direction"
    );
    input.pointer_kind = REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    input.pointer_direction[0] = std::numeric_limits<float>::infinity();
    Check(
        ReverieNativeSanitizePointerV1(&input) == 0,
        "pointer sanitizer must reject infinity"
    );
    input.pointer_kind = REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    input.pointer_direction[0] = 1.0f;
    input.pointer_origin[0] = 1001.0f;
    Check(
        ReverieNativeSanitizePointerV1(&input) == 0,
        "pointer sanitizer must reject out-of-range origin"
    );
    input.pointer_kind = REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    input.pointer_origin[0] = 0.0f;
    input.pointer_direction[0] = 1.0e-20f;
    Check(
        ReverieNativeSanitizePointerV1(&input) == 0,
        "pointer sanitizer must reject degenerate ray"
    );
    input.pointer_kind = 99u;
    input.pointer_direction[0] = 1.0f;
    Check(
        ReverieNativeSanitizePointerV1(&input) == 0,
        "pointer sanitizer must reject unknown kind"
    );
    input.pointer_kind = REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER;
    input.struct_size = REVERIE_NATIVE_INPUT_V1_POINTER_MIN_SIZE - 1u;
    Check(
        ReverieNativeSanitizePointerV1(&input) == 0
        && input.pointer_kind == REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER,
        "pointer sanitizer must not mutate truncated input prefix"
    );

    ReverieNativeEyeV1 eye = {};
    eye.struct_size =
        REVERIE_NATIVE_EYE_V1_MIN_SIZE;
    Check(
        ReverieNativeEyeHasMatricesV1(&eye) != 0,
        "eye matrix prefix should be readable"
    );
    Check(
        ReverieNativeEyeRenderableV1(&eye) != 0,
        "finite left-eye matrices should be renderable"
    );
    eye.eye_index = 1;
    Check(
        ReverieNativeEyeRenderableV1(&eye) != 0,
        "finite right-eye matrices should be renderable"
    );
    eye.eye_index = 2;
    Check(
        ReverieNativeEyeRenderableV1(&eye) == 0,
        "render guard must reject out-of-range eye index"
    );
    eye.eye_index = -1;
    Check(
        ReverieNativeEyeRenderableV1(&eye) == 0,
        "render guard must reject negative eye index"
    );
    eye.eye_index = 0;
    eye.view[7] = std::numeric_limits<float>::quiet_NaN();
    Check(
        ReverieNativeEyeRenderableV1(&eye) == 0,
        "render guard must reject NaN view matrix"
    );
    eye.view[7] = 0.0f;
    eye.projection[11] = std::numeric_limits<float>::infinity();
    Check(
        ReverieNativeEyeRenderableV1(&eye) == 0,
        "render guard must reject infinite projection matrix"
    );
    eye.projection[11] = 0.0f;
    eye.struct_size = REVERIE_NATIVE_EYE_V1_MIN_SIZE - 1u;
    Check(
        ReverieNativeEyeRenderableV1(&eye) == 0,
        "render guard must reject truncated eye prefix"
    );
    Check(
        ReverieNativeEyeRenderableV1(nullptr) == 0,
        "render guard must reject null eye"
    );

    ReverieNativeModuleCapabilitiesV1 capabilities = {};
    capabilities.struct_size =
        REVERIE_NATIVE_CAPABILITIES_V1_LOCOMOTION_MIN_SIZE;
    capabilities.flags =
        REVERIE_NATIVE_CAPABILITY_SHELL_LOCOMOTION;
    capabilities.locomotion_limit_x = 1.0f;
    capabilities.locomotion_limit_z = 2.0f;

    Check(
        ReverieNativeCapabilitiesHasShellLocomotionV1(
            &capabilities
        ) != 0,
        "valid locomotion capability should be admitted"
    );
    capabilities.flags = 0u;
    Check(
        ReverieNativeCapabilitiesHasShellLocomotionV1(
            &capabilities
        ) == 0,
        "locomotion bounds without flag must not opt in"
    );
    capabilities.flags =
        REVERIE_NATIVE_CAPABILITY_SHELL_LOCOMOTION;
    capabilities.locomotion_limit_x =
        std::numeric_limits<float>::infinity();
    Check(
        ReverieNativeCapabilitiesHasShellLocomotionV1(
            &capabilities
        ) == 0,
        "locomotion capability must reject non-finite X bounds"
    );
    capabilities.locomotion_limit_x = 1.0f;
    capabilities.locomotion_limit_z =
        std::numeric_limits<float>::infinity();
    Check(
        ReverieNativeCapabilitiesHasShellLocomotionV1(
            &capabilities
        ) == 0,
        "locomotion capability must reject non-finite Z bounds"
    );
    capabilities.locomotion_limit_z =
        std::numeric_limits<float>::quiet_NaN();
    Check(
        ReverieNativeCapabilitiesHasShellLocomotionV1(
            &capabilities
        ) == 0,
        "locomotion capability must reject NaN bounds"
    );
    capabilities.locomotion_limit_z = 2.0f;
    capabilities.locomotion_limit_x = 0.0f;
    Check(
        ReverieNativeCapabilitiesHasShellLocomotionV1(
            &capabilities
        ) == 0,
        "locomotion capability must reject zero bounds"
    );
    capabilities.locomotion_limit_x = 1.0f;
    capabilities.struct_size =
        REVERIE_NATIVE_CAPABILITIES_V1_LOCOMOTION_MIN_SIZE - 1u;
    Check(
        ReverieNativeCapabilitiesHasShellLocomotionV1(
            &capabilities
        ) == 0,
        "locomotion capability must reject truncated prefix"
    );
    capabilities.struct_size =
        REVERIE_NATIVE_CAPABILITIES_V1_LOCOMOTION_MIN_SIZE;

    ReverieNativeModuleApiV1 api = {};
    api.abi_version =
        REVERIE_NATIVE_MODULE_ABI_VERSION;
    api.capabilities = &capabilities;

    api.struct_size =
        REVERIE_NATIVE_MODULE_API_V1_MIN_SIZE;
    Check(
        ReverieNativeApiHasMandatoryV1(&api) != 0,
        "old mandatory v1 API prefix must remain valid"
    );
    Check(
        ReverieNativeApiCapabilitiesV1(&api) == nullptr,
        "old v1 API prefix must not expose unseen capability tail"
    );

    api.struct_size =
        REVERIE_NATIVE_MODULE_API_V1_CAPABILITIES_MIN_SIZE;
    Check(
        ReverieNativeApiCapabilitiesV1(&api)
            == &capabilities,
        "capability-aware v1 API should expose capability block"
    );

    api.abi_version =
        REVERIE_NATIVE_MODULE_ABI_VERSION + 1u;
    Check(
        ReverieNativeApiHasMandatoryV1(&api) == 0,
        "different ABI version must fail mandatory v1 helper"
    );

    if (failures != 0) {
        std::fprintf(
            stderr,
            "NATIVE ABI CONTRACT: FAIL (%d)\n",
            failures
        );
        return 1;
    }

    std::puts("NATIVE ABI CONTRACT: PASS");
    return 0;
}
