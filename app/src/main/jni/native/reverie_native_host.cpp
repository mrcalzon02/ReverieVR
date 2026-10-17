#include "reverie_native_sdk.h"
#include "reverie_native_sound_queue.h"

#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <GLES2/gl2.h>
#include <dlfcn.h>
#include <jni.h>

#include <algorithm>
#include <cstdint>
#include <atomic>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <mutex>
#include <new>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace {

struct BuiltInModuleSpec {
    const char *id;
    const char *display_name;
    const char *library_name;
};

static const BuiltInModuleSpec kBuiltIns[] = {
    {
        "procedural-test-chamber",
        "Procedural Test Chamber",
        "libreverie_module_test_chamber.so"
    },
    {
        "between-deliveries-red-ledger",
        "Between Deliveries: The Red Ledger VR [DEV]",
        "libreverie_module_red_ledger.so"
    },
    {
        "breakwater-battery",
        "Breakwater Battery VR [DEV]",
        "libreverie_module_breakwater.so"
    },
    {"gumball-armada", "Gumball Armada: Candy King Search [DEV]",
     "libreverie_module_gumball_armada.so"},
    {"omega-front", "Omega Front: Hearthwell Defense [DEV]",
     "libreverie_module_omega_front.so"},
    {"ministry-lantern-desk", "Ministry of Intelligence: Lantern Desk [DEV]",
     "libreverie_module_lantern_desk.so"},
    {"iron-sight-forward-detachment", "Iron Sight: Forward Detachment [DEV]",
     "libreverie_module_forward_detachment.so"},
    {"classic-pong", "Classic Pong", "libreverie_module_classic_pong.so"},
    {"classic-snake", "Classic Snake", "libreverie_module_classic_snake.so"},
    {"classic-chutes-ladders", "Chutes and Ladders",
     "libreverie_module_classic_chutes_ladders.so"},
    {"classic-checkers", "Classic Checkers",
     "libreverie_module_classic_checkers.so"},
    {"classic-go", "Go 19x19", "libreverie_module_classic_go.so"},
    {"classic-chess", "Classic Chess",
     "libreverie_module_classic_chess.so"},
    {"classic-breakout", "Classic Breakout",
     "libreverie_module_classic_breakout.so"},
    {"classic-reversi", "Classic Reversi",
     "libreverie_module_classic_reversi.so"}
};

struct NativeSession {
    const BuiltInModuleSpec *spec = nullptr;
    void *library = nullptr;
    const ReverieNativeModuleApiV1 *api = nullptr;
    void *instance = nullptr;
    std::string storage_root;
    AAssetManager *asset_manager = nullptr;
    bool gl_ready = false;
    bool warned_inherited_gl_error = false;
    // Stored only for a failed GL creation callback, not a global log sink.
    std::string last_guest_error;
    std::atomic<uint32_t> pending_feedback_flags{0u};
    // Ordered 8-bit cue IDs, bounded to eight per JNI update/drain.
    // A mutex protects callbacks if guest work eventually leaves the GL
    // thread. No per-event memory allocation and no per-eye requests.
    std::mutex sound_mutex;
    ReverieNativeSoundQueueV1 pending_sounds;
};

std::mutex g_error_mutex;
std::string g_last_error;
thread_local NativeSession *g_callback_session = nullptr;

class CallbackSessionScope {
public:
    explicit CallbackSessionScope(
        NativeSession *session
    )
        : previous_(g_callback_session) {
        g_callback_session = session;
    }

    ~CallbackSessionScope() {
        g_callback_session = previous_;
    }

private:
    NativeSession *previous_;
};

void SetError(const std::string &message) {
    std::lock_guard<std::mutex> lock(g_error_mutex);
    g_last_error = message;
    __android_log_print(
        ANDROID_LOG_ERROR,
        "ReverieNativeHost",
        "%s",
        message.c_str()
    );
}

void ClearError() {
    std::lock_guard<std::mutex> lock(g_error_mutex);
    g_last_error.clear();
}

std::string GetError() {
    std::lock_guard<std::mutex> lock(g_error_mutex);
    return g_last_error;
}

// Isolate pre-existing Cardboard/shell errors at BOTH guest lifecycle entry
// points. Draining only before render is too late for modules that inspect GL
// errors while creating shaders and buffers in on_gl_context_created.
// Errors raised by the guest after this boundary remain guest-owned.
void DrainInheritedGlErrors(NativeSession *session, const char *phase,
                            int eye_index) {
    GLenum first = GL_NO_ERROR;
    for (int i = 0; i < 8; ++i) {
        GLenum error = glGetError();
        if (error == GL_NO_ERROR) break;
        if (first == GL_NO_ERROR) first = error;
    }
    if (first != GL_NO_ERROR && session != nullptr
        && !session->warned_inherited_gl_error) {
        session->warned_inherited_gl_error = true;
        __android_log_print(ANDROID_LOG_WARN, "ReverieNativeHost",
            "Inherited GL error module=%s phase=%s eye=%d code=0x%x",
            session->spec != nullptr ? session->spec->id : "unknown",
            phase, eye_index, static_cast<unsigned>(first));
    }
}

void HostLog(
    int32_t level,
    const char *tag,
    const char *message
) {
    int priority = ANDROID_LOG_INFO;
    switch (level) {
        case REVERIE_NATIVE_LOG_DEBUG:
            priority = ANDROID_LOG_DEBUG;
            break;
        case REVERIE_NATIVE_LOG_WARN:
            priority = ANDROID_LOG_WARN;
            break;
        case REVERIE_NATIVE_LOG_ERROR:
            priority = ANDROID_LOG_ERROR;
            break;
        case REVERIE_NATIVE_LOG_INFO:
        default:
            priority = ANDROID_LOG_INFO;
            break;
    }

    __android_log_print(
        priority,
        tag != nullptr ? tag : "ReverieNativeModule",
        "%s",
        message != nullptr ? message : ""
    );
    if (level == REVERIE_NATIVE_LOG_ERROR
        && g_callback_session != nullptr && message != nullptr) {
        // Preserve the *guest's* bounded error while the GL callback is
        // active; the later host failure must not erase shader diagnostics.
        g_callback_session->last_guest_error.assign(message, strnlen(message, 192));
    }
}

bool IsSafeSaveSlot(const char *slot) {
    if (slot == nullptr) {
        return false;
    }

    const size_t length = std::strlen(slot);
    if (length == 0u || length > 64u) {
        return false;
    }

    for (size_t index = 0; index < length; ++index) {
        const char ch = slot[index];
        const bool safe =
            (ch >= 'a' && ch <= 'z')
                || (ch >= 'A' && ch <= 'Z')
                || (ch >= '0' && ch <= '9')
                || ch == '-'
                || ch == '_'
                || ch == '.';
        if (!safe) {
            return false;
        }
    }

    return std::strcmp(slot, ".") != 0
        && std::strcmp(slot, "..") != 0;
}

std::string SavePath(
    const NativeSession *session,
    const char *slot
) {
    if (session == nullptr
        || session->storage_root.empty()
        || !IsSafeSaveSlot(slot)) {
        return std::string();
    }

    return session->storage_root
        + "/"
        + slot;
}

int32_t HostReadSave(
    const char *slot,
    void *buffer,
    uint32_t capacity,
    uint32_t *out_size
) {
    NativeSession *session =
        g_callback_session;
    if (session == nullptr
        || out_size == nullptr) {
        SetError(
            "Native save read attempted outside an active module callback."
        );
        return REVERIE_NATIVE_SAVE_ERROR;
    }

    const std::string path =
        SavePath(session, slot);
    if (path.empty()) {
        SetError("Native save slot name is invalid.");
        return REVERIE_NATIVE_SAVE_ERROR;
    }

    struct stat info = {};
    if (stat(path.c_str(), &info) != 0) {
        if (errno == ENOENT) {
            *out_size = 0u;
            return REVERIE_NATIVE_SAVE_NOT_FOUND;
        }
        SetError("Could not inspect native save slot.");
        return REVERIE_NATIVE_SAVE_ERROR;
    }

    if (info.st_size < 0
        || static_cast<uint64_t>(info.st_size)
            > REVERIE_NATIVE_SAVE_MAX_BYTES) {
        SetError("Native save slot exceeds the host size limit.");
        return REVERIE_NATIVE_SAVE_ERROR;
    }

    const uint32_t size =
        static_cast<uint32_t>(info.st_size);
    *out_size = size;

    if (buffer == nullptr) {
        return REVERIE_NATIVE_SAVE_OK;
    }
    if (capacity < size) {
        return REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL;
    }

    int fd =
        open(
            path.c_str(),
            O_RDONLY
        );
    if (fd < 0) {
        SetError("Could not open native save slot for reading.");
        return REVERIE_NATIVE_SAVE_ERROR;
    }

    uint8_t *target =
        static_cast<uint8_t *>(buffer);
    uint32_t total = 0u;
    while (total < size) {
        const ssize_t count =
            read(
                fd,
                target + total,
                size - total
            );
        if (count <= 0) {
            close(fd);
            SetError("Native save slot read was incomplete.");
            return REVERIE_NATIVE_SAVE_ERROR;
        }
        total += static_cast<uint32_t>(count);
    }

    close(fd);
    return REVERIE_NATIVE_SAVE_OK;
}

int32_t HostWriteSave(
    const char *slot,
    const void *data,
    uint32_t size
) {
    NativeSession *session =
        g_callback_session;
    if (session == nullptr) {
        SetError(
            "Native save write attempted outside an active module callback."
        );
        return REVERIE_NATIVE_SAVE_ERROR;
    }
    if ((data == nullptr && size != 0u)
        || size > REVERIE_NATIVE_SAVE_MAX_BYTES) {
        SetError("Native save payload is invalid or too large.");
        return REVERIE_NATIVE_SAVE_ERROR;
    }

    const std::string path =
        SavePath(session, slot);
    if (path.empty()) {
        SetError("Native save slot name is invalid.");
        return REVERIE_NATIVE_SAVE_ERROR;
    }

    const std::string temporary =
        path + ".tmp";
    int fd =
        open(
            temporary.c_str(),
            O_WRONLY | O_CREAT | O_TRUNC,
            0600
        );
    if (fd < 0) {
        SetError("Could not open native save slot for writing.");
        return REVERIE_NATIVE_SAVE_ERROR;
    }

    const uint8_t *source =
        static_cast<const uint8_t *>(data);
    uint32_t total = 0u;
    while (total < size) {
        const ssize_t count =
            write(
                fd,
                source + total,
                size - total
            );
        if (count <= 0) {
            close(fd);
            unlink(temporary.c_str());
            SetError("Native save slot write was incomplete.");
            return REVERIE_NATIVE_SAVE_ERROR;
        }
        total += static_cast<uint32_t>(count);
    }

    if (fsync(fd) != 0) {
        close(fd);
        unlink(temporary.c_str());
        SetError("Native save slot could not be flushed.");
        return REVERIE_NATIVE_SAVE_ERROR;
    }
    if (close(fd) != 0) {
        unlink(temporary.c_str());
        SetError("Native save slot could not be closed cleanly.");
        return REVERIE_NATIVE_SAVE_ERROR;
    }

    if (rename(
            temporary.c_str(),
            path.c_str()
        ) != 0) {
        unlink(temporary.c_str());
        SetError("Native save slot atomic replace failed.");
        return REVERIE_NATIVE_SAVE_ERROR;
    }

    return REVERIE_NATIVE_SAVE_OK;
}

void HostRequestFeedback(
    uint32_t flags
) {
    NativeSession *session =
        g_callback_session;
    if (session == nullptr) {
        return;
    }

    const uint32_t safe_flags =
        flags
        & static_cast<uint32_t>(
            REVERIE_NATIVE_FEEDBACK_ALL
        );
    if (safe_flags == 0u) {
        return;
    }

    session->pending_feedback_flags.fetch_or(
        safe_flags,
        std::memory_order_relaxed
    );
}

void HostRequestSound(uint32_t cue_id) {
    NativeSession *session=g_callback_session;
    if(session==nullptr || cue_id<REVERIE_NATIVE_SOUND_WEAPON_MACHINE ||
        cue_id>REVERIE_NATIVE_SOUND_MAX_V1) return;
    std::lock_guard<std::mutex> guard(session->sound_mutex);
    session->pending_sounds.Push(cue_id);
}

int32_t HostReadAsset(const char *path,void *buffer,uint32_t capacity,
                      uint32_t *out_size) {
    NativeSession *session=g_callback_session;
    if(out_size) *out_size=0u;
    if(!session || !session->asset_manager || !out_size || !path ||
       !session->spec ||
       std::strcmp(session->spec->id,"between-deliveries-red-ledger")!=0)
        return REVERIE_NATIVE_SAVE_ERROR;
    constexpr char root[]="native/red-ledger/";
    constexpr size_t root_length=sizeof(root)-1;
    const size_t n=strnlen(path,128u);
    if(n<root_length+8u || n>=128u ||
       std::strncmp(path,root,root_length)!=0 ||
       std::strstr(path,"..")!=nullptr) return REVERIE_NATIVE_SAVE_ERROR;
    const char *leaf=path+root_length;
    const bool icon=std::strncmp(leaf,"icons/",6)==0;
    const bool model=std::strncmp(leaf,"models/",7)==0;
    const bool texture=std::strncmp(leaf,"textures/",9)==0;
    if(!icon && !model && !texture)return REVERIE_NATIVE_SAVE_ERROR;
    const size_t prefix_length=root_length+(icon?6u:model?7u:9u);
    for(size_t i=prefix_length;i<n;++i) {
        const char ch=path[i];
        if(!((ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||
             (ch>='0'&&ch<='9')||ch=='_'||ch=='-'||ch=='.'))
            return REVERIE_NATIVE_SAVE_ERROR;
    }
    AAsset *asset=AAssetManager_open(session->asset_manager,path,
                                    AASSET_MODE_STREAMING);
    if(!asset)return REVERIE_NATIVE_SAVE_NOT_FOUND;
    const off64_t size=AAsset_getLength64(asset);
    if(size<=0 || size>(icon?1024*1024:3*1024*1024)) {
        AAsset_close(asset);
        return REVERIE_NATIVE_SAVE_ERROR;
    }
    *out_size=static_cast<uint32_t>(size);
    if(!buffer || capacity<static_cast<uint32_t>(size)) {
        AAsset_close(asset);
        return REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL;
    }
    uint8_t *dst=static_cast<uint8_t *>(buffer);
    size_t at=0;
    while(at<static_cast<size_t>(size)) {
        const int got=AAsset_read(asset,dst+at,
                                  static_cast<size_t>(size)-at);
        if(got<=0) {
            AAsset_close(asset);
            return REVERIE_NATIVE_SAVE_ERROR;
        }
        at+=static_cast<size_t>(got);
    }
    AAsset_close(asset);
    return REVERIE_NATIVE_SAVE_OK;
}

const ReverieNativeHostV1 kHostServices = {
    sizeof(ReverieNativeHostV1),
    REVERIE_NATIVE_MODULE_ABI_VERSION,
    HostLog,
    HostReadSave,
    HostWriteSave,
    HostRequestFeedback,
    HostRequestSound,
    HostReadAsset
};

const BuiltInModuleSpec *FindSpec(const char *id) {
    if (id == nullptr) {
        return nullptr;
    }

    for (const BuiltInModuleSpec &spec : kBuiltIns) {
        if (std::strcmp(spec.id, id) == 0) {
            return &spec;
        }
    }

    return nullptr;
}

bool ValidateApi(
    const BuiltInModuleSpec &spec,
    const ReverieNativeModuleApiV1 *api
) {
    if (!ReverieNativeApiHasMandatoryV1(api)) {
        SetError("Native module API mandatory v1 prefix or ABI version is invalid.");
        return false;
    }
    if (!ReverieNativeDescriptorHasMandatoryV1(
            &api->descriptor
        )) {
        SetError("Native module descriptor mandatory v1 prefix or ABI version is invalid.");
        return false;
    }
    if (api->descriptor.id == nullptr
        || std::strcmp(api->descriptor.id, spec.id) != 0) {
        SetError("Native module id does not match the allowlisted module.");
        return false;
    }
    if (!ReverieNativeGlesRequirementSupportedV1(
            &api->descriptor,
            2u,
            0u
        )) {
        SetError(
            "Native module OpenGL ES requirement exceeds host support 2.0."
        );
        return false;
    }
    if (api->create == nullptr
        || api->destroy == nullptr
        || api->on_gl_context_created == nullptr
        || api->release_gl_context == nullptr
        || api->resume == nullptr
        || api->pause == nullptr
        || api->update == nullptr
        || api->render_eye == nullptr) {
        SetError("Native module API is missing a required callback.");
        return false;
    }

    return true;
}

NativeSession *FromHandle(jlong handle) {
    return reinterpret_cast<NativeSession *>(
        static_cast<intptr_t>(handle)
    );
}

jlong ToHandle(NativeSession *session) {
    return static_cast<jlong>(
        reinterpret_cast<intptr_t>(session)
    );
}

void DestroySession(NativeSession *session) {
    if (session == nullptr) {
        return;
    }

    if (session->api != nullptr
        && session->instance != nullptr) {
        CallbackSessionScope scope(session);
        session->api->destroy(session->instance);
        session->instance = nullptr;
    }

    if (session->library != nullptr) {
        dlclose(session->library);
        session->library = nullptr;
    }

    delete session;
}

jobjectArray NewStringArray(
    JNIEnv *env,
    jsize size
) {
    jclass string_class =
        env->FindClass("java/lang/String");
    if (string_class == nullptr) {
        return nullptr;
    }

    return env->NewObjectArray(
        size,
        string_class,
        nullptr
    );
}

}  // namespace

extern "C"
JNIEXPORT jobjectArray JNICALL
Java_io_github_mrcalzon02_reverievr_NativeModuleRuntime_nativeListBuiltIns(
    JNIEnv *env,
    jclass
) {
    const jsize count =
        static_cast<jsize>(
            sizeof(kBuiltIns)
                / sizeof(kBuiltIns[0])
        );
    jobjectArray result =
        NewStringArray(env, count * 2);
    if (result == nullptr) {
        return nullptr;
    }

    for (jsize index = 0; index < count; ++index) {
        const BuiltInModuleSpec &spec =
            kBuiltIns[index];
        jstring id =
            env->NewStringUTF(spec.id);
        jstring name =
            env->NewStringUTF(spec.display_name);

        env->SetObjectArrayElement(
            result,
            index * 2,
            id
        );
        env->SetObjectArrayElement(
            result,
            index * 2 + 1,
            name
        );

        env->DeleteLocalRef(id);
        env->DeleteLocalRef(name);
    }

    return result;
}

extern "C"
JNIEXPORT jlong JNICALL
Java_io_github_mrcalzon02_reverievr_NativeModuleRuntime_nativeCreate(
    JNIEnv *env,
    jclass,
    jstring module_id,
    jstring storage_root,
    jobject asset_manager
) {
    ClearError();

    if (module_id == nullptr
        || storage_root == nullptr || asset_manager == nullptr) {
        SetError(
            "Native module id and storage root are required."
        );
        return 0;
    }

    const char *id =
        env->GetStringUTFChars(
            module_id,
            nullptr
        );
    if (id == nullptr) {
        SetError("Could not read native module id.");
        return 0;
    }

    const BuiltInModuleSpec *spec =
        FindSpec(id);
    env->ReleaseStringUTFChars(
        module_id,
        id
    );

    if (spec == nullptr) {
        SetError("Native module id is not allowlisted.");
        return 0;
    }

    const char *storage =
        env->GetStringUTFChars(
            storage_root,
            nullptr
        );
    if (storage == nullptr) {
        SetError("Could not read native module storage root.");
        return 0;
    }

    std::string storage_path(storage);
    env->ReleaseStringUTFChars(
        storage_root,
        storage
    );

    if (storage_path.empty()
        || storage_path[0] != '/') {
        SetError("Native module storage root is invalid.");
        return 0;
    }

    void *library =
        dlopen(
            spec->library_name,
            RTLD_NOW | RTLD_LOCAL
        );
    if (library == nullptr) {
        const char *detail = dlerror();
        SetError(
            std::string("Could not load packaged native module: ")
                + (detail != nullptr ? detail : "unknown dlopen error")
        );
        return 0;
    }

    dlerror();
    void *symbol =
        dlsym(
            library,
            REVERIE_NATIVE_MODULE_ENTRY_SYMBOL
        );
    const char *symbol_error = dlerror();
    if (symbol == nullptr
        || symbol_error != nullptr) {
        SetError(
            std::string("Native module entry symbol is unavailable: ")
                + (
                    symbol_error != nullptr
                        ? symbol_error
                        : "unknown dlsym error"
                )
        );
        dlclose(library);
        return 0;
    }

    ReverieNativeModuleEntryV1 entry =
        reinterpret_cast<
            ReverieNativeModuleEntryV1
        >(symbol);
    const ReverieNativeModuleApiV1 *api =
        entry();

    if (!ValidateApi(*spec, api)) {
        dlclose(library);
        return 0;
    }

    NativeSession *session =
        new (std::nothrow) NativeSession();
    if (session == nullptr) {
        SetError("Could not allocate native module session.");
        dlclose(library);
        return 0;
    }

    session->spec = spec;
    session->library = library;
    session->api = api;
    session->storage_root = storage_path;
    session->asset_manager = AAssetManager_fromJava(env,asset_manager);
    if(session->asset_manager==nullptr) {
        SetError("Could not access packaged Android assets.");
        DestroySession(session);
        return 0;
    }

    {
        CallbackSessionScope scope(session);
        session->instance =
            api->create(&kHostServices);
    }
    if (session->instance == nullptr) {
        SetError("Native module create callback failed.");
        DestroySession(session);
        return 0;
    }

    __android_log_print(
        ANDROID_LOG_INFO,
        "ReverieNativeHost",
        "Loaded native module id=%s",
        spec->id
    );

    return ToHandle(session);
}

extern "C"
JNIEXPORT jfloatArray JNICALL
Java_io_github_mrcalzon02_reverievr_NativeModuleRuntime_nativeGetShellLocomotionBounds(
    JNIEnv *env,
    jclass,
    jlong handle
) {
    NativeSession *session = FromHandle(handle);
    if (session == nullptr
        || session->api == nullptr) {
        return nullptr;
    }

    const ReverieNativeModuleCapabilitiesV1 *capabilities =
        ReverieNativeApiCapabilitiesV1(
            session->api
        );
    if (!ReverieNativeCapabilitiesHasShellLocomotionV1(
            capabilities
        )) {
        return nullptr;
    }

    const float limit_x =
        capabilities->locomotion_limit_x;
    const float limit_z =
        capabilities->locomotion_limit_z;

    if (!std::isfinite(limit_x)
        || !std::isfinite(limit_z)
        || limit_x <= 0.0f
        || limit_z <= 0.0f
        || limit_x > 100.0f
        || limit_z > 100.0f) {
        SetError(
            "Native module shell-locomotion capability has invalid bounds."
        );
        return nullptr;
    }

    jfloatArray result =
        env->NewFloatArray(2);
    if (result == nullptr) {
        return nullptr;
    }

    const jfloat values[2] = {
        static_cast<jfloat>(limit_x),
        static_cast<jfloat>(limit_z)
    };
    env->SetFloatArrayRegion(
        result,
        0,
        2,
        values
    );
    return result;
}

extern "C"
JNIEXPORT void JNICALL
Java_io_github_mrcalzon02_reverievr_NativeModuleRuntime_nativeDestroy(
    JNIEnv *,
    jclass,
    jlong handle
) {
    DestroySession(FromHandle(handle));
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_io_github_mrcalzon02_reverievr_NativeModuleRuntime_nativeOnSurfaceCreated(
    JNIEnv *,
    jclass,
    jlong handle
) {
    NativeSession *session = FromHandle(handle);
    if (session == nullptr
        || session->api == nullptr
        || session->instance == nullptr) {
        SetError("Native module session is unavailable.");
        return JNI_FALSE;
    }

    // The parent shell has already drawn/created GLES resources. A stale GL
    // flag from that work must not fail a valid guest initialization.
    DrainInheritedGlErrors(session, "context-create", -1);
    session->last_guest_error.clear();
    {
        CallbackSessionScope scope(session);
        session->gl_ready =
            session->api->on_gl_context_created(
                session->instance
            ) != 0;
    }

    if (!session->gl_ready) {
        const GLenum guest_error = glGetError();
        char error_text[24];
        std::snprintf(error_text, sizeof(error_text), "0x%x",
            static_cast<unsigned>(guest_error));
        SetError(std::string("Native GL initialization failed module=")
            + session->spec->id + " guest_GL=" + error_text
            + (session->last_guest_error.empty()
                ? "" : " detail=" + session->last_guest_error));
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

extern "C"
JNIEXPORT void JNICALL
Java_io_github_mrcalzon02_reverievr_NativeModuleRuntime_nativeReleaseSurface(
    JNIEnv *,
    jclass,
    jlong handle
) {
    NativeSession *session = FromHandle(handle);
    if (session != nullptr
        && session->api != nullptr
        && session->instance != nullptr
        && session->gl_ready) {
        CallbackSessionScope scope(session);
        session->api->release_gl_context(
            session->instance
        );
        session->gl_ready = false;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_io_github_mrcalzon02_reverievr_NativeModuleRuntime_nativeResume(
    JNIEnv *,
    jclass,
    jlong handle
) {
    NativeSession *session = FromHandle(handle);
    if (session != nullptr
        && session->api != nullptr
        && session->instance != nullptr) {
        CallbackSessionScope scope(session);
        session->api->resume(session->instance);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_io_github_mrcalzon02_reverievr_NativeModuleRuntime_nativePause(
    JNIEnv *,
    jclass,
    jlong handle
) {
    NativeSession *session = FromHandle(handle);
    if (session != nullptr
        && session->api != nullptr
        && session->instance != nullptr) {
        CallbackSessionScope scope(session);
        session->api->pause(session->instance);
        // Never replay combat sounds after a paused native session resumes.
        std::lock_guard<std::mutex> guard(session->sound_mutex);
        session->pending_sounds.Clear();
    }
}

extern "C"
JNIEXPORT jlong JNICALL
Java_io_github_mrcalzon02_reverievr_NativeModuleRuntime_nativeDrainSoundCues(
    JNIEnv *, jclass, jlong handle
) {
    NativeSession *session=FromHandle(handle);
    if(session==nullptr)return 0;
    std::lock_guard<std::mutex> guard(session->sound_mutex);
    const uint64_t packet=session->pending_sounds.Drain();
    return static_cast<jlong>(packet);
}

extern "C"
JNIEXPORT jint JNICALL
Java_io_github_mrcalzon02_reverievr_NativeModuleRuntime_nativeUpdate(
    JNIEnv *,
    jclass,
    jlong handle,
    jfloat delta_seconds,
    jfloat move_x,
    jfloat move_y,
    jboolean primary_down,
    jboolean secondary_down,
    jint pointer_kind,
    jfloat pointer_origin_x,
    jfloat pointer_origin_y,
    jfloat pointer_origin_z,
    jfloat pointer_direction_x,
    jfloat pointer_direction_y,
    jfloat pointer_direction_z
) {
    NativeSession *session =
        FromHandle(handle);
    if (session == nullptr
        || session->api == nullptr
        || session->instance == nullptr) {
        return 0;
    }

    ReverieNativeInputV1 input = {};
    input.struct_size =
        sizeof(ReverieNativeInputV1);
    input.delta_seconds = static_cast<float>(delta_seconds);
    input.move_x = static_cast<float>(move_x);
    input.move_y = static_cast<float>(move_y);
    input.primary_down =
        primary_down == JNI_TRUE
            ? 1u
            : 0u;
    input.secondary_down =
        secondary_down == JNI_TRUE
            ? 1u
            : 0u;

    // Validate base input before invoking a native game.
    ReverieNativeSanitizeBaseInputV1(&input);

    input.pointer_kind = static_cast<uint32_t>(pointer_kind);
    input.pointer_origin[0] = static_cast<float>(pointer_origin_x);
    input.pointer_origin[1] = static_cast<float>(pointer_origin_y);
    input.pointer_origin[2] = static_cast<float>(pointer_origin_z);
    input.pointer_direction[0] = static_cast<float>(pointer_direction_x);
    input.pointer_direction[1] = static_cast<float>(pointer_direction_y);
    input.pointer_direction[2] = static_cast<float>(pointer_direction_z);
    // Shared fail-closed ray validation at the JNI boundary.
    ReverieNativeSanitizePointerV1(&input);

    CallbackSessionScope scope(session);
    session->api->update(
        session->instance,
        &input
    );
    // Quick Menu/controller loss yields POINTER_NONE; never carry a
    // previous sound burst forward into resumed gameplay.
    if(input.pointer_kind==REVERIE_NATIVE_POINTER_NONE) {
        std::lock_guard<std::mutex> guard(session->sound_mutex);
        session->pending_sounds.Clear();
    }

    return static_cast<jint>(
        session->pending_feedback_flags.exchange(
            0u,
            std::memory_order_relaxed
        )
    );
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_io_github_mrcalzon02_reverievr_NativeModuleRuntime_nativeRenderEye(
    JNIEnv *env,
    jclass,
    jlong handle,
    jint eye_index,
    jfloatArray view,
    jfloatArray projection
) {
    NativeSession *session = FromHandle(handle);
    if (session == nullptr
        || session->api == nullptr
        || session->instance == nullptr
        || !session->gl_ready) {
        SetError("Native module cannot render before GL initialization.");
        return JNI_FALSE;
    }

    if (view == nullptr
        || projection == nullptr
        || env->GetArrayLength(view) < 16
        || env->GetArrayLength(projection) < 16) {
        SetError("Native module render matrices are invalid.");
        return JNI_FALSE;
    }

    ReverieNativeEyeV1 eye = {};
    eye.struct_size = sizeof(ReverieNativeEyeV1);
    eye.eye_index = static_cast<int32_t>(eye_index);

    env->GetFloatArrayRegion(
        view,
        0,
        16,
        eye.view
    );
    env->GetFloatArrayRegion(
        projection,
        0,
        16,
        eye.projection
    );

    // Reject invalid optical input before invoking guest GLES code.
    if (!ReverieNativeEyeRenderableV1(&eye)) {
        SetError("Native module render eye index or matrices are invalid.");
        return JNI_FALSE;
    }

    // Per-eye checks stay independent from the context-creation boundary.
    // Never clear errors produced by the guest itself.
    DrainInheritedGlErrors(session, "render-eye",
        static_cast<int>(eye_index));

    int32_t rendered = 0;
    {
        CallbackSessionScope scope(session);
        rendered = session->api->render_eye(session->instance, &eye);
    }

    if (rendered == 0) {
        SetError(std::string("Native module render failed: ")
            + session->spec->id + " eye="
            + std::to_string(static_cast<int>(eye_index))
            + ". Review guest GL diagnostics.");
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_io_github_mrcalzon02_reverievr_NativeModuleRuntime_nativeGetLastError(
    JNIEnv *env,
    jclass
) {
    std::string error = GetError();
    return env->NewStringUTF(error.c_str());
}
