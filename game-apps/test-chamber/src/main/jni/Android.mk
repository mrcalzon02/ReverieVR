LOCAL_PATH := $(call my-dir)
PROBE_ROOT := $(abspath $(LOCAL_PATH)/../../../../..)
include $(CLEAR_VARS)
LOCAL_MODULE := reverie_companion_test_chamber
LOCAL_SRC_FILES := \
    companion_test_chamber_bridge.cpp \
    companion_showcase_renderer.cpp \
    ../../../../../app/src/main/jni/native/test_chamber_module.cpp \
    ../../../../../third_party/openktg/src/gentexture.cpp
LOCAL_C_INCLUDES := \
    $(PROBE_ROOT)/app/src/main/jni/native \
    $(PROBE_ROOT)/third_party/openktg/src
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -fexceptions \
    -fvisibility=hidden
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384
LOCAL_LDLIBS += -llog -lGLESv2 -lm
include $(BUILD_SHARED_LIBRARY)
