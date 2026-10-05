LOCAL_PATH := $(call my-dir)
REVERIE_JNI_PATH := $(LOCAL_PATH)
REVERIE_ROOT := $(abspath $(REVERIE_JNI_PATH)/../../../..)

DOSBOX_PURE_ROOT := $(REVERIE_ROOT)/third_party/dosbox-pure/src
OPENKTG_ROOT := $(REVERIE_ROOT)/third_party/openktg/src

ifneq ($(wildcard $(DOSBOX_PURE_ROOT)/jni/Android.mk),)
REVERIE_HAS_DOSBOX_PURE := 1
include $(DOSBOX_PURE_ROOT)/jni/Android.mk
endif

LOCAL_PATH := $(REVERIE_JNI_PATH)

include $(CLEAR_VARS)

LOCAL_MODULE := reverie_native_host
LOCAL_SRC_FILES := native/reverie_native_host.cpp
LOCAL_C_INCLUDES := $(REVERIE_JNI_PATH)/native
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -fexceptions
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384
LOCAL_LDLIBS += -llog -ldl

include $(BUILD_SHARED_LIBRARY)

LOCAL_PATH := $(REVERIE_JNI_PATH)
include $(CLEAR_VARS)

LOCAL_MODULE := reverie_module_test_chamber
LOCAL_SRC_FILES := \
    native/test_chamber_module.cpp \
    ../../../../third_party/openktg/src/gentexture.cpp
LOCAL_C_INCLUDES := \
    $(REVERIE_JNI_PATH)/native \
    $(OPENKTG_ROOT)
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -fexceptions -fvisibility=hidden
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384
LOCAL_LDLIBS += -llog -lGLESv2

include $(BUILD_SHARED_LIBRARY)

ifdef REVERIE_HAS_DOSBOX_PURE

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

endif
