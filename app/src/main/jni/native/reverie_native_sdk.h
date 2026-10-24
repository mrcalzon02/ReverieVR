#ifndef REVERIE_NATIVE_SDK_H
#define REVERIE_NATIVE_SDK_H

#include "reverie_native_module.h"

#include <float.h>
#include <math.h>

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

static inline int ReverieNativeHostSupportsSoundV1(
    const ReverieNativeHostV1 *host
) {
    return host != NULL
        && host->abi_version == REVERIE_NATIVE_MODULE_ABI_VERSION
        && host->struct_size >= REVERIE_NATIVE_HOST_V1_SOUND_MIN_SIZE
        && host->request_sound != NULL;
}

// Sound cue requests carry exactly one logical event, not coalesced flags.
// Host-owned fallback must be used when this optional tail is absent.
static inline int ReverieNativeRequestSoundV1(
    const ReverieNativeHostV1 *host, uint32_t cue_id
) {
    if (!ReverieNativeHostSupportsSoundV1(host)
        || cue_id < REVERIE_NATIVE_SOUND_WEAPON_MACHINE
        || cue_id > REVERIE_NATIVE_SOUND_MAX_V1) return 0;
    host->request_sound(cue_id);
    return 1;
}


/** Validate every index, including fields sourced from older saved avatars. */
static inline int ReverieNativeAvatarValidV1(
    const ReverieNativeAvatarV1 *avatar
) {
    static const uint32_t counts[REVERIE_NATIVE_AVATAR_FIELD_COUNT_V1] = {
        48u,16u,16u,16u,4u,3u,12u,7u,7u,7u,7u,7u,24u,12u,16u
    };
    if (avatar == NULL
        || avatar->struct_size != sizeof(ReverieNativeAvatarV1)
        || avatar->schema_version != REVERIE_NATIVE_AVATAR_SCHEMA_V1)
        return 0;
    for(uint32_t i=0u;i<REVERIE_NATIVE_AVATAR_FIELD_COUNT_V1;i++)
        if(avatar->fields[i]>=counts[i])return 0;
    return 1;
}

/** Fixed nominal VR player body: a stretched 3D sphere (ellipsoid).
 * Physical collision is independent of cosmetic height, shape and costume.
 * The caller places feet at groundY; body center is groundY + 0.85 m.
 */
typedef struct ReverieNativeBodyEllipsoidV1 {
    float center_x,center_y,center_z;
    float radius_x,radius_y,radius_z;
} ReverieNativeBodyEllipsoidV1;

static inline ReverieNativeBodyEllipsoidV1
ReverieNativeAvatarCollisionV1(float player_x,float ground_y,float player_z) {
    ReverieNativeBodyEllipsoidV1 shape = {
        player_x, ground_y + .85f, player_z, .27f, .85f, .27f
    };
    return shape;
}

static inline int ReverieNativeEllipsoidContainsV1(
    const ReverieNativeBodyEllipsoidV1 *body,
    float x,float y,float z
) {
    if(body==NULL || !(body->radius_x>0.0f)
        || !(body->radius_y>0.0f) || !(body->radius_z>0.0f)
        || !(fabsf(body->center_x)<=FLT_MAX)
        || !(fabsf(body->center_y)<=FLT_MAX)
        || !(fabsf(body->center_z)<=FLT_MAX)
        || !(fabsf(body->radius_x)<=FLT_MAX)
        || !(fabsf(body->radius_y)<=FLT_MAX)
        || !(fabsf(body->radius_z)<=FLT_MAX)
        || !(fabsf(x)<=FLT_MAX) || !(fabsf(y)<=FLT_MAX)
        || !(fabsf(z)<=FLT_MAX))return 0;
    const float dx=(x-body->center_x)/body->radius_x;
    const float dy=(y-body->center_y)/body->radius_y;
    const float dz=(z-body->center_z)/body->radius_z;
    return dx*dx+dy*dy+dz*dz<=1.0f;
}

static inline int ReverieNativeHostSupportsAvatarV1(
    const ReverieNativeHostV1 *host
) {
    return host!=NULL
        && host->abi_version==REVERIE_NATIVE_MODULE_ABI_VERSION
        && host->struct_size>=REVERIE_NATIVE_HOST_V1_AVATAR_MIN_SIZE
        && host->read_avatar!=NULL;
}
static inline int ReverieNativeReadAvatarV1(
    const ReverieNativeHostV1 *host,ReverieNativeAvatarV1 *out
) {
    if(out==NULL||!ReverieNativeHostSupportsAvatarV1(host))return 0;
    ReverieNativeAvatarV1 tmp = {0};
    if(host->read_avatar(&tmp,(uint32_t)sizeof(tmp))!=1
        || !ReverieNativeAvatarValidV1(&tmp))return 0;
    *out=tmp;
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
        ) || descriptor->required_gles_major == 0u
          || host_major == 0u) {
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

/* Normalize mandatory base input without touching the pointer tail.
 * Non-finite samples fail neutral rather than saturating an axis. */
static inline float reverie_native_clamp_finite(
    float value, float minimum, float maximum
) {
    if (!(fabsf(value) <= FLT_MAX)) return 0.0f;
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static inline int ReverieNativeSanitizeBaseInputV1(
    ReverieNativeInputV1 *input
) {
    if (!ReverieNativeInputHasBaseV1(input)) return 0;
    input->delta_seconds = reverie_native_clamp_finite(
        input->delta_seconds, 0.0f, 0.1f
    );
    input->move_x = reverie_native_clamp_finite(
        input->move_x, -1.0f, 1.0f
    );
    input->move_y = reverie_native_clamp_finite(
        input->move_y, -1.0f, 1.0f
    );
    input->primary_down = input->primary_down != 0u ? 1u : 0u;
    input->secondary_down = input->secondary_down != 0u ? 1u : 0u;
    return 1;
}

static inline int ReverieNativeInputHasPointerV1(
    const ReverieNativeInputV1 *input
) {
    return input != NULL
        && input->struct_size
            >= REVERIE_NATIVE_INPUT_V1_POINTER_MIN_SIZE;
}

/*
 * Normalize a writable pointer ray without squared-length overflow.
 * Invalid full-prefix rays are cleared; truncated prefixes stay untouched.
 */
static inline int ReverieNativeSanitizePointerV1(
    ReverieNativeInputV1 *input
) {
    if (!ReverieNativeInputHasPointerV1(input)) {
        return 0;
    }

    const int known_kind =
        input->pointer_kind == REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER
        || input->pointer_kind == REVERIE_NATIVE_POINTER_VIRTUAL_CONTROLLER;
    const float abs_x = fabsf(input->pointer_direction[0]);
    const float abs_y = fabsf(input->pointer_direction[1]);
    const float abs_z = fabsf(input->pointer_direction[2]);
    float largest = abs_x;
    if (abs_y > largest) largest = abs_y;
    if (abs_z > largest) largest = abs_z;

    if (known_kind
        && fabsf(input->pointer_origin[0]) <= 1000.0f
        && fabsf(input->pointer_origin[1]) <= 1000.0f
        && fabsf(input->pointer_origin[2]) <= 1000.0f
        && abs_x <= FLT_MAX
        && abs_y <= FLT_MAX
        && abs_z <= FLT_MAX
        && largest > 0.0f) {
        const float x = input->pointer_direction[0] / largest;
        const float y = input->pointer_direction[1] / largest;
        const float z = input->pointer_direction[2] / largest;
        const float length = sqrtf(x * x + y * y + z * z);
        if (largest > 0.001f / length) {
            input->pointer_direction[0] = x / length;
            input->pointer_direction[1] = y / length;
            input->pointer_direction[2] = z / length;
            return 1;
        }
    }

    input->pointer_kind = REVERIE_NATIVE_POINTER_NONE;
    for (int axis = 0; axis < 3; ++axis) {
        input->pointer_origin[axis] = 0.0f;
        input->pointer_direction[axis] = 0.0f;
    }
    return 0;
}

static inline int ReverieNativeEyeHasMatricesV1(
    const ReverieNativeEyeV1 *eye
) {
    return eye != NULL
        && eye->struct_size
            >= REVERIE_NATIVE_EYE_V1_MIN_SIZE;
}

/* Render-boundary validation: never rewrite Cardboard optics. */
static inline int ReverieNativeEyeRenderableV1(
    const ReverieNativeEyeV1 *eye
) {
    if (!ReverieNativeEyeHasMatricesV1(eye)
        || (eye->eye_index != 0 && eye->eye_index != 1)) {
        return 0;
    }
    for (int i = 0; i < 16; ++i) {
        if (!(fabsf(eye->view[i]) <= FLT_MAX)
            || !(fabsf(eye->projection[i]) <= FLT_MAX)) {
            return 0;
        }
    }
    return 1;
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
        && capabilities->locomotion_limit_x <= FLT_MAX
        && capabilities->locomotion_limit_z > 0.0f
        && capabilities->locomotion_limit_z <= FLT_MAX;
}

#ifdef __cplusplus
}
#endif

#endif
