#!/usr/bin/env bash
# Source-only Sitka terrain cross-compile gate. This is intentionally not
# an Android.mk module, an installed APK asset, or a playable game.
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
version="27.2.12479018"
if [[ "$(uname -s)" != "Linux" || "$(uname -m)" != "x86_64" ]]; then
  echo 'Sitka NDK validation requires a Linux x86_64 host' >&2
  exit 2
fi
ndk="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}"
if [[ -z "$ndk" || ! -x "$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++" ]]; then
  if [[ -z "${ANDROID_SDK_ROOT:-}" ]]; then
    echo 'Set ANDROID_NDK_HOME or ANDROID_SDK_ROOT' >&2
    exit 2
  fi
  ndk="$ANDROID_SDK_ROOT/ndk/$version"
  if [[ ! -x "$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++" ]]; then
    sdkmanager="$ANDROID_SDK_ROOT/cmdline-tools/latest/bin/sdkmanager"
    if [[ ! -x "$sdkmanager" ]]; then
      echo 'Android SDK command-line tools are unavailable' >&2
      exit 2
    fi
    "$sdkmanager" --install "ndk;$version"
  fi
fi
bin="$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin"
if [[ ! -x "$bin/llvm-nm" || ! -x "$bin/llvm-readelf" ]]; then
  echo 'NDK LLVM inspection tools unavailable' >&2
  exit 2
fi
out="${SITKA_NDK_OUTPUT_DIR:-${RUNNER_TEMP:-/tmp}/sitka-ndk-terrain}"
mkdir -p "$out"
for item in 'armv7a-linux-androideabi21 ARM' 'aarch64-linux-android21 AArch64'; do
  read -r target arch <<<"$item"
  compiler="$bin/${target}-clang++"
  if [[ ! -x "$compiler" ]]; then
    echo "Missing NDK C++ driver: $compiler" >&2
    exit 2
  fi
  library="$out/libreverie_sitka_terrain_${target}.so"
  "$compiler" -std=c++17 -Wall -Wextra -Werror -Wpedantic -fPIC \
    -fexceptions -fvisibility=hidden -shared \
    -Wl,--no-undefined -Wl,-z,max-page-size=16384 \
    -I"$root/app/src/main/jni/native" \
    "$root/app/src/main/jni/native/sitka_terrain_module.cpp" \
    -o "$library" -lGLESv2 -lm
  "$bin/llvm-readelf" -h "$library" | grep -Eq "Machine: *${arch}($|[[:space:]])"
  "$bin/llvm-nm" -D --defined-only "$library" | \
    grep -Eq '[[:space:]]T[[:space:]]reverie_native_module_entry_v1$'
  echo "PASS: Sitka source-only $target linked with exported native entry"
done
echo 'PASS: NDK source proof only, not a registered or packaged Sitka module'
