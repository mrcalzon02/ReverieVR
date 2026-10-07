#ifndef REVERIE_NATIVE_MODULE_H
#define REVERIE_NATIVE_MODULE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define REVERIE_NATIVE_MODULE_ABI_VERSION 1u
#define REVERIE_NATIVE_MODULE_ENTRY_SYMBOL "reverie_native_module_entry_v1"
#define REVERIE_NATIVE_SAVE_MAX_BYTES 65536u

enum ReverieNativeLogLevel {
    REVERIE_NATIVE_LOG_DEBUG = 0,
    REVERIE_NATIVE_LOG_INFO = 1,
    REVERIE_NATIVE_LOG_WARN = 2,
    REVERIE_NATIVE_LOG_ERROR = 3
};

enum ReverieNativePointerKind {
    REVERIE_NATIVE_POINTER_NONE = 0,
    REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER = 1,
    REVERIE_NATIVE_POINTER_VIRTUAL_CONTROLLER = 2
};

enum ReverieNativeSaveResult {
    REVERIE_NATIVE_SAVE_ERROR = -1,
    REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL = -2,
    REVERIE_NATIVE_SAVE_NOT_FOUND = 0,
    REVERIE_NATIVE_SAVE_OK = 1
};

enum ReverieNativeFeedbackFlagV1 {
    REVERIE_NATIVE_FEEDBACK_FOCUS = 1u << 0,
    REVERIE_NATIVE_FEEDBACK_ACTIVATION = 1u << 1,
    REVERIE_NATIVE_FEEDBACK_FAILURE = 1u << 2,
    REVERIE_NATIVE_FEEDBACK_ALL =
        REVERIE_NATIVE_FEEDBACK_FOCUS
        | REVERIE_NATIVE_FEEDBACK_ACTIVATION
        | REVERIE_NATIVE_FEEDBACK_FAILURE
};

typedef struct ReverieNativeHostV1 {
    uint32_t struct_size;
    uint32_t abi_version;
    void (*log)(
        int32_t level,
        const char *tag,
        const char *message
    );

    /*
     * Append-only ABI v1 service extension. Save slots are scoped by the host
     * to the active packaged module. Slot names must be simple filenames; the
     * module never receives an arbitrary filesystem path.
     */
    int32_t (*read_save)(
        const char *slot,
        void *buffer,
        uint32_t capacity,
        uint32_t *out_size
    );
    int32_t (*write_save)(
        const char *slot,
        const void *data,
        uint32_t size
    );

    /*
     * Optional shell-owned cue request. Requests are coalesced by flag until
     * the host drains them after the current native update.
     */
    void (*request_feedback)(
        uint32_t flags
    );
} ReverieNativeHostV1;

#define REVERIE_NATIVE_HOST_V1_LOG_MIN_SIZE \
    ((uint32_t)(offsetof(ReverieNativeHostV1, log) \
        + sizeof(((ReverieNativeHostV1 *)0)->log)))
#define REVERIE_NATIVE_HOST_V1_SAVE_MIN_SIZE \
    ((uint32_t)(offsetof(ReverieNativeHostV1, write_save) \
        + sizeof(((ReverieNativeHostV1 *)0)->write_save)))
#define REVERIE_NATIVE_HOST_V1_FEEDBACK_MIN_SIZE \
    ((uint32_t)(offsetof(ReverieNativeHostV1, request_feedback) \
        + sizeof(((ReverieNativeHostV1 *)0)->request_feedback)))

typedef struct ReverieNativeModuleDescriptorV1 {
    uint32_t struct_size;
    uint32_t abi_version;
    const char *id;
    const char *display_name;
    uint32_t required_gles_major;
    uint32_t required_gles_minor;
} ReverieNativeModuleDescriptorV1;

/*
 * ABI v1 layout freeze: this descriptor is embedded by value in
 * ReverieNativeModuleApiV1 before callback pointers. Appending fields here
 * would shift the callback offsets and therefore requires a new ABI version.
 */
#define REVERIE_NATIVE_DESCRIPTOR_V1_SIZE \
    ((uint32_t)sizeof(ReverieNativeModuleDescriptorV1))
#define REVERIE_NATIVE_DESCRIPTOR_V1_MIN_SIZE \
    REVERIE_NATIVE_DESCRIPTOR_V1_SIZE

typedef struct ReverieNativeInputV1 {
    uint32_t struct_size;
    float delta_seconds;
    float move_x;
    float move_y;
    uint32_t primary_down;
    uint32_t secondary_down;

    /*
     * Append-only ABI v1 pointer extension. The host owns controller
     * calibration and supplies a normalized world-space ray. Modules do not
     * consume raw Android controller quaternions.
     */
    uint32_t pointer_kind;
    float pointer_origin[3];
    float pointer_direction[3];
} ReverieNativeInputV1;

#define REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE \
    ((uint32_t)(offsetof(ReverieNativeInputV1, secondary_down) \
        + sizeof(((ReverieNativeInputV1 *)0)->secondary_down)))
#define REVERIE_NATIVE_INPUT_V1_POINTER_MIN_SIZE \
    ((uint32_t)(offsetof(ReverieNativeInputV1, pointer_direction) \
        + sizeof(((ReverieNativeInputV1 *)0)->pointer_direction)))

typedef struct ReverieNativeEyeV1 {
    uint32_t struct_size;
    int32_t eye_index;
    float view[16];
    float projection[16];
} ReverieNativeEyeV1;

#define REVERIE_NATIVE_EYE_V1_MIN_SIZE \
    ((uint32_t)(offsetof(ReverieNativeEyeV1, projection) \
        + sizeof(((ReverieNativeEyeV1 *)0)->projection)))

enum ReverieNativeModuleCapabilityFlagV1 {
    REVERIE_NATIVE_CAPABILITY_SHELL_LOCOMOTION = 1u << 0
};

typedef struct ReverieNativeModuleCapabilitiesV1 {
    uint32_t struct_size;
    uint32_t flags;

    /*
     * Horizontal shell-owned travel envelope in meters when
     * REVERIE_NATIVE_CAPABILITY_SHELL_LOCOMOTION is set.
     */
    float locomotion_limit_x;
    float locomotion_limit_z;
} ReverieNativeModuleCapabilitiesV1;

#define REVERIE_NATIVE_CAPABILITIES_V1_LOCOMOTION_MIN_SIZE \
    ((uint32_t)(offsetof(ReverieNativeModuleCapabilitiesV1, locomotion_limit_z) \
        + sizeof(((ReverieNativeModuleCapabilitiesV1 *)0)->locomotion_limit_z)))

typedef struct ReverieNativeModuleApiV1 {
    uint32_t struct_size;
    uint32_t abi_version;
    ReverieNativeModuleDescriptorV1 descriptor;

    void *(*create)(
        const ReverieNativeHostV1 *host
    );
    void (*destroy)(void *instance);

    int32_t (*on_gl_context_created)(
        void *instance
    );
    void (*release_gl_context)(
        void *instance
    );

    void (*resume)(void *instance);
    void (*pause)(void *instance);

    void (*update)(
        void *instance,
        const ReverieNativeInputV1 *input
    );

    int32_t (*render_eye)(
        void *instance,
        const ReverieNativeEyeV1 *eye
    );

    /*
     * Optional ABI-v1 tail extension. Hosts must prove this field exists
     * from struct_size before reading it. NULL means no declared capabilities.
     */
    const ReverieNativeModuleCapabilitiesV1 *capabilities;
} ReverieNativeModuleApiV1;

#define REVERIE_NATIVE_MODULE_API_V1_MIN_SIZE \
    ((uint32_t)(offsetof(ReverieNativeModuleApiV1, render_eye) \
        + sizeof(((ReverieNativeModuleApiV1 *)0)->render_eye)))
#define REVERIE_NATIVE_MODULE_API_V1_CAPABILITIES_MIN_SIZE \
    ((uint32_t)(offsetof(ReverieNativeModuleApiV1, capabilities) \
        + sizeof(((ReverieNativeModuleApiV1 *)0)->capabilities)))

typedef const ReverieNativeModuleApiV1 *
    (*ReverieNativeModuleEntryV1)(void);

#ifdef __cplusplus
}
#endif

#endif
