#include <jni.h>
#include <android/log.h>
#include <libretro.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace {
constexpr char TAG[] = "ReverieVR-DOS";
constexpr size_t AUDIO_FRAMES = 48000u * 2u;
constexpr int16_t AXIS_MAX = 32767;

struct AudioRing {
    std::vector<int16_t> data = std::vector<int16_t>(AUDIO_FRAMES * 2u);
    size_t read = 0, write = 0, count = 0;

    void clear() { read = write = count = 0; }
    void push(const int16_t* src, size_t frames) {
        if (!src || !frames) return;
        if (frames >= AUDIO_FRAMES) {
            src += (frames - AUDIO_FRAMES) * 2u;
            frames = AUDIO_FRAMES;
            clear();
        }
        while (count + frames > AUDIO_FRAMES) {
            read = (read + 1u) % AUDIO_FRAMES;
            --count;
        }
        for (size_t i = 0; i < frames; ++i) {
            data[write * 2u] = src[i * 2u];
            data[write * 2u + 1u] = src[i * 2u + 1u];
            write = (write + 1u) % AUDIO_FRAMES;
        }
        count += frames;
    }
    size_t pop(int16_t* dst, size_t maxFrames) {
        const size_t frames = std::min(maxFrames, count);
        for (size_t i = 0; i < frames; ++i) {
            dst[i * 2u] = data[read * 2u];
            dst[i * 2u + 1u] = data[read * 2u + 1u];
            read = (read + 1u) % AUDIO_FRAMES;
        }
        count -= frames;
        return frames;
    }
};

struct InputState {
    std::array<uint8_t, RETROK_LAST> keys{};
    std::array<bool, 3> mouse{};
    std::array<bool, 16> joy{};
    int16_t relX = 0, relY = 0, absX = 0, absY = 0;
    int16_t joyX = 0, joyY = 0;
    int8_t wheel = 0;
    void release() {
        keys.fill(0); mouse.fill(false); joy.fill(false);
        relX = relY = absX = absY = joyX = joyY = 0;
        wheel = 0;
    }
    void clearTransient() { relX = relY = 0; wheel = 0; }
};

struct Host {
    Host(std::string systemDir, std::string saveDir)
        : system(std::move(systemDir)), save(std::move(saveDir)) {}
    std::string system, save, content, contentDir, error;
    std::map<std::string, std::string> options;
    bool initialized = false, loaded = false, shutdown = false;
    retro_pixel_format format = RETRO_PIXEL_FORMAT_XRGB8888;
    double fps = 0.0, sampleRate = 0.0;

    std::mutex frameMutex;
    std::vector<uint8_t> frame;
    int width = 0, height = 0;
    uint64_t frameSerial = 0;

    std::mutex audioMutex;
    AudioRing audio;

    std::mutex inputMutex;
    InputState liveInput, frameInput;
};

Host* gHost = nullptr;
std::mutex gLifecycle;

Host* hostFrom(jlong handle) {
    auto* host = reinterpret_cast<Host*>(static_cast<intptr_t>(handle));
    return host == gHost ? host : nullptr;
}
jlong handleOf(Host* host) {
    return static_cast<jlong>(reinterpret_cast<intptr_t>(host));
}
std::string jstr(JNIEnv* env, jstring value) {
    if (!value) return {};
    const char* chars = env->GetStringUTFChars(value, nullptr);
    if (!chars) return {};
    std::string out(chars);
    env->ReleaseStringUTFChars(value, chars);
    return out;
}
std::string parentDir(const std::string& path) {
    const auto split = path.find_last_of("/\\");
    if (split == std::string::npos) return {};
    return split == 0 ? path.substr(0, 1) : path.substr(0, split);
}
std::string optionDefault(const char* descriptor) {
    if (!descriptor) return {};
    const char* p = std::strchr(descriptor, ';');
    if (!p) return {};
    for (++p; *p == ' ' || *p == '\t'; ++p) {}
    const char* end = std::strchr(p, '|');
    if (!end) end = descriptor + std::strlen(descriptor);
    while (end > p && (end[-1] == ' ' || end[-1] == '\t')) --end;
    return std::string(p, static_cast<size_t>(end - p));
}
void coreLog(retro_log_level level, const char* format, ...) {
    int priority = ANDROID_LOG_INFO;
    if (level == RETRO_LOG_DEBUG) priority = ANDROID_LOG_DEBUG;
    else if (level == RETRO_LOG_WARN) priority = ANDROID_LOG_WARN;
    else if (level == RETRO_LOG_ERROR) priority = ANDROID_LOG_ERROR;
    char buffer[2048];
    va_list args; va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    __android_log_print(priority, TAG, "%s", buffer);
}
void applyAv(Host* host, const retro_system_av_info* info) {
    if (!host || !info) return;
    host->fps = info->timing.fps;
    host->sampleRate = info->timing.sample_rate;
}

bool environment(unsigned command, void* data) {
    Host* host = gHost;
    if (!host) return false;
    switch (command) {
        case RETRO_ENVIRONMENT_GET_CAN_DUPE:
            if (!data) return false;
            *static_cast<bool*>(data) = true; return true;
        case RETRO_ENVIRONMENT_SET_MESSAGE:
            if (!data) return false;
            if (static_cast<retro_message*>(data)->msg)
                coreLog(RETRO_LOG_INFO, "%s", static_cast<retro_message*>(data)->msg);
            return true;
        case RETRO_ENVIRONMENT_SHUTDOWN:
            host->shutdown = true; return true;
        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
            if (!data) return false;
            *static_cast<const char**>(data) = host->system.c_str(); return true;
        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
            if (!data) return false;
            *static_cast<const char**>(data) = host->save.c_str(); return true;
        case RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY:
            if (!data) return false;
            *static_cast<const char**>(data) =
                (host->contentDir.empty() ? host->system : host->contentDir).c_str();
            return true;
        case RETRO_ENVIRONMENT_GET_VARIABLE: {
            if (!data) return false;
            auto* var = static_cast<retro_variable*>(data);
            if (!var->key) return false;
            auto found = host->options.find(var->key);
            var->value = found == host->options.end() ? nullptr : found->second.c_str();
            return var->value != nullptr;
        }
        case RETRO_ENVIRONMENT_SET_VARIABLES: {
            if (!data) return false;
            auto* var = static_cast<const retro_variable*>(data);
            for (; var->key; ++var) {
                if (host->options.count(var->key)) continue;
                std::string value = optionDefault(var->value);
                if (!value.empty()) host->options.emplace(var->key, std::move(value));
            }
            // No libretro hardware-render callback yet: keep Voodoo on software.
            host->options["dosbox_pure_voodoo_perf"] = "1";
            host->options["dosbox_pure_audiorate"] = "48000";
            return true;
        }
        case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
            if (!data) return false;
            *static_cast<bool*>(data) = false; return true;
        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
            if (!data) return false;
            if (*static_cast<retro_pixel_format*>(data) != RETRO_PIXEL_FORMAT_XRGB8888)
                return false;
            host->format = RETRO_PIXEL_FORMAT_XRGB8888; return true;
        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
            if (!data) return false;
            static_cast<retro_log_callback*>(data)->log = coreLog; return true;
        case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
            if (!data) return false;
            *static_cast<unsigned*>(data) = 0; return true;
        case RETRO_ENVIRONMENT_GET_LANGUAGE:
            if (!data) return false;
            *static_cast<unsigned*>(data) = RETRO_LANGUAGE_ENGLISH; return true;
        case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO:
            if (!data) return false;
            applyAv(host, static_cast<const retro_system_av_info*>(data)); return true;
        case RETRO_ENVIRONMENT_SET_GEOMETRY:
        case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
        case RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS:
            return true;
#ifdef RETRO_ENVIRONMENT_GET_FASTFORWARDING
        case RETRO_ENVIRONMENT_GET_FASTFORWARDING:
            if (!data) return false;
            *static_cast<bool*>(data) = false; return true;
#endif
        default:
            return false;
    }
}

void video(const void* data, unsigned width, unsigned height, size_t pitch) {
    Host* host = gHost;
    if (!host || !data || data == RETRO_HW_FRAME_BUFFER_VALID || !width || !height)
        return;
    const size_t rowBytes = static_cast<size_t>(width) * 4u;
    if (pitch < rowBytes || rowBytes > std::numeric_limits<size_t>::max() / height) {
        host->error = "DOS framebuffer geometry is invalid."; return;
    }
    std::lock_guard<std::mutex> lock(host->frameMutex);
    host->frame.resize(rowBytes * height);
    const auto* src = static_cast<const uint8_t*>(data);
    for (unsigned y = 0; y < height; ++y)
        std::memcpy(host->frame.data() + static_cast<size_t>(y) * rowBytes,
                    src + static_cast<size_t>(y) * pitch, rowBytes);
    host->width = static_cast<int>(width);
    host->height = static_cast<int>(height);
    ++host->frameSerial;
}
void audioOne(int16_t left, int16_t right) {
    Host* host = gHost; if (!host) return;
    const int16_t frame[2] = {left, right};
    std::lock_guard<std::mutex> lock(host->audioMutex);
    host->audio.push(frame, 1);
}
size_t audioBatch(const int16_t* data, size_t frames) {
    Host* host = gHost; if (!host || !data) return 0;
    std::lock_guard<std::mutex> lock(host->audioMutex);
    host->audio.push(data, frames); return frames;
}
void inputPoll() {}
int16_t inputState(unsigned port, unsigned device, unsigned index, unsigned id) {
    Host* host = gHost; if (!host || port) return 0;
    const InputState& in = host->frameInput;
    switch (device & RETRO_DEVICE_MASK) {
        case RETRO_DEVICE_KEYBOARD:
            return id < in.keys.size() && in.keys[id] ? 1 : 0;
        case RETRO_DEVICE_MOUSE:
            switch (id) {
                case RETRO_DEVICE_ID_MOUSE_X: return in.relX;
                case RETRO_DEVICE_ID_MOUSE_Y: return in.relY;
                case RETRO_DEVICE_ID_MOUSE_LEFT: return in.mouse[0] ? 1 : 0;
                case RETRO_DEVICE_ID_MOUSE_RIGHT: return in.mouse[1] ? 1 : 0;
                case RETRO_DEVICE_ID_MOUSE_MIDDLE: return in.mouse[2] ? 1 : 0;
                case RETRO_DEVICE_ID_MOUSE_WHEELUP: return in.wheel > 0 ? 1 : 0;
                case RETRO_DEVICE_ID_MOUSE_WHEELDOWN: return in.wheel < 0 ? 1 : 0;
                default: return 0;
            }
        case RETRO_DEVICE_POINTER:
            if (index) return id == RETRO_DEVICE_ID_POINTER_COUNT ? 0 : 0;
            if (id == RETRO_DEVICE_ID_POINTER_X) return in.absX;
            if (id == RETRO_DEVICE_ID_POINTER_Y) return in.absY;
            if (id == RETRO_DEVICE_ID_POINTER_PRESSED) return in.mouse[0] ? 1 : 0;
            if (id == RETRO_DEVICE_ID_POINTER_COUNT) return in.mouse[0] ? 1 : 0;
            return 0;
        case RETRO_DEVICE_JOYPAD:
            if (id == RETRO_DEVICE_ID_JOYPAD_MASK) {
                uint16_t mask = 0;
                for (unsigned i = 0; i < in.joy.size(); ++i) if (in.joy[i]) mask |= 1u << i;
                return static_cast<int16_t>(mask);
            }
            return id < in.joy.size() && in.joy[id] ? 1 : 0;
        case RETRO_DEVICE_ANALOG:
            if (index == RETRO_DEVICE_INDEX_ANALOG_LEFT) {
                if (id == RETRO_DEVICE_ID_ANALOG_X) return in.joyX;
                if (id == RETRO_DEVICE_ID_ANALOG_Y) return in.joyY;
            }
            if (index == RETRO_DEVICE_INDEX_ANALOG_BUTTON && id < in.joy.size())
                return in.joy[id] ? AXIS_MAX : 0;
            return 0;
        default:
            return 0;
    }
}

int16_t axis(float value) {
    value = std::max(-1.0f, std::min(1.0f, value));
    return static_cast<int16_t>(std::lround(value * AXIS_MAX));
}
int16_t pointer(float value) {
    value = std::max(0.0f, std::min(1.0f, value));
    return static_cast<int16_t>(std::lround((value * 2.0f - 1.0f) * AXIS_MAX));
}
int16_t relative(float value) {
    value = std::max(-static_cast<float>(AXIS_MAX),
                     std::min(static_cast<float>(AXIS_MAX), value));
    return static_cast<int16_t>(std::lround(value));
}
void clearBuffers(Host* host) {
    std::lock_guard<std::mutex> frameLock(host->frameMutex);
    host->frame.clear(); host->width = host->height = 0; host->frameSerial = 0;
    std::lock_guard<std::mutex> audioLock(host->audioMutex);
    host->audio.clear();
}

jlong nCreate(JNIEnv* env, jclass, jstring system, jstring save) {
    std::string sys = jstr(env, system), sav = jstr(env, save);
    if (sys.empty() || sav.empty()) return 0;
    std::lock_guard<std::mutex> lifecycle(gLifecycle);
    if (gHost || retro_api_version() != RETRO_API_VERSION) return 0;
    auto* host = new Host(std::move(sys), std::move(sav));
    gHost = host;
    retro_set_environment(environment);
    retro_set_video_refresh(video);
    retro_set_audio_sample(audioOne);
    retro_set_audio_sample_batch(audioBatch);
    retro_set_input_poll(inputPoll);
    retro_set_input_state(inputState);
    retro_init();
    host->initialized = true;
    return handleOf(host);
}
jboolean nLoad(JNIEnv* env, jclass, jlong handle, jstring pathValue) {
    Host* host = hostFrom(handle); if (!host) return JNI_FALSE;
    std::string path = jstr(env, pathValue); if (path.empty()) return JNI_FALSE;
    if (host->loaded) { retro_unload_game(); host->loaded = false; }
    host->content = std::move(path); host->contentDir = parentDir(host->content);
    host->shutdown = false; host->error.clear(); clearBuffers(host);
    retro_game_info info{}; info.path = host->content.c_str();
    if (!retro_load_game(&info)) {
        host->error = "DOSBox Pure rejected the selected content."; return JNI_FALSE;
    }
    host->loaded = true;
    retro_system_av_info av{}; retro_get_system_av_info(&av); applyAv(host, &av);
    return JNI_TRUE;
}
void nUpdateInput(JNIEnv* env, jclass, jlong handle, jintArray map, jbooleanArray keys,
                  jbooleanArray mouse, jfloat relX, jfloat relY, jfloat absX, jfloat absY,
                  jfloat wheel, jfloat joyX, jfloat joyY, jbooleanArray joy) {
    Host* host = hostFrom(handle); if (!host) return;
    std::lock_guard<std::mutex> lock(host->inputMutex);
    InputState& in = host->liveInput;
    in.keys.fill(0); in.mouse.fill(false); in.joy.fill(false);
    if (map && keys) {
        jsize count = std::min(env->GetArrayLength(map), env->GetArrayLength(keys));
        std::vector<jint> codes(count); std::vector<jboolean> pressed(count);
        if (count) { env->GetIntArrayRegion(map, 0, count, codes.data());
                     env->GetBooleanArrayRegion(keys, 0, count, pressed.data()); }
        for (jsize i = 0; i < count; ++i)
            if (codes[i] > RETROK_UNKNOWN && codes[i] < RETROK_LAST && pressed[i])
                in.keys[static_cast<size_t>(codes[i])] = 1;
    }
    if (mouse) {
        jsize count = std::min<jsize>(3, env->GetArrayLength(mouse));
        jboolean values[3]{}; if (count) env->GetBooleanArrayRegion(mouse, 0, count, values);
        for (jsize i = 0; i < count; ++i) in.mouse[i] = values[i];
    }
    if (joy) {
        jsize count = std::min<jsize>(16, env->GetArrayLength(joy));
        jboolean values[16]{}; if (count) env->GetBooleanArrayRegion(joy, 0, count, values);
        for (jsize i = 0; i < count; ++i) in.joy[i] = values[i];
    }
    in.relX = relative(relX); in.relY = relative(relY);
    in.absX = pointer(absX); in.absY = pointer(absY);
    in.wheel = wheel > 0 ? 1 : (wheel < 0 ? -1 : 0);
    in.joyX = axis(joyX); in.joyY = axis(joyY);
}
void nRelease(JNIEnv*, jclass, jlong handle) {
    Host* host = hostFrom(handle); if (!host) return;
    std::lock_guard<std::mutex> lock(host->inputMutex); host->liveInput.release();
}
void nRun(JNIEnv*, jclass, jlong handle) {
    Host* host = hostFrom(handle); if (!host || !host->loaded) return;
    {
        std::lock_guard<std::mutex> lock(host->inputMutex);
        host->frameInput = host->liveInput;
        host->liveInput.clearTransient();
    }
    retro_run();
}
jdouble nFps(JNIEnv*, jclass, jlong h) { Host* x = hostFrom(h); return x ? x->fps : 0.0; }
jdouble nRate(JNIEnv*, jclass, jlong h) { Host* x = hostFrom(h); return x ? x->sampleRate : 0.0; }
jint nWidth(JNIEnv*, jclass, jlong h) {
    Host* x = hostFrom(h); if (!x) return 0; std::lock_guard<std::mutex> lock(x->frameMutex); return x->width;
}
jint nHeight(JNIEnv*, jclass, jlong h) {
    Host* x = hostFrom(h); if (!x) return 0; std::lock_guard<std::mutex> lock(x->frameMutex); return x->height;
}
jint nFrameBytes(JNIEnv*, jclass, jlong h) {
    Host* x = hostFrom(h); if (!x) return 0; std::lock_guard<std::mutex> lock(x->frameMutex);
    return static_cast<jint>(std::min(x->frame.size(), static_cast<size_t>(std::numeric_limits<jint>::max())));
}
jlong nSerial(JNIEnv*, jclass, jlong h) {
    Host* x = hostFrom(h); if (!x) return 0; std::lock_guard<std::mutex> lock(x->frameMutex); return static_cast<jlong>(x->frameSerial);
}
jint nCopyFrame(JNIEnv* env, jclass, jlong h, jobject target, jint capacity) {
    Host* x = hostFrom(h); if (!x || !target || capacity <= 0) return 0;
    void* dst = env->GetDirectBufferAddress(target); jlong cap = env->GetDirectBufferCapacity(target);
    if (!dst || cap <= 0) { x->error = "Frame target must be a direct ByteBuffer."; return 0; }
    std::lock_guard<std::mutex> lock(x->frameMutex);
    size_t bytes = std::min(x->frame.size(), static_cast<size_t>(std::min<jlong>(cap, capacity)));
    if (bytes) std::memcpy(dst, x->frame.data(), bytes);
    return static_cast<jint>(bytes);
}
jint nReadAudio(JNIEnv* env, jclass, jlong h, jobject target, jint maxFrames) {
    Host* x = hostFrom(h); if (!x || !target || maxFrames <= 0) return 0;
    auto* dst = static_cast<int16_t*>(env->GetDirectBufferAddress(target));
    jlong cap = env->GetDirectBufferCapacity(target);
    if (!dst || cap <= 0) { x->error = "Audio target must be a direct ByteBuffer."; return 0; }
    size_t frames = std::min(static_cast<size_t>(maxFrames), static_cast<size_t>(cap) / 4u);
    std::lock_guard<std::mutex> lock(x->audioMutex);
    return static_cast<jint>(x->audio.pop(dst, frames));
}
jboolean nShutdown(JNIEnv*, jclass, jlong h) { Host* x = hostFrom(h); return x && x->shutdown ? JNI_TRUE : JNI_FALSE; }
jstring nError(JNIEnv* env, jclass, jlong h) { Host* x = hostFrom(h); return env->NewStringUTF(x ? x->error.c_str() : ""); }
void nDestroy(JNIEnv*, jclass, jlong handle) {
    std::lock_guard<std::mutex> lifecycle(gLifecycle);
    Host* host = hostFrom(handle); if (!host) return;
    if (host->loaded) retro_unload_game();
    if (host->initialized) retro_deinit();
    gHost = nullptr; delete host;
}

#define NATIVE(name, sig, fn) {const_cast<char*>(name), const_cast<char*>(sig), reinterpret_cast<void*>(fn)}
const JNINativeMethod METHODS[] = {
    NATIVE("nativeCreate", "(Ljava/lang/String;Ljava/lang/String;)J", nCreate),
    NATIVE("nativeLoadContent", "(JLjava/lang/String;)Z", nLoad),
    NATIVE("nativeUpdateInput", "(J[I[Z[ZFFFFFFF[Z)V", nUpdateInput),
    NATIVE("nativeReleaseAllInput", "(J)V", nRelease),
    NATIVE("nativeRunFrame", "(J)V", nRun),
    NATIVE("nativeGetFramesPerSecond", "(J)D", nFps),
    NATIVE("nativeGetAudioSampleRate", "(J)D", nRate),
    NATIVE("nativeGetFrameWidth", "(J)I", nWidth),
    NATIVE("nativeGetFrameHeight", "(J)I", nHeight),
    NATIVE("nativeGetFrameByteCount", "(J)I", nFrameBytes),
    NATIVE("nativeGetFrameSerial", "(J)J", nSerial),
    NATIVE("nativeCopyLatestFrame", "(JLjava/nio/ByteBuffer;I)I", nCopyFrame),
    NATIVE("nativeReadAudio", "(JLjava/nio/ByteBuffer;I)I", nReadAudio),
    NATIVE("nativeIsShutdownRequested", "(J)Z", nShutdown),
    NATIVE("nativeGetLastError", "(J)Ljava/lang/String;", nError),
    NATIVE("nativeDestroy", "(J)V", nDestroy),
};
#undef NATIVE
} // namespace

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK || !env)
        return JNI_ERR;
    jclass cls = env->FindClass("io/github/mrcalzon02/reverievr/DosNativeRuntime");
    if (!cls) return JNI_ERR;
    if (env->RegisterNatives(cls, METHODS, static_cast<jint>(sizeof(METHODS) / sizeof(METHODS[0]))) != JNI_OK)
        return JNI_ERR;
    return JNI_VERSION_1_6;
}
