#include "reverie_native_module.h"

#include <android/log.h>
#include <dlfcn.h>
#include <jni.h>

#include <algorithm>
#include <cstring>
#include <mutex>
#include <string>

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
    }
};

struct NativeSession {
    const BuiltInModuleSpec *spec = nullptr;
    void *library = nullptr;
    const ReverieNativeModuleApiV1 *api = nullptr;
    void *instance = nullptr;
    bool gl_ready = false;
};

std::mutex g_error_mutex;
std::string g_last_error;

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
}

const ReverieNativeHostV1 kHostServices = {
    sizeof(ReverieNativeHostV1),
    REVERIE_NATIVE_MODULE_ABI_VERSION,
    HostLog
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
    if (api == nullptr) {
        SetError("Native module entry returned a null API.");
        return false;
    }
    if (api->struct_size < sizeof(ReverieNativeModuleApiV1)) {
        SetError("Native module API structure is too small.");
        return false;
    }
    if (api->abi_version != REVERIE_NATIVE_MODULE_ABI_VERSION) {
        SetError("Native module ABI version is unsupported.");
        return false;
    }
    if (api->descriptor.struct_size
        < sizeof(ReverieNativeModuleDescriptorV1)) {
        SetError("Native module descriptor structure is too small.");
        return false;
    }
    if (api->descriptor.abi_version
        != REVERIE_NATIVE_MODULE_ABI_VERSION) {
        SetError("Native module descriptor ABI version is unsupported.");
        return false;
    }
    if (api->descriptor.id == nullptr
        || std::strcmp(api->descriptor.id, spec.id) != 0) {
        SetError("Native module id does not match the allowlisted module.");
        return false;
    }
    if (api->descriptor.required_gles_major > 2u) {
        SetError("Native module requires a newer OpenGL ES major version.");
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
    jstring module_id
) {
    ClearError();

    if (module_id == nullptr) {
        SetError("Native module id is required.");
        return 0;
    }

    const char *id =
        env->GetStringUTFChars(module_id, nullptr);
    if (id == nullptr) {
        SetError("Could not read native module id.");
        return 0;
    }

    const BuiltInModuleSpec *spec = FindSpec(id);
    env->ReleaseStringUTFChars(module_id, id);

    if (spec == nullptr) {
        SetError("Native module id is not allowlisted.");
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
    if (symbol == nullptr || symbol_error != nullptr) {
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

    void *instance = api->create(&kHostServices);
    if (instance == nullptr) {
        SetError("Native module create callback failed.");
        dlclose(library);
        return 0;
    }

    NativeSession *session = new NativeSession();
    session->spec = spec;
    session->library = library;
    session->api = api;
    session->instance = instance;

    __android_log_print(
        ANDROID_LOG_INFO,
        "ReverieNativeHost",
        "Loaded native module id=%s",
        spec->id
    );

    return ToHandle(session);
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

    session->gl_ready =
        session->api->on_gl_context_created(
            session->instance
        ) != 0;

    if (!session->gl_ready) {
        SetError("Native module failed to initialize its GL resources.");
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
        session->api->pause(session->instance);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_io_github_mrcalzon02_reverievr_NativeModuleRuntime_nativeUpdate(
    JNIEnv *,
    jclass,
    jlong handle,
    jfloat delta_seconds,
    jfloat move_x,
    jfloat move_y,
    jboolean primary_down,
    jboolean secondary_down
) {
    NativeSession *session = FromHandle(handle);
    if (session == nullptr
        || session->api == nullptr
        || session->instance == nullptr) {
        return;
    }

    ReverieNativeInputV1 input = {};
    input.struct_size =
        sizeof(ReverieNativeInputV1);
    input.delta_seconds =
        std::max(
            0.0f,
            std::min(
                static_cast<float>(delta_seconds),
                0.1f
            )
        );
    input.move_x =
        std::max(
            -1.0f,
            std::min(
                static_cast<float>(move_x),
                1.0f
            )
        );
    input.move_y =
        std::max(
            -1.0f,
            std::min(
                static_cast<float>(move_y),
                1.0f
            )
        );
    input.primary_down =
        primary_down == JNI_TRUE ? 1u : 0u;
    input.secondary_down =
        secondary_down == JNI_TRUE ? 1u : 0u;

    session->api->update(
        session->instance,
        &input
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

    const int32_t rendered =
        session->api->render_eye(
            session->instance,
            &eye
        );

    if (rendered == 0) {
        SetError("Native module render callback reported failure.");
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
