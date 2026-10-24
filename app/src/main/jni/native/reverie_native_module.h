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
    REVERIE_NATIVE_FEEDBACK_WEAPON_FIRE = 1u << 3,
    REVERIE_NATIVE_FEEDBACK_RELOAD_START = 1u << 4,
    REVERIE_NATIVE_FEEDBACK_RELOAD_READY = 1u << 5,
    REVERIE_NATIVE_FEEDBACK_ARTILLERY = 1u << 6,
    REVERIE_NATIVE_FEEDBACK_AIRSTRIKE = 1u << 7,
    REVERIE_NATIVE_FEEDBACK_ALL =
        REVERIE_NATIVE_FEEDBACK_FOCUS
        | REVERIE_NATIVE_FEEDBACK_ACTIVATION
        | REVERIE_NATIVE_FEEDBACK_FAILURE
        | REVERIE_NATIVE_FEEDBACK_WEAPON_FIRE
        | REVERIE_NATIVE_FEEDBACK_RELOAD_START
        | REVERIE_NATIVE_FEEDBACK_RELOAD_READY
        | REVERIE_NATIVE_FEEDBACK_ARTILLERY
        | REVERIE_NATIVE_FEEDBACK_AIRSTRIKE
};

// Optional ordered one-shot sound events (unlike coalesced UI flags).
// IDs are shared shell sounds, not module-provided PCM or file paths.
enum ReverieNativeSoundCueV1 {
    REVERIE_NATIVE_SOUND_WEAPON_MACHINE = 1u,
    REVERIE_NATIVE_SOUND_WEAPON_HEAVY = 2u,
    REVERIE_NATIVE_SOUND_WEAPON_FLAK = 3u,
    REVERIE_NATIVE_SOUND_WEAPON_MISSILE = 4u,
    REVERIE_NATIVE_SOUND_WEAPON_CANNON = 5u,
    // Added optional ordered one-shot effects. Hosts with only the original
    // five sound IDs can continue falling back to existing feedback.
    REVERIE_NATIVE_SOUND_HULL_IMPACT = 6u,
    REVERIE_NATIVE_SOUND_WATER_SPLASH = 7u,
    REVERIE_NATIVE_SOUND_BOAT_DESTROYED = 8u,
    REVERIE_NATIVE_SOUND_GUNBOAT_SALVO = 9u,
    REVERIE_NATIVE_SOUND_INFANTRY_VOLLEY = 10u,
    REVERIE_NATIVE_SOUND_AIRCRAFT_BOMB = 11u,
    REVERIE_NATIVE_SOUND_AIRCRAFT_STRAFE = 12u,
    REVERIE_NATIVE_SOUND_SAM_LAUNCH = 13u,
    REVERIE_NATIVE_SOUND_WAVE_START = 14u,
    REVERIE_NATIVE_SOUND_SHORE_COLLISION = 15u,
    REVERIE_NATIVE_SOUND_SHOP_PURCHASE = 16u,
    REVERIE_NATIVE_SOUND_WAVE_CLEAR = 17u,
    REVERIE_NATIVE_SOUND_CAMPAIGN_VICTORY = 18u,
    REVERIE_NATIVE_SOUND_BATTERY_LOST = 19u,
    REVERIE_NATIVE_SOUND_ARMORED_GUN = 20u,
    REVERIE_NATIVE_SOUND_ARMOR_DESTROYED = 21u,
    // Original owner-exported Red Ledger SoundWave effects.
    REVERIE_NATIVE_SOUND_RED_LEDGER_CLICK = 22u,
    REVERIE_NATIVE_SOUND_RED_LEDGER_METAL = 23u,
    REVERIE_NATIVE_SOUND_RED_LEDGER_COINS = 24u,
    REVERIE_NATIVE_SOUND_RED_LEDGER_BOOK = 25u,
    REVERIE_NATIVE_SOUND_RED_LEDGER_DOOR = 26u,
    REVERIE_NATIVE_SOUND_RED_LEDGER_DRAWER = 27u,
    REVERIE_NATIVE_SOUND_MAX_V1 = REVERIE_NATIVE_SOUND_RED_LEDGER_DRAWER
};


/*
 * Shell-wide, read-only avatar identity snapshot. This is the 15-field
 * materialized projection of HomeAvatarProfile v2, NOT a module save.
 * One profile belongs to the user regardless of which native game loads.
 */
#define REVERIE_NATIVE_AVATAR_SCHEMA_V1 1u
#define REVERIE_NATIVE_AVATAR_FIELD_COUNT_V1 15u
enum ReverieNativeAvatarFieldV1 {
    REVERIE_NATIVE_AVATAR_THEME=0,
    REVERIE_NATIVE_AVATAR_ACCENT_COLOR=1,
    REVERIE_NATIVE_AVATAR_HAT=2,
    REVERIE_NATIVE_AVATAR_ACCESSORY=3,
    REVERIE_NATIVE_AVATAR_EYES=4,
    REVERIE_NATIVE_AVATAR_PRESENTATION=5,
    REVERIE_NATIVE_AVATAR_PHYSIQUE=6,
    REVERIE_NATIVE_AVATAR_HEIGHT=7,
    REVERIE_NATIVE_AVATAR_SHOULDERS=8,
    REVERIE_NATIVE_AVATAR_WAIST=9,
    REVERIE_NATIVE_AVATAR_HIPS=10,
    REVERIE_NATIVE_AVATAR_MUSCLE=11,
    REVERIE_NATIVE_AVATAR_OUTFIT=12,
    REVERIE_NATIVE_AVATAR_HAIR=13,
    REVERIE_NATIVE_AVATAR_FABRIC=14
};
typedef struct ReverieNativeAvatarV1 {
    uint32_t struct_size;
    uint32_t schema_version;
    uint32_t fields[REVERIE_NATIVE_AVATAR_FIELD_COUNT_V1];
} ReverieNativeAvatarV1;

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

    // Optional ABI-v1 append-only tail. Each valid request is one ordered
    // event, not a bitmask; old modules/hosts keep using request_feedback.
    // The shell bounds per-frame queue capacity and owns all audio data.
    void (*request_sound)(
        uint32_t cue_id
    );

    /* Optional append-only asset reader: packaged, read-only and scoped.
       Asset bytes are never loaded from untrusted module save paths. */
    int32_t (*read_asset)(
        const char *asset_path,
        void *buffer,
        uint32_t capacity,
        uint32_t *out_size
    );
    /*
     * Optional ABI-v1 append-only global profile snapshot. Games may copy
     * it during create/update; no writes or access to another module's save.
     * Returns 1 on success, 0 without a profile, negative for bad buffers.
     */
    int32_t (*read_avatar)(
        ReverieNativeAvatarV1 *out_avatar,
        uint32_t capacity
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
#define REVERIE_NATIVE_HOST_V1_SOUND_MIN_SIZE \
    ((uint32_t)(offsetof(ReverieNativeHostV1, request_sound) \
        + sizeof(((ReverieNativeHostV1 *)0)->request_sound)))
#define REVERIE_NATIVE_HOST_V1_ASSET_MIN_SIZE \
    ((uint32_t)(offsetof(ReverieNativeHostV1, read_asset) \
        + sizeof(((ReverieNativeHostV1 *)0)->read_asset)))
#define REVERIE_NATIVE_HOST_V1_AVATAR_MIN_SIZE \
    ((uint32_t)(offsetof(ReverieNativeHostV1, read_avatar) \
        + sizeof(((ReverieNativeHostV1 *)0)->read_avatar)))

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
