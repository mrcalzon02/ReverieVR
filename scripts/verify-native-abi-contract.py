#!/usr/bin/env python3
"""Compile and run the host-buildable ReverieVR native ABI contract test."""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TEST = ROOT / "app/src/test/native/native_abi_contract_test.cpp"
INCLUDE = ROOT / "app/src/main/jni/native"


def find_compiler() -> str:
    configured = os.environ.get("CXX", "").strip()
    if configured:
        path = shutil.which(configured)
        if path:
            return path

    for candidate in ("c++", "g++", "clang++"):
        path = shutil.which(candidate)
        if path:
            return path

    raise RuntimeError(
        "No C++ compiler found. Set CXX or install c++/g++/clang++."
    )


def main() -> int:
    compiler = find_compiler()

    with tempfile.TemporaryDirectory(prefix="reverie-native-abi-") as temp:
        output = Path(temp) / (
            "native_abi_contract_test.exe"
            if os.name == "nt"
            else "native_abi_contract_test"
        )
        command = [
            compiler,
            "-std=c++17",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-I",
            str(INCLUDE),
            str(TEST),
            "-o",
            str(output),
        ]
        subprocess.run(
            command,
            cwd=ROOT,
            check=True,
        )
        completed = subprocess.run(
            [str(output)],
            cwd=ROOT,
            check=False,
        )
        return completed.returncode


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError) as exc:
        print(
            f"NATIVE ABI CONTRACT: FAIL: {exc}",
            file=sys.stderr,
        )
        sys.exit(1)
