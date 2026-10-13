#!/usr/bin/env python3
"""Check all six ReverieVR classic games are really wired for native packaging.

Source audit is dependency-free and never builds or triggers GitHub Actions.
Pass --apk to also inspect the exact built APK (both supported ABIs).
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from zipfile import BadZipFile, ZipFile

ROOT = Path(__file__).resolve().parents[1]
GAMES = (
    (0, "classic-pong", "reverie_module_classic_pong"),
    (1, "classic-snake", "reverie_module_classic_snake"),
    (2, "classic-chutes-ladders", "reverie_module_classic_chutes_ladders"),
    (3, "classic-checkers", "reverie_module_classic_checkers"),
    (4, "classic-go", "reverie_module_classic_go"),
    (5, "classic-chess", "reverie_module_classic_chess"),
)
ABIS = ("armeabi-v7a", "arm64-v8a")


def read(path: str, root: Path) -> str:
    return (root / path).read_text(encoding="utf-8")


def verify_source(root: Path) -> list[str]:
    errors: list[str] = []
    mk = read("app/src/main/jni/Android.mk", root)
    host = read("app/src/main/jni/native/reverie_native_host.cpp", root)
    source = read("app/src/main/jni/native/classic_games_module.cpp", root)
    shell = read("app/src/main/java/io/github/mrcalzon02/reverievr/VrShellRenderer.java", root)
    tests = read("app/src/test/native/classic_games_test.cpp", root)
    build = read("app/build.gradle", root)

    # Build each game as its own named native shared library, not as a menu stub.
    blocks = mk.split("include $(CLEAR_VARS)")
    selectors = re.findall(
        r'(?m)^#(?:if|elif)\s+CLASSIC_GAME_KIND\s*==\s*(\d+)\s*\n'
        r'constexpr\s+char\s+kId\[\]\s*=\s*"([^"]+)";',
        source,
    )
    expected = [(str(i), game_id) for i, game_id, _ in GAMES]
    if selectors != expected:
        errors.append("classic module selector-to-ID mapping differs from six known games")

    for kind, game_id, library in GAMES:
        matching = [block for block in blocks if re.search(
            rf'(?m)^LOCAL_MODULE\s*:=\s*{re.escape(library)}\s*$', block
        )]
        if len(matching) != 1:
            errors.append(f"{game_id}: expected exactly one native library declaration")
        else:
            block = matching[0].split("include $(BUILD_SHARED_LIBRARY)")[0]
            for item in ("native/classic_games_module.cpp", f"-DCLASSIC_GAME_KIND={kind}"):
                if item not in block:
                    errors.append(f"{game_id}: native build block missing {item}")
            if "include $(BUILD_SHARED_LIBRARY)" not in matching[0]:
                errors.append(f"{game_id}: native target lacks BUILD_SHARED_LIBRARY")
        registered = re.search(
            rf'\{{\s*"{re.escape(game_id)}"\s*,\s*"[^"]+"\s*,\s*'
            rf'"lib{re.escape(library)}\.so"\s*\}}',
            host,
        )
        if not registered:
            errors.append(f"{game_id}: native host registry does not match build library")

    for required in (
        'apps.addAll(classics);',
        'games.addAll(classics);',
        'mode = MODE_NATIVE_GAMES;',
        'host.onNativeModulePlaybackRequested(',
    ):
        if required not in shell:
            errors.append(f"launcher missing route: {required}")
    if '#include "classic_games.h"' not in source or '#include "classic_board_games.h"' not in source:
        errors.append("native renderer does not include both game rule engines")
    for required in ('Game::Pong', 'Game::Snake', 'Game::ChutesLadders',
                     'Kind::Checkers', 'Kind::Go', 'Kind::Chess'):
        if required not in tests:
            errors.append(f"native test coverage missing {required}")
    for abi in ABIS:
        if abi not in build:
            errors.append(f"Android Gradle configuration does not include ABI {abi}")
    return errors


def verify_apk(path: Path) -> list[str]:
    if not path.is_file():
        return [f"APK does not exist: {path}"]
    try:
        with ZipFile(path) as archive:
            present = set(archive.namelist())
    except (OSError, BadZipFile) as exc:
        return [f"APK cannot be inspected: {exc}"]
    errors = []
    for abi in ABIS:
        for _, game_id, library in GAMES:
            member = f"lib/{abi}/lib{library}.so"
            if member not in present:
                errors.append(f"{game_id}: APK missing {member}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--apk", type=Path, help="also audit an existing built APK")
    args = parser.parse_args()
    try:
        errors = verify_source(ROOT)
    except OSError as exc:
        print(f"CLASSIC PACKAGING: FAIL: required source unreadable: {exc}", file=sys.stderr)
        return 1
    if args.apk is not None:
        errors.extend(verify_apk(args.apk))
    if errors:
        for error in errors:
            print(f"CLASSIC PACKAGING: FAIL: {error}", file=sys.stderr)
        return 1
    kind = "source + APK" if args.apk is not None else "source registry"
    print(f"CLASSIC PACKAGING: PASS ({kind}; 6 games, 2 configured ABIs)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
