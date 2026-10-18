LOCAL_PATH := $(call my-dir)
COMPANION_ROOT := $(abspath $(LOCAL_PATH)/../../../../..)
include $(CLEAR_VARS)
LOCAL_MODULE := reverie_companion_breakwater
LOCAL_SRC_FILES := \
    companion_breakwater_bridge.cpp \
    ../../../../../app/src/main/jni/native/breakwater_module.cpp \
    ../../../../../app/src/main/jni/native/breakwater_campaign.cpp
LOCAL_C_INCLUDES := $(COMPANION_ROOT)/app/src/main/jni/native
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -fexceptions \
    -fvisibility=hidden
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384
LOCAL_LDLIBS += -llog -lGLESv2 -landroid -lm -lz
include $(BUILD_SHARED_LIBRARY)
