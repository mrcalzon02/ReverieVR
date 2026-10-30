#!/usr/bin/env python3
"""Guard genuine licensed-font use in the native Classic and featured game UIs.

Source/build-link checks only: installed Android GLES2 menu captures and
Galaxy S9 binocular checks remain separate acceptance requirements.
"""
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
NATIVE=ROOT/"app/src/main/jni/native"
CLASSES=(
    "classic_pong","classic_snake","classic_chutes_ladders",
    "classic_checkers","classic_go","classic_chess",
    "classic_breakout","classic_reversi"
)
REAL_FONT_FILES={
    "classic_games_module.cpp":"reverie::breakwater::font::AppendText",
    "breakwater_module.cpp":"font::AppendText",
    "omega_front_module.cpp":"reverie::nativetext::LatoLayer",
    "gumball_armada_module.cpp":"reverie::nativetext::LatoLayer",
    "recon_projects_module.cpp":"reverie::nativetext::LatoLayer",
}
for name,marker in REAL_FONT_FILES.items():
    source=(NATIVE/name).read_text(encoding="utf-8")
    assert marker in source, f"{name} lacks the existing Lato atlas path"
    assert "StrokeText(" not in source, (
        f"{name} reverted to hand-built segment lettering")
    assert "for(int bit=0;bit<15;bit++)" not in source
classic=(NATIVE/"classic_games_module.cpp").read_text(encoding="utf-8")
assert "CLASSIC_GAME_KIND == 3 || CLASSIC_GAME_KIND == 5" not in classic, (
    "Font still restricted to Chess and Checkers")
assert "reverie::breakwater::font::FittedScale" in classic
assert "CreateNativeFont(m)" in classic and "m->fontReady" in classic
assert '#include "breakwater_font_atlas.h"' in classic
mk=(ROOT/"app/src/main/jni/Android.mk").read_text(encoding="utf-8")
for module in CLASSES:
    name="LOCAL_MODULE := reverie_module_"+module
    assert name in mk, f"Native module {module} is missing from Android build"
    begin=mk.index(name)
    end=mk.index("include $(BUILD_SHARED_LIBRARY)",begin)
    block=mk[begin:end]
    libs=[line for line in block.splitlines()
          if line.startswith("LOCAL_LDLIBS +=")]
    assert len(libs)==1 and "-lz" in libs[0].split(), (
        f"{module} missing system zlib for licensed Lato font atlas")
print("PASS: eight native Classics and all Lato-migrated featured UI modules "
      "use real glyphs with required zlib linkage")
