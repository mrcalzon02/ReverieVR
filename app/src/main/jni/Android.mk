LOCAL_PATH := $(call my-dir)
REVERIE_JNI_DIR := $(LOCAL_PATH)
REVERIE_ROOT := $(abspath $(REVERIE_JNI_DIR)/../../../..)
DOSBOX_PURE_DIR := $(REVERIE_ROOT)/third_party/dosbox-pure/src

ifneq ($(wildcard $(DOSBOX_PURE_DIR)/dosbox_pure_libretro.cpp),)
include $(DOSBOX_PURE_DIR)/jni/Android.mk
else
$(error DOSBox Pure source is missing. Run scripts/fetch-dosbox-pure.sh or scripts\fetch-dosbox-pure.bat)
endif

LOCAL_PATH := $(REVERIE_JNI_DIR)

include $(CLEAR_VARS)
LOCAL_MODULE := reverie_dos_host
LOCAL_SRC_FILES := reverie_dos_host.cpp
LOCAL_C_INCLUDES := $(DOSBOX_PURE_DIR)/libretro-common/include
LOCAL_CPPFLAGS := -std=c++17 -fexceptions -frtti -Wall -Wextra
LOCAL_LDLIBS := -llog -ldl
include $(BUILD_SHARED_LIBRARY)
