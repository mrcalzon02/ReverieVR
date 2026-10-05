LOCAL_PATH := $(call my-dir)
REVERIE_JNI_PATH := $(LOCAL_PATH)
REVERIE_ROOT := $(abspath $(REVERIE_JNI_PATH)/../../../..)
DOSBOX_PURE_ROOT := $(REVERIE_ROOT)/third_party/dosbox-pure/src

ifeq ($(wildcard $(DOSBOX_PURE_ROOT)/jni/Android.mk),)
$(error DOSBox Pure source is missing. Run scripts/fetch-dosbox-pure.sh or the Windows .bat helper.)
endif

include $(DOSBOX_PURE_ROOT)/jni/Android.mk

LOCAL_PATH := $(REVERIE_JNI_PATH)
include $(CLEAR_VARS)

LOCAL_MODULE := reverie_dos_host
LOCAL_SRC_FILES := reverie_dos_host.cpp
LOCAL_C_INCLUDES := $(DOSBOX_PURE_ROOT)/libretro-common/include
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -fexceptions
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384
LOCAL_LDLIBS += -llog
LOCAL_SHARED_LIBRARIES := retro

include $(BUILD_SHARED_LIBRARY)
