#include "reverie_native_sdk.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <cmath>
#include <cstdio>

namespace {

int failures = 0;
int captured_log_count = 0;
uint32_t captured_feedback_flags = 0u;

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
