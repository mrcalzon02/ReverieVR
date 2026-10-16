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
LOCAL_LDLIBS += -llog -ldl -lGLESv2

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

LOCAL_PATH := $(REVERIE_JNI_PATH)
include $(CLEAR_VARS)

LOCAL_MODULE := reverie_module_red_ledger
LOCAL_SRC_FILES := \
    native/red_ledger_module.cpp \
    native/red_ledger_simulation.cpp \
    native/procedural_material_atlas.cpp \
    native/red_ledger_static_geometry.cpp
LOCAL_C_INCLUDES := $(REVERIE_JNI_PATH)/native
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -fexceptions -fvisibility=hidden
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384
LOCAL_LDLIBS += -lGLESv2 -lm

include $(BUILD_SHARED_LIBRARY)

# Development-only recon project boards; existing ABI v1 host and kernels.
LOCAL_PATH := $(REVERIE_JNI_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE := reverie_module_lantern_desk
LOCAL_SRC_FILES := native/recon_projects_module.cpp native/lantern_desk_simulation.cpp
LOCAL_C_INCLUDES := $(REVERIE_JNI_PATH)/native
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -fexceptions -fvisibility=hidden -DRECON_PROJECT_KIND=0
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384
LOCAL_LDLIBS += -lGLESv2 -lm
include $(BUILD_SHARED_LIBRARY)

LOCAL_PATH := $(REVERIE_JNI_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE := reverie_module_forward_detachment
LOCAL_SRC_FILES := native/recon_projects_module.cpp native/forward_detachment_simulation.cpp
LOCAL_C_INCLUDES := $(REVERIE_JNI_PATH)/native
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -fexceptions -fvisibility=hidden -DRECON_PROJECT_KIND=1
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384
LOCAL_LDLIBS += -lGLESv2 -lm
include $(BUILD_SHARED_LIBRARY)

# Development-only seated-defense native module
LOCAL_PATH := $(REVERIE_JNI_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE := reverie_module_breakwater
LOCAL_SRC_FILES := native/breakwater_module.cpp native/breakwater_campaign.cpp
LOCAL_C_INCLUDES := $(REVERIE_JNI_PATH)/native
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -fexceptions -fvisibility=hidden
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384
LOCAL_LDLIBS += -lGLESv2 -lm
include $(BUILD_SHARED_LIBRARY)

# Candy-space flight campaign; single APK / no new runtime dependency.
LOCAL_PATH := $(REVERIE_JNI_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE := reverie_module_gumball_armada
LOCAL_SRC_FILES := native/gumball_armada_module.cpp
LOCAL_C_INCLUDES := $(REVERIE_JNI_PATH)/native
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -fexceptions -fvisibility=hidden
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384
LOCAL_LDLIBS += -lGLESv2 -lm
include $(BUILD_SHARED_LIBRARY)

# Omega Front Development-only native war table.
LOCAL_PATH := $(REVERIE_JNI_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE := reverie_module_omega_front
LOCAL_SRC_FILES := native/omega_front_module.cpp
LOCAL_C_INCLUDES := $(REVERIE_JNI_PATH)/native
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -fexceptions -fvisibility=hidden
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384
LOCAL_LDLIBS += -lGLESv2 -lm
include $(BUILD_SHARED_LIBRARY)

# Original low-allocation GLES2 classic game module
LOCAL_PATH := $(REVERIE_JNI_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE := reverie_module_classic_pong
LOCAL_SRC_FILES := native/classic_games_module.cpp
LOCAL_C_INCLUDES := $(REVERIE_JNI_PATH)/native
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -fexceptions -fvisibility=hidden -DCLASSIC_GAME_KIND=0
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384
LOCAL_LDLIBS += -lGLESv2 -lm
include $(BUILD_SHARED_LIBRARY)

# Original low-allocation GLES2 classic game module
LOCAL_PATH := $(REVERIE_JNI_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE := reverie_module_classic_snake
LOCAL_SRC_FILES := native/classic_games_module.cpp
LOCAL_C_INCLUDES := $(REVERIE_JNI_PATH)/native
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -fexceptions -fvisibility=hidden -DCLASSIC_GAME_KIND=1
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384
LOCAL_LDLIBS += -lGLESv2 -lm
include $(BUILD_SHARED_LIBRARY)

# Original low-allocation GLES2 classic game module
LOCAL_PATH := $(REVERIE_JNI_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE := reverie_module_classic_chutes_ladders
LOCAL_SRC_FILES := native/classic_games_module.cpp
LOCAL_C_INCLUDES := $(REVERIE_JNI_PATH)/native
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -fexceptions -fvisibility=hidden -DCLASSIC_GAME_KIND=2
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384
LOCAL_LDLIBS += -lGLESv2 -lm
include $(BUILD_SHARED_LIBRARY)


# Original low-poly board game; no extra runtime or textures.
LOCAL_PATH := $(REVERIE_JNI_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE := reverie_module_classic_checkers
LOCAL_SRC_FILES := native/classic_games_module.cpp
LOCAL_C_INCLUDES := $(REVERIE_JNI_PATH)/native
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -fexceptions -fvisibility=hidden -DCLASSIC_GAME_KIND=3
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384
LOCAL_LDLIBS += -lGLESv2 -lm
include $(BUILD_SHARED_LIBRARY)


# Original low-poly board game; no extra runtime or textures.
LOCAL_PATH := $(REVERIE_JNI_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE := reverie_module_classic_go
LOCAL_SRC_FILES := native/classic_games_module.cpp
LOCAL_C_INCLUDES := $(REVERIE_JNI_PATH)/native
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -fexceptions -fvisibility=hidden -DCLASSIC_GAME_KIND=4
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384
LOCAL_LDLIBS += -lGLESv2 -lm
include $(BUILD_SHARED_LIBRARY)


# Original low-poly board game; no extra runtime or textures.
LOCAL_PATH := $(REVERIE_JNI_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE := reverie_module_classic_chess
LOCAL_SRC_FILES := native/classic_games_module.cpp
LOCAL_C_INCLUDES := $(REVERIE_JNI_PATH)/native
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -fexceptions -fvisibility=hidden -DCLASSIC_GAME_KIND=5
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384
LOCAL_LDLIBS += -lGLESv2 -lm
include $(BUILD_SHARED_LIBRARY)

# Original GLES2 Breakout rules and colored geometry; game-private.
LOCAL_PATH := $(REVERIE_JNI_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE := reverie_module_classic_breakout
LOCAL_SRC_FILES := native/classic_games_module.cpp
LOCAL_C_INCLUDES := $(REVERIE_JNI_PATH)/native
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -fexceptions -fvisibility=hidden -DCLASSIC_GAME_KIND=6
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384
LOCAL_LDLIBS += -lGLESv2 -lm
include $(BUILD_SHARED_LIBRARY)

# Full 8x8 native Reversi with legal flips and bounded CPU search.
LOCAL_PATH := $(REVERIE_JNI_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE := reverie_module_classic_reversi
LOCAL_SRC_FILES := native/classic_games_module.cpp
LOCAL_C_INCLUDES := $(REVERIE_JNI_PATH)/native
LOCAL_CPPFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -fexceptions -fvisibility=hidden -DCLASSIC_GAME_KIND=7
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384
LOCAL_LDLIBS += -lGLESv2 -lm
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
