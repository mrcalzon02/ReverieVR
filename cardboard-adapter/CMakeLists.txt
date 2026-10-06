cmake_minimum_required(VERSION 3.22.1)
project(reverie_cardboard_adapter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(
    CARDBOARD_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../third_party/cardboard"
)

# ReverieVR needs the Cardboard Java runtime and GLES2 renderer only.
# Keep these in the cache before importing the pinned upstream SDK target.
set(
    CARDBOARDSDK_UNITY_PLUGIN
    OFF
    CACHE BOOL
    "Disable Cardboard Unity plugin sources"
    FORCE
)
set(
    CARDBOARDSDK_RENDERING_VULKAN
    OFF
    CACHE BOOL
    "Disable Cardboard Vulkan rendering"
    FORCE
)
set(
    CARDBOARDSDK_RENDERING_GLESv3
    OFF
    CACHE BOOL
    "Disable Cardboard GLES3 rendering"
    FORCE
)

add_subdirectory(
    "${CARDBOARD_ROOT}/sdk"
    cardboard-sdk
)

find_library(ANDROID_LIB android)
find_library(GLESv2_LIB GLESv2)
find_library(LOG_LIB log)

# The Cardboard Java API calls System.loadLibrary("cardboard_sdk_jni").
# Upstream's SDK Gradle module includes the Java API but does not build this
# bridge target. Google's Paperscope sample builds it explicitly; ReverieVR
# does the same here so the Java API is actually runnable in the APK.
add_library(
    cardboard_sdk_jni
    SHARED
    "${CARDBOARD_ROOT}/java_api/src/main/jni/cardboard_jni.cc"
    "${CARDBOARD_ROOT}/sdk/jni_utils/android/jni_utils.cc"
)

target_include_directories(
    cardboard_sdk_jni
    PRIVATE
    "${CARDBOARD_ROOT}/sdk"
)

target_link_libraries(
    cardboard_sdk_jni
    PRIVATE
    GfxPluginCardboard
    ${ANDROID_LIB}
    ${LOG_LIB}
    ${GLESv2_LIB}
)

target_link_options(
    cardboard_sdk_jni
    PRIVATE
    "-Wl,-z,max-page-size=16384"
)
