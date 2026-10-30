#!/usr/bin/env python3
"""Fail closed when native game UI regresses to illegible stroke lettering.

This is a static source/build-link gate, NOT proof of emulator or S9 legibility.
Actual appearance is checked through the real installed-game menu capture CI.
"""
import re
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
    text=(NATIVE/name).read_text(encoding="utf-8")
    assert marker in text, f"{name} lacks the installed Lato font path"
    assert not re.search(r"(?:::|\\b)StrokeText\\s*\\(",text), (
        f"{name} reverted to geometry-generated lettering")
    assert "for(int bit=0;bit<15;bit++)" not in text
classic=(NATIVE/"classic_games_module.cpp").read_text(encoding="utf-8")
assert "CLASSIC_GAME_KIND == 3 || CLASSIC_GAME_KIND == 5" not in classic, (
    "Font still gated to Chess and Checkers")
assert "reverie::breakwater::font::FittedScale" in classic
assert "CreateNativeFont(m)" in classic and "m->fontReady" in classic
assert "#include \"breakwater_font_atlas.h\"" in classic
assert "classicart::StrokeText" not in classic
mk=(ROOT/"app/src/main/jni/Android.mk").read_text(encoding="utf-8")
for module in CLASSES:
    match=re.search(
        r"LOCAL_MODULE := reverie_module_"+module+
        r"\\n(?:(?!include \\$\\(BUILD_SHARED_LIBRARY\\)).)*?LOCAL_LDLIBS \\+= ([^\\n]+)",
        mk,re.S)
    assert match, f"Missing Android build block for {module}"
    assert "-lz" in match.group(1).split(), (
        f"{module} omitted Android zlib needed for genuine font atlas")
print("PASS: all eight native Classics and core game UIs use actual Lato glyphs; "
      "all font modules link zlib")
