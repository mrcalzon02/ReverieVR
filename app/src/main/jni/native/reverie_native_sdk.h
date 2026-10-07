#ifndef REVERIE_NATIVE_SDK_H
#define REVERIE_NATIVE_SDK_H

#include "reverie_native_module.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Header-only helpers for consuming ABI v1 safely.
 *
 * These helpers do not add ABI fields. They encode the current prefix and
 * capability checks so first-party modules do not duplicate size/version
 * logic or accidentally regress to newest-struct sizeof validation.
 */

static inline int ReverieNativeHostSupportsLogV1(
    const ReverieNativeHostV1 *host
) {
    return host != NULL
        && host->abi_version
            == REVERIE_NATIVE_MODULE_ABI_VERSION
        && host->struct_size
            >= REVERIE_NATIVE_HOST_V1_LOG_MIN_SIZE
        && host->log != NULL;
}

static inline int ReverieNativeLogV1(
    const ReverieNativeHostV1 *host,
    int32_t level,
    const char *tag,
    const char *message
) {
    if (!ReverieNativeHostSupportsLogV1(
            host
        )) {
        return 0;
    }

    host->log(
        level,
        tag != NULL
            ? tag
            : "ReverieNativeModule",
        message != NULL
            ? message
            : ""
    );
    return 1;
}

static inline int ReverieNativeHostSupportsSaveV1(
    const ReverieNativeHostV1 *host
) {
    return host != NULL
        && host->abi_version
            == REVERIE_NATIVE_MODULE_ABI_VERSION
        && host->struct_size
            >= REVERIE_NATIVE_HOST_V1_SAVE_MIN_SIZE
        && host->read_save != NULL
        && host->write_save != NULL;
}

static inline int ReverieNativeHostSupportsFeedbackV1(
    const ReverieNativeHostV1 *host
) {
    return host != NULL
        && host->abi_version
            == REVERIE_NATIVE_MODULE_ABI_VERSION
        && host->struct_size
            >= REVERIE_NATIVE_HOST_V1_FEEDBACK_MIN_SIZE
        && host->request_feedback != NULL;
}

static inline int ReverieNativeRequestFeedbackV1(
    const ReverieNativeHostV1 *host,
    uint32_t flags
) {
    if (!ReverieNativeHostSupportsFeedbackV1(
            host
        )) {
        return 0;
    }

    const uint32_t safe_flags =
        flags
        & (uint32_t)REVERIE_NATIVE_FEEDBACK_ALL;
    if (safe_flags == 0u) {
        return 0;
    }

    host->request_feedback(
        safe_flags
    );
    return 1;
}

static inline int ReverieNativeDescriptorHasMandatoryV1(
    const ReverieNativeModuleDescriptorV1 *descriptor
) {
    return descriptor != NULL
        && descriptor->abi_version
            == REVERIE_NATIVE_MODULE_ABI_VERSION
        && descriptor->struct_size
            == REVERIE_NATIVE_DESCRIPTOR_V1_SIZE;
}

static inline int ReverieNativeGlesRequirementSupportedV1(
    const ReverieNativeModuleDescriptorV1 *descriptor,
    uint32_t host_major,
    uint32_t host_minor
) {
    if (!ReverieNativeDescriptorHasMandatoryV1(
            descriptor
        )) {
        return 0;
    }

    if (descriptor->required_gles_major
        < host_major) {
        return 1;
    }
    if (descriptor->required_gles_major
        > host_major) {
        return 0;
    }
    return descriptor->required_gles_minor
        <= host_minor;
}

static inline int ReverieNativeInputHasBaseV1(
    const ReverieNativeInputV1 *input
) {
    return input != NULL
        && input->struct_size
            >= REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE;
}

static inline int ReverieNativeInputHasPointerV1(
    const ReverieNativeInputV1 *input
) {
    return input != NULL
        && input->struct_size
            >= REVERIE_NATIVE_INPUT_V1_POINTER_MIN_SIZE;
}

static inline int ReverieNativeEyeHasMatricesV1(
    const ReverieNativeEyeV1 *eye
) {
    return eye != NULL
        && eye->struct_size
            >= REVERIE_NATIVE_EYE_V1_MIN_SIZE;
}

static inline int ReverieNativeApiHasMandatoryV1(
    const ReverieNativeModuleApiV1 *api
) {
    return api != NULL
        && api->abi_version
            == REVERIE_NATIVE_MODULE_ABI_VERSION
        && api->struct_size
            >= REVERIE_NATIVE_MODULE_API_V1_MIN_SIZE;
}

static inline const ReverieNativeModuleCapabilitiesV1 *
ReverieNativeApiCapabilitiesV1(
    const ReverieNativeModuleApiV1 *api
) {
    if (!ReverieNativeApiHasMandatoryV1(api)
        || api->struct_size
            < REVERIE_NATIVE_MODULE_API_V1_CAPABILITIES_MIN_SIZE) {
        return NULL;
    }
    return api->capabilities;
}

static inline int ReverieNativeCapabilitiesHasShellLocomotionV1(
    const ReverieNativeModuleCapabilitiesV1 *capabilities
) {
    return capabilities != NULL
        && capabilities->struct_size
            >= REVERIE_NATIVE_CAPABILITIES_V1_LOCOMOTION_MIN_SIZE
        && (
            capabilities->flags
            & REVERIE_NATIVE_CAPABILITY_SHELL_LOCOMOTION
        ) != 0u
        && capabilities->locomotion_limit_x > 0.0f
        && capabilities->locomotion_limit_z > 0.0f;
}

#ifdef __cplusplus
}
#endif

#endif
