#!/usr/bin/env python3
"""Reject APKs whose registered native games cannot link or expose ABI v1.

Checks the shipped ELF shared objects, not just the module names in source.
Does not execute Android binaries or assert that a game worked on a phone.
"""
from __future__ import annotations

import re
import struct
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HOST_SOURCE = ROOT / "app/src/main/jni/native/reverie_native_host.cpp"
ANDROID_MK = ROOT / "app/src/main/jni/Android.mk"
SONAME_RE = re.compile(r'"(libreverie_module_[a-z0-9_]+\.so)"')
MAKE_RE = re.compile(r"^\s*LOCAL_MODULE\s*:=\s*(reverie_module_[a-z0-9_]+)\s*$", re.M)
SYMBOL_RE = re.compile(
    r"^\s*\d+:\s+\S+\s+\d+\s+\w+\s+GLOBAL\s+DEFAULT\s+"
    r"(?!UND\b)\S+\s+(\S+)\s*$", re.M
)
NEEDED_RE = re.compile(r"\(NEEDED\)\s+Shared library: \[([^\]]+)\]")
ABIS = {"armeabi-v7a": (1, 40), "arm64-v8a": (2, 183)}
ENTRY_SYMBOL = "reverie_native_module_entry_v1"
HOST_JNI = {
    "Java_io_github_mrcalzon02_reverievr_NativeModuleRuntime_nativeCreate",
    "Java_io_github_mrcalzon02_reverievr_NativeModuleRuntime_nativeOnSurfaceCreated",
    "Java_io_github_mrcalzon02_reverievr_NativeModuleRuntime_nativeRenderEye",
}


def fail(message: str) -> None:
    raise ValueError(message)


def expected_modules() -> list[str]:
    host = HOST_SOURCE.read_text(encoding="utf-8")
    make = ANDROID_MK.read_text(encoding="utf-8")
    registered = SONAME_RE.findall(host)
    built = MAKE_RE.findall(make)
    modules = ["lib" + name + ".so" for name in built]
    if len(registered) < 11 or len(registered) != len(set(registered)):
        fail("native host module registry is incomplete or has duplicates")
    if len(built) != len(set(built)) or set(modules) != set(registered):
        fail("host module registry and Android.mk shared-library targets disagree")
    return sorted(registered)


def elf_symbols_and_dependencies(binary: bytes, label: str,
                                 abi: str, directory: Path) -> tuple[set[str], set[str]]:
    if len(binary) < 20 or binary[:4] != b"\x7fELF":
        fail(f"{label}: missing ELF header")
    expected_class, expected_machine = ABIS[abi]
    if binary[4] != expected_class or binary[5] != 1:
        fail(f"{label}: wrong ELF class or endianness for {abi}")
    elf_type, machine = struct.unpack_from("<HH", binary, 16)
    if elf_type != 3 or machine != expected_machine:
        fail(f"{label}: wrong ELF type or machine ({elf_type}, {machine})")
    destination = directory / ("elf_" + label.replace("/", "_"))
    destination.write_bytes(binary)
    symbol_result = subprocess.run(
        ["readelf", "-W", "--dyn-syms", str(destination)],
        check=True, capture_output=True, text=True, timeout=20
    )
    dep_result = subprocess.run(
        ["readelf", "-W", "-d", str(destination)],
        check=True, capture_output=True, text=True, timeout=20
    )
    symbols = set(SYMBOL_RE.findall(symbol_result.stdout))
    needed = set(NEEDED_RE.findall(dep_result.stdout))
    return symbols, needed


def validate(apk: Path) -> int:
    modules = expected_modules()
    inspected = 0
    with zipfile.ZipFile(apk) as archive, tempfile.TemporaryDirectory(
            prefix="reverie-native-elf-") as temp:
        entries = set(archive.namelist())
        names = ["libreverie_native_host.so", *modules]
        for abi in ABIS:
            for name in names:
                item = f"lib/{abi}/{name}"
                if item not in entries:
                    fail(f"{item}: registered library is missing from APK")
                binary = archive.read(item)
                symbols, dependencies = elf_symbols_and_dependencies(
                    binary, item, abi, Path(temp))
                required = HOST_JNI if name == "libreverie_native_host.so" \
                    else {ENTRY_SYMBOL}
                missing = required - symbols
                if missing:
                    fail(f"{item}: no dynamic exported symbol: {sorted(missing)}")
                for dependency in dependencies:
                    if (dependency.startswith("libreverie_")
                            or dependency == "libc++_shared.so"):
                        if f"lib/{abi}/{dependency}" not in entries:
                            fail(f"{item}: private runtime dependency absent: {dependency}")
                inspected += 1
    return inspected


def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: verify-native-module-exports.py app-phoneTest.apk", file=sys.stderr)
        return 2
    try:
        count = validate(Path(sys.argv[1]))
    except (ValueError, OSError, subprocess.SubprocessError,
            zipfile.BadZipFile) as error:
        print(f"NATIVE MODULE ELF CONTRACT: FAIL: {error}", file=sys.stderr)
        return 1
    print(f"NATIVE MODULE ELF CONTRACT: PASS: {count} host/module ELFs "
          f"({len(ABIS)} ABIs; {count // len(ABIS) - 1} registered modules)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
