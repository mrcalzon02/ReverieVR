#!/usr/bin/env python3
"""Verify ReverieVR native implementation/documentation synchronization.

This guard is intentionally dependency-free. It validates factual anchors in the
current tree and can also require mapped implementation + documentation files to
change together.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

API_HEADER = Path("app/src/main/jni/native/reverie_native_module.h")
HOST_CPP = Path("app/src/main/jni/native/reverie_native_host.cpp")
RUNTIME_JAVA = Path(
    "app/src/main/java/io/github/mrcalzon02/reverievr/NativeModuleRuntime.java"
)
MATERIAL_H = Path("app/src/main/jni/native/procedural_material_atlas.h")
MATERIAL_CPP = Path("app/src/main/jni/native/procedural_material_atlas.cpp")
STATIC_H = Path("app/src/main/jni/native/red_ledger_static_geometry.h")
STATIC_CPP = Path("app/src/main/jni/native/red_ledger_static_geometry.cpp")

API_DOC = Path("docs/native/API_V1.md")
PROCEDURAL_DOC = Path("docs/native/PROCEDURAL_CONTENT_STANDARD.md")
CONTRACT_INDEX = Path("docs/native/CONTRACT_INDEX.md")

SYNC_MAP = {
    API_HEADER: {API_DOC},
    HOST_CPP: {API_DOC},
    RUNTIME_JAVA: {API_DOC},
    MATERIAL_H: {PROCEDURAL_DOC},
    MATERIAL_CPP: {PROCEDURAL_DOC},
    STATIC_H: {PROCEDURAL_DOC},
    STATIC_CPP: {PROCEDURAL_DOC},
}


class ContractError(RuntimeError):
    pass


def read(path: Path) -> str:
    full = ROOT / path
    if not full.is_file():
        raise ContractError(f"required file is missing: {path}")
    return full.read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ContractError(message)


def regex_value(pattern: str, text: str, label: str) -> str:
    match = re.search(pattern, text, flags=re.MULTILINE)
    if not match:
        raise ContractError(f"could not derive {label} from authoritative source")
    return match.group(1)


def check_current_content() -> None:
    header = read(API_HEADER)
    host = read(HOST_CPP)
    runtime = read(RUNTIME_JAVA)
    material_h = read(MATERIAL_H)
    static_h = read(STATIC_H)
    api_doc = read(API_DOC)
    procedural_doc = read(PROCEDURAL_DOC)
    contract_index = read(CONTRACT_INDEX)

    abi_version = regex_value(
        r"^#define\s+REVERIE_NATIVE_MODULE_ABI_VERSION\s+(\d+)u\s*$",
        header,
        "native ABI version",
    )
    entry_symbol = regex_value(
        r'^#define\s+REVERIE_NATIVE_MODULE_ENTRY_SYMBOL\s+"([^"]+)"\s*$',
        header,
        "native entry symbol",
    )
    save_max = regex_value(
        r"^#define\s+REVERIE_NATIVE_SAVE_MAX_BYTES\s+(\d+)u\s*$",
        header,
        "maximum save bytes",
    )

    require(
        f"REVERIE_NATIVE_MODULE_ABI_VERSION == {abi_version}" in api_doc,
        "API_V1.md does not report the current ABI version",
    )
    require(
        f"`{entry_symbol}`" in api_doc,
        "API_V1.md does not report the current module entry symbol",
    )
    require(
        f"`{save_max}` bytes" in api_doc,
        "API_V1.md does not report the current save-size limit",
    )

    for token in (
        "REVERIE_NATIVE_POINTER_NONE",
        "REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER",
        "REVERIE_NATIVE_POINTER_VIRTUAL_CONTROLLER",
        "read_save",
        "write_save",
        "on_gl_context_created",
        "release_gl_context",
        "render_eye",
    ):
        require(token in header, f"authoritative ABI source lost expected token: {token}")
        require(
            f"`{token}`" in api_doc or token in api_doc,
            f"API_V1.md does not mention current ABI token: {token}",
        )

    for token in ("HostReadSave", "HostWriteSave", "IsSafeSaveSlot"):
        require(token in host, f"native host lost expected service implementation: {token}")

    require(
        "value.length() > 80" in runtime,
        "NativeModuleRuntime module-id length policy changed; update verifier/docs",
    )
    require(
        "maximum length of 80 characters" in api_doc,
        "API_V1.md does not report the current module-id length policy",
    )

    tile_size = int(
        regex_value(
            r"constexpr\s+int\s+kTileSize\s*=\s*(\d+)\s*;",
            material_h,
            "procedural material tile size",
        )
    )
    static_cube_count = int(
        regex_value(
            r"constexpr\s+size_t\s+kStaticCubeCount\s*=\s*(\d+)u\s*;",
            static_h,
            "static cube count",
        )
    )
    cube_vertex_count = int(
        regex_value(
            r"constexpr\s+size_t\s+kCubeVertexCount\s*=\s*(\d+)u\s*;",
            static_h,
            "cube vertex count",
        )
    )
    stride = int(
        regex_value(
            r"constexpr\s+size_t\s+kStaticVertexStride\s*=\s*(\d+)u\s*;",
            static_h,
            "static vertex stride",
        )
    )

    atlas_side = tile_size * 2
    rgb565_bytes = atlas_side * atlas_side * 2
    static_bytes = static_cube_count * cube_vertex_count * stride * 4

    require(
        f"{atlas_side}x{atlas_side}" in procedural_doc,
        "procedural standard does not report the current atlas dimensions",
    )
    require(
        f"{rgb565_bytes // 1024} KiB" in procedural_doc,
        "procedural standard does not report the current RGB565 atlas footprint",
    )
    require(
        f"{static_cube_count} static" in procedural_doc,
        "procedural standard does not report the current static cube count",
    )
    require(
        f"{static_bytes:,}-byte" in procedural_doc,
        "procedural standard does not report the current static VBO size",
    )
    for token in (
        "GenerateMaterialAtlas",
        "GenerateMaterialAtlasRgb565",
        "BuildStaticRoomVertices",
    ):
        source = material_h if "MaterialAtlas" in token else static_h
        require(token in source, f"authoritative utility source lost expected function: {token}")
        require(
            f"`{token}`" in procedural_doc,
            f"procedural standard does not name current utility: {token}",
        )

    for mapped_source, mapped_docs in SYNC_MAP.items():
        require(str(mapped_source) in contract_index, f"contract index missing {mapped_source}")
        for mapped_doc in mapped_docs:
            require(str(mapped_doc) in contract_index, f"contract index missing {mapped_doc}")


def git_changed_files(args: argparse.Namespace) -> set[Path]:
    if args.staged:
        command = ["git", "diff", "--cached", "--name-only", "--diff-filter=ACMR"]
    elif args.base:
        command = ["git", "diff", "--name-only", "--diff-filter=ACMR", f"{args.base}...HEAD"]
    else:
        return set()

    try:
        output = subprocess.check_output(command, cwd=ROOT, text=True)
    except (OSError, subprocess.CalledProcessError) as exc:
        raise ContractError(f"could not inspect git changes: {exc}") from exc

    return {Path(line.strip()) for line in output.splitlines() if line.strip()}


def check_same_change(changed: set[Path]) -> None:
    if not changed:
        return

    failures: list[str] = []
    for source, docs in SYNC_MAP.items():
        if source not in changed:
            continue
        if not any(doc in changed for doc in docs):
            failures.append(
                f"{source} changed without mapped documentation update "
                f"({', '.join(str(doc) for doc in sorted(docs, key=str))})"
            )

    if failures:
        raise ContractError("; ".join(failures))


def main() -> int:
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "--base",
        help="git base revision for same-change validation, e.g. HEAD^",
    )
    group.add_argument(
        "--staged",
        action="store_true",
        help="validate currently staged changes as one scoped change",
    )
    args = parser.parse_args()

    try:
        check_current_content()
        check_same_change(git_changed_files(args))
    except ContractError as exc:
        print(f"NATIVE DOC CONTRACT: FAIL: {exc}", file=sys.stderr)
        return 1

    print("NATIVE DOC CONTRACT: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
