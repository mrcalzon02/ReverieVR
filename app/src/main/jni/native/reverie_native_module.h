#ifndef REVERIE_NATIVE_MODULE_H
#define REVERIE_NATIVE_MODULE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define REVERIE_NATIVE_MODULE_ABI_VERSION 1u
#define REVERIE_NATIVE_MODULE_ENTRY_SYMBOL "reverie_native_module_entry_v1"

enum ReverieNativeLogLevel {
    REVERIE_NATIVE_LOG_DEBUG = 0,
    REVERIE_NATIVE_LOG_INFO = 1,
    REVERIE_NATIVE_LOG_WARN = 2,
    REVERIE_NATIVE_LOG_ERROR = 3
};

typedef struct ReverieNativeHostV1 {
    uint32_t struct_size;
    uint32_t abi_version;
    void (*log)(
        int32_t level,
        const char *tag,
        const char *message
    );
} ReverieNativeHostV1;

typedef struct ReverieNativeModuleDescriptorV1 {
    uint32_t struct_size;
    uint32_t abi_version;
    const char *id;
    const char *display_name;
    uint32_t required_gles_major;
    uint32_t required_gles_minor;
} ReverieNativeModuleDescriptorV1;

typedef struct ReverieNativeInputV1 {
    uint32_t struct_size;
    float delta_seconds;
    float move_x;
    float move_y;
    uint32_t primary_down;
    uint32_t secondary_down;
} ReverieNativeInputV1;

typedef struct ReverieNativeEyeV1 {
    uint32_t struct_size;
    int32_t eye_index;
    float view[16];
    float projection[16];
} ReverieNativeEyeV1;

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
} ReverieNativeModuleApiV1;

typedef const ReverieNativeModuleApiV1 *
    (*ReverieNativeModuleEntryV1)(void);

#ifdef __cplusplus
}
#endif

#endif
