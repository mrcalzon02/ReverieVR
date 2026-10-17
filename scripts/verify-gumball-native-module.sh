#!/usr/bin/env bash
# Build and link the actual game-private native module for both Android ABIs.
# Unlike header/unit tests, this catches renderer, SDK, GLES2 and link errors.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
NDK="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}"
if [[ -z "$NDK" || ! -x "$NDK/ndk-build" ]]; then
    echo "ERROR: Android NDK ndk-build not found; set ANDROID_NDK_HOME (or ANDROID_NDK_ROOT)." >&2
    exit 2
fi
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
"$NDK/ndk-build" \
    NDK_PROJECT_PATH="$ROOT" \
    APP_BUILD_SCRIPT="$ROOT/app/src/main/jni/Android.mk" \
    NDK_APPLICATION_MK="$ROOT/app/src/main/jni/Application.mk" \
    APP_MODULES=reverie_module_gumball_armada \
    'APP_ABI=armeabi-v7a arm64-v8a' \
    NDK_OUT="$WORK/obj" NDK_LIBS_OUT="$WORK/libs" -j2
READ_ELF=""
for platform in "$NDK"/toolchains/llvm/prebuilt/*; do
    if [[ -x "$platform/bin/llvm-readelf" ]]; then
        READ_ELF="$platform/bin/llvm-readelf"
        break
    fi
done
if [[ -z "$READ_ELF" ]]; then
    echo "ERROR: NDK llvm-readelf missing; cannot verify exported module entry." >&2
    exit 2
fi
for abi in armeabi-v7a arm64-v8a; do
    so="$WORK/libs/$abi/libreverie_module_gumball_armada.so"
    if [[ ! -s "$so" ]]; then
        echo "ERROR: missing or empty $abi Gumball native library" >&2
        exit 1
    fi
    if ! "$READ_ELF" --dyn-syms "$so" | grep -q 'reverie_native_module_entry_v1'; then
        echo "ERROR: $abi library lacks exported native module entry v1" >&2
        exit 1
    fi
    echo "PASS: $abi Gumball GLES2 module compiled, linked and exports module entry"
done
