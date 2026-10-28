#!/usr/bin/env python3
"""Unified-release guard: registered games must be physically inside the one host APK."""
import re
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "app/src/main/jni/native/reverie_native_host.cpp"
ACTIVITY = ROOT / "app/src/main/java/io/github/mrcalzon02/reverievr/GameModulesActivity.java"
def verify(apk):
    host = SOURCE.read_text(encoding="utf-8")
    names = re.findall(r'"(libreverie_module_[a-z0-9_]+\\.so)"', host)
    if len(names) < 15 or len(set(names)) != len(names):
        raise ValueError("Incomplete or duplicate compiled game registry")
    menu = ACTIVITY.read_text(encoding="utf-8")
    forbidden = ("GameModuleUpdateService", "CompanionGameApkInstaller",
                 "RvPackInstaller", "ACTION_OPEN_DOCUMENT", "installRemote(")
    if any(name in menu for name in forbidden):
        raise ValueError("Game library still exposes independent game acquisition")
    with zipfile.ZipFile(apk) as archive:
        bundled = set(archive.namelist())
        expected = {"lib/%s/%s" % (abi, name)
                    for abi in ("armeabi-v7a", "arm64-v8a")
                    for name in names}
        missing = sorted(expected - bundled)
        if missing:
            raise ValueError("Main APK missing bundled games: " + ", ".join(missing))
    print("PASS: unified main APK packages %d native games in both ARM ABIs; no per-game downloads in library UI" % len(names))

if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("Usage: verify-unified-game-apk.py <signed-app.apk>")
    try:
        verify(Path(sys.argv[1]))
    except (ValueError, OSError, zipfile.BadZipFile) as e:
        raise SystemExit("FAIL: " + str(e))
