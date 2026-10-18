// Independent, actual Breakwater Battery native C++ game companion.
// Provides private, durable save slots and packaged owner-approved BWM1
// meshes to the unchanged Reverie native module ABI v1.
#include "reverie_native_sdk.h"
#include "reverie_native_asset_scope.h"
#include "breakwater_checkpoint.h"
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <jni.h>
#include <GLES2/gl2.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <new>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

extern "C" const ReverieNativeModuleApiV1*
reverie_native_module_entry_v1(void);

namespace {
constexpr const char* kGameId="breakwater-battery";
constexpr uint32_t kMaxSave=65536u;
struct Session {
    const ReverieNativeModuleApiV1* api=nullptr;
    void* instance=nullptr;
    AAssetManager* assets=nullptr;
    std::string private_root;
    bool gl_ready=false;
    uint8_t sounds[8]{};
    unsigned sound_count=0;
};
thread_local Session* active=nullptr;
struct SessionScope {
    Session* previous;
    explicit SessionScope(Session* s):previous(active){active=s;}
    ~SessionScope(){active=previous;}
};
Session* From(jlong ptr) {
    return reinterpret_cast<Session*>(static_cast<uintptr_t>(ptr));
}
bool SaveSlot(const char* slot){
    if(!slot)return false;
    for(uint8_t i=0;i<3u;++i)
        if(std::strcmp(slot,reverie::breakwater::CheckpointSlot(i))==0)
            return true;
    return false;
}
void Log(int32_t level,const char* tag,const char* text){
    int priority=level==REVERIE_NATIVE_LOG_ERROR?ANDROID_LOG_ERROR:
        level==REVERIE_NATIVE_LOG_WARN?ANDROID_LOG_WARN:
        level==REVERIE_NATIVE_LOG_DEBUG?ANDROID_LOG_DEBUG:ANDROID_LOG_INFO;
    __android_log_print(priority,tag?tag:"BreakwaterCompanion",
        "%s",text?text:"");
}
int32_t ReadSave(const char* name,void* data,uint32_t capacity,
                 uint32_t* out){
    if(out)*out=0u;
    if(!active||!out||!SaveSlot(name))return REVERIE_NATIVE_SAVE_ERROR;
    const std::string path=active->private_root+"/"+name;
    int fd=open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
    if(fd<0)return errno==ENOENT?
        REVERIE_NATIVE_SAVE_NOT_FOUND:REVERIE_NATIVE_SAVE_ERROR;
    struct stat st={};
    if(fstat(fd,&st)!=0||!S_ISREG(st.st_mode)||st.st_size<0||
       st.st_size>kMaxSave){close(fd);return REVERIE_NATIVE_SAVE_ERROR;}
    const uint32_t len=static_cast<uint32_t>(st.st_size);
    *out=len;
    if(capacity<len||(!data&&len!=0u)){
        close(fd);return REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL;
    }
    uint32_t at=0u;
    while(at<len){
        ssize_t got=read(fd,static_cast<uint8_t*>(data)+at,len-at);
        if(got<=0){close(fd);return REVERIE_NATIVE_SAVE_ERROR;}
        at+=static_cast<uint32_t>(got);
    }
    close(fd);
    return REVERIE_NATIVE_SAVE_OK;
}
int32_t WriteSave(const char* name,const void* bytes,uint32_t size){
    if(!active||!SaveSlot(name)||size>kMaxSave||(!bytes&&size>0u))
        return REVERIE_NATIVE_SAVE_ERROR;
    const std::string path=active->private_root+"/"+name;
    const std::string stage=path+".pending";
    int fd=open(stage.c_str(),O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC|O_NOFOLLOW,0600);
    if(fd<0)return REVERIE_NATIVE_SAVE_ERROR;
    uint32_t at=0;
    while(at<size){
        ssize_t written=write(fd,static_cast<const uint8_t*>(bytes)+at,size-at);
        if(written<=0){close(fd);unlink(stage.c_str());
            return REVERIE_NATIVE_SAVE_ERROR;}
        at+=static_cast<uint32_t>(written);
    }
    bool valid=fsync(fd)==0 && close(fd)==0;
    if(!valid||rename(stage.c_str(),path.c_str())!=0){
        unlink(stage.c_str());return REVERIE_NATIVE_SAVE_ERROR;
    }
    int dir=open(active->private_root.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC);
    if(dir<0)return REVERIE_NATIVE_SAVE_ERROR;
    valid=fsync(dir)==0;
    close(dir);
    if(!valid)return REVERIE_NATIVE_SAVE_ERROR;
    __android_log_print(ANDROID_LOG_DEBUG,"BreakwaterCompanion",
        "Private checkpoint saved slot=%s bytes=%u",name,size);
    return REVERIE_NATIVE_SAVE_OK;
}
void Feedback(uint32_t flags){
    __android_log_print(ANDROID_LOG_DEBUG,"BreakwaterCompanion",
        "feedback=0x%x",flags);
}
void Sound(uint32_t cue){
    if(!active||cue==0u||cue>REVERIE_NATIVE_SOUND_MAX_V1||
       active->sound_count>=8u)return;
    active->sounds[active->sound_count++]=static_cast<uint8_t>(cue);
}
int32_t ReadAsset(const char* path,void* buffer,uint32_t capacity,
                  uint32_t* size){
    if(size)*size=0u;
    if(!active||!active->assets||!path||!size)return REVERIE_NATIVE_SAVE_ERROR;
    uint32_t max=reverie::native::assets::MaximumBytes(kGameId,path);
    if(max==0u)return REVERIE_NATIVE_SAVE_ERROR;
    AAsset* asset=AAssetManager_open(active->assets,path,AASSET_MODE_STREAMING);
    if(!asset)return REVERIE_NATIVE_SAVE_NOT_FOUND;
    const off64_t available=AAsset_getLength64(asset);
    if(available<=0||static_cast<uint64_t>(available)>max){
        AAsset_close(asset);return REVERIE_NATIVE_SAVE_ERROR;
    }
    const uint32_t len=static_cast<uint32_t>(available);
    *size=len;
    if(!buffer||capacity<len){
        AAsset_close(asset);return REVERIE_NATIVE_SAVE_BUFFER_TOO_SMALL;
    }
    uint32_t at=0;
    while(at<len){
        int got=AAsset_read(asset,static_cast<uint8_t*>(buffer)+at,len-at);
        if(got<=0){AAsset_close(asset);return REVERIE_NATIVE_SAVE_ERROR;}
        at+=static_cast<uint32_t>(got);
    }
    AAsset_close(asset);
    __android_log_print(ANDROID_LOG_INFO,"BreakwaterCompanion",
        "Packed game asset loaded %s bytes=%u",path,len);
    return REVERIE_NATIVE_SAVE_OK;
}
const ReverieNativeHostV1 kServices={
    sizeof(ReverieNativeHostV1),REVERIE_NATIVE_MODULE_ABI_VERSION,
    Log,ReadSave,WriteSave,Feedback,Sound,ReadAsset
};
bool Accepted(const ReverieNativeModuleApiV1* api){
    return api&&api->struct_size>=REVERIE_NATIVE_MODULE_API_V1_MIN_SIZE&&
        api->abi_version==REVERIE_NATIVE_MODULE_ABI_VERSION&&
        api->descriptor.abi_version==REVERIE_NATIVE_MODULE_ABI_VERSION&&
        api->descriptor.id&&std::strcmp(api->descriptor.id,kGameId)==0&&
        api->descriptor.required_gles_major<=2u&&
        api->create&&api->destroy&&api->on_gl_context_created&&
        api->release_gl_context&&api->resume&&api->pause&&
        api->update&&api->render_eye;
}
} // namespace

extern "C" JNIEXPORT jlong JNICALL
Java_io_github_mrcalzon02_reverievr_game_breakwater_GameVrActivity_nativeCreate(
    JNIEnv* env,jclass,jobject manager,jstring files){
    if(!manager||!files)return 0;
    const ReverieNativeModuleApiV1* api=reverie_native_module_entry_v1();
    if(!Accepted(api)){
        __android_log_print(ANDROID_LOG_ERROR,"BreakwaterCompanion",
            "Game native entry or ABI rejected");
        return 0;
    }
    AAssetManager* am=AAssetManager_fromJava(env,manager);
    if(!am)return 0;
    const char* path=env->GetStringUTFChars(files,nullptr);
    if(!path)return 0;
    std::string root(path);
    env->ReleaseStringUTFChars(files,path);
    if(root.size()<3u||root[0]!='/'||root.find("..")!=std::string::npos)
        return 0;
    root+="/native-breakwater";
    if(mkdir(root.c_str(),0700)!=0&&errno!=EEXIST)return 0;
    struct stat st={};
    if(lstat(root.c_str(),&st)!=0||!S_ISDIR(st.st_mode))return 0;
    Session* s=new(std::nothrow) Session;
    if(!s)return 0;
    s->api=api;s->assets=am;s->private_root=root;
    {
        SessionScope scope(s);
        s->instance=api->create(&kServices);
    }
    if(!s->instance){delete s;return 0;}
    __android_log_print(ANDROID_LOG_INFO,"BreakwaterCompanion",
        "Started actual native game id=%s own-save-root=true",kGameId);
    return static_cast<jlong>(reinterpret_cast<uintptr_t>(s));
}
extern "C" JNIEXPORT jboolean JNICALL
Java_io_github_mrcalzon02_reverievr_game_breakwater_GameVrActivity_nativeSurfaceCreated(
    JNIEnv*,jclass,jlong handle){
    Session* s=From(handle);
    if(!s)return JNI_FALSE;
    s->gl_ready=false;
    SessionScope scope(s);
    s->gl_ready=s->api->on_gl_context_created(s->instance)!=0;
    if(s->gl_ready)__android_log_print(ANDROID_LOG_INFO,
        "BreakwaterCompanion","GLES2 initialized for native Breakwater");
    return s->gl_ready?JNI_TRUE:JNI_FALSE;
}
extern "C" JNIEXPORT void JNICALL
Java_io_github_mrcalzon02_reverievr_game_breakwater_GameVrActivity_nativeSurfaceReleased(
    JNIEnv*,jclass,jlong handle){
    Session* s=From(handle);
    if(!s||!s->gl_ready)return;
    SessionScope scope(s);
    s->api->release_gl_context(s->instance);
    s->gl_ready=false;
}
extern "C" JNIEXPORT void JNICALL
Java_io_github_mrcalzon02_reverievr_game_breakwater_GameVrActivity_nativeResume(
    JNIEnv*,jclass,jlong handle){
    Session* s=From(handle);
    if(s){SessionScope scope(s);s->api->resume(s->instance);}
}
extern "C" JNIEXPORT void JNICALL
Java_io_github_mrcalzon02_reverievr_game_breakwater_GameVrActivity_nativePause(
    JNIEnv*,jclass,jlong handle){
    Session* s=From(handle);
    if(s){SessionScope scope(s);s->api->pause(s->instance);}
}
extern "C" JNIEXPORT jlong JNICALL
Java_io_github_mrcalzon02_reverievr_game_breakwater_GameVrActivity_nativeUpdate(
    JNIEnv* env,jclass,jlong handle,jfloat seconds,jboolean primary,
    jboolean secondary,jfloat moveX,jfloat moveY,jfloatArray head){
    Session* s=From(handle);
    if(!s||!head||env->GetArrayLength(head)<16)return 0L;
    jfloat m[16]{};
    env->GetFloatArrayRegion(head,0,16,m);
    if(env->ExceptionCheck())return 0L;
    ReverieNativeInputV1 input={};
    input.struct_size=sizeof(input);
    input.delta_seconds=std::isfinite(seconds)?
        std::max(0.f,std::min(.05f,seconds)):0.f;
    input.move_x=std::isfinite(moveX)?std::max(-1.f,std::min(1.f,moveX)):0.f;
    input.move_y=std::isfinite(moveY)?std::max(-1.f,std::min(1.f,moveY)):0.f;
    input.primary_down=primary?1u:0u;
    input.secondary_down=secondary?1u:0u;
    // The Daydream headset supplies a genuine world-space gaze ray.
    const float x=-m[2],y=-m[6],z=-m[10];
    const float n=std::sqrt(x*x+y*y+z*z);
    if(std::isfinite(n)&&n>.2f){
        input.pointer_kind=REVERIE_NATIVE_POINTER_VIRTUAL_CONTROLLER;
        input.pointer_direction[0]=x/n;
        input.pointer_direction[1]=y/n;
        input.pointer_direction[2]=z/n;
    }
    SessionScope scope(s);
    s->api->update(s->instance,&input);
    uint64_t cues=0u;
    for(unsigned i=0;i<s->sound_count;++i)
        cues|=static_cast<uint64_t>(s->sounds[i])<<(8u*i);
    s->sound_count=0u;
    return static_cast<jlong>(cues);
}
extern "C" JNIEXPORT jboolean JNICALL
Java_io_github_mrcalzon02_reverievr_game_breakwater_GameVrActivity_nativeRenderEye(
    JNIEnv* env,jclass,jlong handle,jint eye,jfloatArray view,
    jfloatArray projection){
    Session* s=From(handle);
    if(!s||!s->gl_ready||eye<0||eye>1||!view||!projection||
       env->GetArrayLength(view)<16||env->GetArrayLength(projection)<16)
        return JNI_FALSE;
    ReverieNativeEyeV1 state={};
    state.struct_size=sizeof(state);state.eye_index=eye;
    env->GetFloatArrayRegion(view,0,16,state.view);
    env->GetFloatArrayRegion(projection,0,16,state.projection);
    if(env->ExceptionCheck())return JNI_FALSE;
    SessionScope scope(s);
    return s->api->render_eye(s->instance,&state)?JNI_TRUE:JNI_FALSE;
}
extern "C" JNIEXPORT void JNICALL
Java_io_github_mrcalzon02_reverievr_game_breakwater_GameVrActivity_nativeDestroy(
    JNIEnv*,jclass,jlong handle){
    Session* s=From(handle);
    if(!s)return;
    SessionScope scope(s);
    if(s->instance)s->api->destroy(s->instance);
    delete s;
}
