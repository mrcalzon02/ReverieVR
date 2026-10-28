#!/usr/bin/env bash
# Source-only Sitka terrain cross-link proof. Never registers a game or ships
# these binaries in an APK. Use the same known NDK revision on every runner.
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
version="27.3.13750724"
if [[ "$(uname -s)" != "Linux" || "$(uname -m)" != "x86_64" ]]; then
  echo 'Sitka NDK validation requires a Linux x86_64 host' >&2
  exit 2
fi

sdk="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
ndk="${SITKA_NDK_HOME:-}"
if [[ -z "$ndk" ]]; then
  # The hosted ubuntu-24.04 image includes this pinned NDK. Do not silently
  # accept an arbitrary ANDROID_NDK_HOME or try an unobserved network install.
  for candidate in "${sdk:+$sdk/ndk/$version}" "${ANDROID_NDK_HOME:-}" "${ANDROID_NDK_ROOT:-}"; do
    [[ -n "$candidate" && -f "$candidate/source.properties" ]] || continue
    if grep -Eq "^[[:space:]]*Pkg.Revision[[:space:]]*=[[:space:]]*$version[[:space:]]*$" "$candidate/source.properties"; then
      ndk="$candidate"
      break
    fi
  done
fi
if [[ -z "$ndk" || ! -f "$ndk/source.properties" ]] ||
   ! grep -Eq "^[[:space:]]*Pkg.Revision[[:space:]]*=[[:space:]]*$version[[:space:]]*$" "$ndk/source.properties"; then
  echo "Missing pinned Android NDK $version (SDK root: ${sdk:-unset}; override: SITKA_NDK_HOME)." >&2
  echo "Install ndk;$version before running this source-only gate." >&2
  exit 2
fi
bin="$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin"
for tool in llvm-nm llvm-readelf; do
  if [[ ! -x "$bin/$tool" ]]; then
    echo "Missing NDK LLVM inspection tool: $bin/$tool" >&2
    exit 2
  fi
done
echo "Sitka source-only NDK revision: $version ($ndk)"
out="${SITKA_NDK_OUTPUT_DIR:-${RUNNER_TEMP:-/tmp}/sitka-ndk-terrain}"
mkdir -p "$out"
for item in 'armv7a-linux-androideabi21 ARM ELF32' 'aarch64-linux-android21 AArch64 ELF64'; do
  read -r target arch elf_class <<<"$item"
  compiler="$bin/${target}-clang++"
  if [[ ! -x "$compiler" ]]; then
    echo "Missing NDK C++ driver: $compiler" >&2
    exit 2
  fi
  library="$out/libreverie_sitka_terrain_${target}.so"
  echo "Linking $target with GLES2 and no undefined symbols..."
  "$compiler" -std=c++17 -Wall -Wextra -Werror -Wpedantic -fPIC \
    -fexceptions -fvisibility=hidden -shared \
    -Wl,--no-undefined -Wl,-z,max-page-size=16384 \
    -I"$root/app/src/main/jni/native" \
    "$root/app/src/main/jni/native/sitka_terrain_module.cpp" \
    -o "$library" -lGLESv2 -lm
  elf_header="$("$bin/llvm-readelf" -h "$library")"
  if ! grep -Eq "Machine: *${arch}($|[[:space:]])" <<<"$elf_header" ||
     ! grep -Eq "Class: *${elf_class}($|[[:space:]])" <<<"$elf_header" ||
     ! grep -Eq 'Type: *DYN ' <<<"$elf_header"; then
    echo "Unexpected ELF machine/class/type for $target:" >&2
    printf '%s\n' "$elf_header" >&2
    exit 1
  fi
  exports="$("$bin/llvm-nm" -D --defined-only "$library")"
  if ! grep -Eq '[[:space:]]T[[:space:]]reverie_native_module_entry_v1$' <<<"$exports"; then
    echo "Missing public module entry in $library" >&2
    printf '%s\n' "$exports" >&2
    exit 1
  fi
  dynamic="$("$bin/llvm-readelf" -d "$library")"
  if ! grep -Fq 'Shared library: [libGLESv2.so]' <<<"$dynamic"; then
    echo "Sitka $target is not dynamically linked to Android GLES2" >&2
    printf '%s\n' "$dynamic" >&2
    exit 1
  fi
  sha256sum "$library"
  echo "PASS: $target $elf_class $arch GLES2, exported native entry"
done
echo 'PASS: NDK source proof only, not a registered or packaged Sitka module'
