// Android JNI bridge for the independent Test Chamber native-code probe.
// Compiles the EXISTING test_chamber_module.cpp unchanged into its own APK.
#include "reverie_native_module.h"
#include <android/log.h>
#include <jni.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <new>

extern "C" const ReverieNativeModuleApiV1 *
reverie_native_module_entry_v1(void);

namespace {
struct Session {
    const ReverieNativeModuleApiV1 *api=nullptr;
    void *instance=nullptr;
    bool gl_ready=false;
};
void Log(int32_t level,const char *tag,const char *message){
    const int priority=level==REVERIE_NATIVE_LOG_ERROR?ANDROID_LOG_ERROR:
        level==REVERIE_NATIVE_LOG_WARN?ANDROID_LOG_WARN:
        level==REVERIE_NATIVE_LOG_DEBUG?ANDROID_LOG_DEBUG:ANDROID_LOG_INFO;
    __android_log_print(priority,tag?tag:"TestChamberCompanion","%s",
        message?message:"");
}
int32_t ReadSave(const char *,void *,uint32_t,uint32_t *out){
    if(out)*out=0u;
    // Test Chamber is stateless. Never manufacture save data.
    return REVERIE_NATIVE_SAVE_NOT_FOUND;
}
int32_t WriteSave(const char *,const void *,uint32_t){
    // This probe intentionally has no save persistence API.
    return REVERIE_NATIVE_SAVE_ERROR;
}
void Feedback(uint32_t flags){
    __android_log_print(ANDROID_LOG_DEBUG,"TestChamberCompanion",
        "feedback=%u",static_cast<unsigned>(flags));
}
const ReverieNativeHostV1 kHost={
    sizeof(ReverieNativeHostV1),
    REVERIE_NATIVE_MODULE_ABI_VERSION,
    Log,ReadSave,WriteSave,Feedback,nullptr,nullptr
};
Session *SessionFrom(jlong value){
    return reinterpret_cast<Session *>(static_cast<uintptr_t>(value));
}
bool Usable(const ReverieNativeModuleApiV1 *api){
    return api&&api->abi_version==REVERIE_NATIVE_MODULE_ABI_VERSION &&
        api->struct_size>=REVERIE_NATIVE_MODULE_API_V1_MIN_SIZE &&
        api->descriptor.abi_version==REVERIE_NATIVE_MODULE_ABI_VERSION &&
        api->descriptor.id&&
        std::strcmp(api->descriptor.id,"procedural-test-chamber")==0 &&
        api->descriptor.required_gles_major<=2u &&
        api->create&&api->destroy&&api->on_gl_context_created&&
        api->release_gl_context&&api->resume&&api->pause&&
        api->update&&api->render_eye;
}
} // namespace

extern "C" JNIEXPORT jlong JNICALL
Java_io_github_mrcalzon02_reverievr_game_testchamber_GameVrActivity_nativeCreate(
    JNIEnv *,jclass){
    const auto *api=reverie_native_module_entry_v1();
    if(!Usable(api))return 0;
    Session *s=new(std::nothrow) Session;
    if(!s)return 0;
    s->api=api;
    s->instance=api->create(&kHost);
    if(!s->instance){delete s;return 0;}
    return static_cast<jlong>(reinterpret_cast<uintptr_t>(s));
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_github_mrcalzon02_reverievr_game_testchamber_GameVrActivity_nativeSurfaceCreated(
    JNIEnv *,jclass,jlong handle){
    Session *s=SessionFrom(handle);
    if(!s)return JNI_FALSE;
    // The module's InitializeGl abandons stale IDs after GL context loss.
    s->gl_ready=false;
    s->gl_ready=s->api->on_gl_context_created(s->instance)!=0;
    return s->gl_ready?JNI_TRUE:JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_io_github_mrcalzon02_reverievr_game_testchamber_GameVrActivity_nativeSurfaceReleased(
    JNIEnv *,jclass,jlong handle){
    Session *s=SessionFrom(handle);
    if(!s||!s->gl_ready)return;
    s->api->release_gl_context(s->instance);
    s->gl_ready=false;
}

extern "C" JNIEXPORT void JNICALL
Java_io_github_mrcalzon02_reverievr_game_testchamber_GameVrActivity_nativeResume(
    JNIEnv *,jclass,jlong handle){
    Session *s=SessionFrom(handle);
    if(s)s->api->resume(s->instance);
}

extern "C" JNIEXPORT void JNICALL
Java_io_github_mrcalzon02_reverievr_game_testchamber_GameVrActivity_nativePause(
    JNIEnv *,jclass,jlong handle){
    Session *s=SessionFrom(handle);
    if(s)s->api->pause(s->instance);
}

extern "C" JNIEXPORT void JNICALL
Java_io_github_mrcalzon02_reverievr_game_testchamber_GameVrActivity_nativeUpdate(
    JNIEnv *,jclass,jlong handle,jfloat seconds,jboolean select){
    Session *s=SessionFrom(handle);
    if(!s)return;
    ReverieNativeInputV1 input={};
    input.struct_size=sizeof(input);
    input.delta_seconds=std::isfinite(seconds)?
        std::max(0.f,std::min(0.05f,seconds)):0.f;
    input.primary_down=select?1u:0u;
    input.pointer_kind=REVERIE_NATIVE_POINTER_NONE;
    s->api->update(s->instance,&input);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_github_mrcalzon02_reverievr_game_testchamber_GameVrActivity_nativeRenderEye(
    JNIEnv *env,jclass,jlong handle,jint index,jfloatArray view,
    jfloatArray projection){
    Session *s=SessionFrom(handle);
    if(!s||!s->gl_ready||!view||!projection||index<0||index>1 ||
       env->GetArrayLength(view)<16||env->GetArrayLength(projection)<16)
        return JNI_FALSE;
    ReverieNativeEyeV1 eye={};
    eye.struct_size=sizeof(eye);
    eye.eye_index=static_cast<int32_t>(index);
    env->GetFloatArrayRegion(view,0,16,eye.view);
    env->GetFloatArrayRegion(projection,0,16,eye.projection);
    if(env->ExceptionCheck())return JNI_FALSE;
    return s->api->render_eye(s->instance,&eye)?JNI_TRUE:JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_io_github_mrcalzon02_reverievr_game_testchamber_GameVrActivity_nativeDestroy(
    JNIEnv *,jclass,jlong handle){
    Session *s=SessionFrom(handle);
    if(!s)return;
    // GL objects are released from onRendererShutdown on the GL thread.
    // Test Chamber's Destroy abandons objects if Android skipped shutdown.
    s->api->destroy(s->instance);
    delete s;
}
