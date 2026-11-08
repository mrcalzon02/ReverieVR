// Counts dispatches from the production Java runtime. This is deliberately
// not the production native host and cannot prove native/Android execution.
#include <jni.h>
namespace { int counts[5]{}; bool ready=true,primary=false; float delta=0,move_x=0; }
#define RUNTIME(name) Java_io_github_mrcalzon02_reverievr_NativeModuleRuntime_##name
#define PROBE(name) Java_io_github_mrcalzon02_reverievr_NativeRuntimePauseProbe_##name
extern "C" {
JNIEXPORT jlong JNICALL RUNTIME(nativeCreate)(JNIEnv*,jclass,jstring,jstring,jobject,jstring,jintArray){++counts[0];return 1;}
JNIEXPORT jfloatArray JNICALL RUNTIME(nativeGetShellLocomotionBounds)(JNIEnv*,jclass,jlong){return nullptr;}
JNIEXPORT void JNICALL RUNTIME(nativeResume)(JNIEnv*,jclass,jlong){++counts[1];}
JNIEXPORT void JNICALL RUNTIME(nativePause)(JNIEnv*,jclass,jlong){++counts[2];}
JNIEXPORT void JNICALL RUNTIME(nativeDestroy)(JNIEnv*,jclass,jlong){++counts[4];}
JNIEXPORT jboolean JNICALL RUNTIME(nativePrepareExit)(JNIEnv*,jclass,jlong){return ready?JNI_TRUE:JNI_FALSE;}
JNIEXPORT jstring JNICALL RUNTIME(nativeGetLastError)(JNIEnv* e,jclass){return e->NewStringUTF("test save failure");}
JNIEXPORT jint JNICALL RUNTIME(nativeUpdate)(JNIEnv*,jclass,jlong,jfloat dt,
    jfloat x,jfloat,jboolean pressed,jboolean,jint,jfloat,jfloat,jfloat,jfloat,jfloat,jfloat,
    jboolean,jfloat,jfloat,jfloat,jfloat,jfloat){++counts[3];delta=dt;primary=pressed;move_x=x;return 0;}
JNIEXPORT jint JNICALL PROBE(calls)(JNIEnv*,jclass,jint kind){return counts[kind];}
JNIEXPORT void JNICALL PROBE(allowExit)(JNIEnv*,jclass,jboolean allow){ready=allow;}
JNIEXPORT jfloat JNICALL PROBE(lastDelta)(JNIEnv*,jclass){return delta;}
JNIEXPORT jboolean JNICALL PROBE(lastPrimary)(JNIEnv*,jclass){return primary?JNI_TRUE:JNI_FALSE;}
JNIEXPORT jfloat JNICALL PROBE(lastMoveX)(JNIEnv*,jclass){return move_x;}
}
