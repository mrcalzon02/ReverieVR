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

static inline int ReverieNativeDescriptorHasMandatoryV1(
    const ReverieNativeModuleDescriptorV1 *descriptor
) {
    return descriptor != NULL
        && descriptor->abi_version
            == REVERIE_NATIVE_MODULE_ABI_VERSION
        && descriptor->struct_size
            == REVERIE_NATIVE_DESCRIPTOR_V1_SIZE;
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

#ifdef __cplusplus
}
#endif

#endif
