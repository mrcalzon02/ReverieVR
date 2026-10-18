#!/usr/bin/env python3
"""Guard the native menu font migration without requiring an Android SDK.

This confirms actual geometry path wiring, not an APK/frame/readability test.
Run: python3 scripts/verify-native-vector-font.py
"""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
MODULES={
    "classic_games_module.cpp": "classicart::StrokeText",
    "breakwater_module.cpp": "font::AppendText",
    "omega_front_module.cpp": "nativeart::StrokeText",
    "recon_projects_module.cpp": "nativeart::StrokeText",
}
for name, expected in MODULES.items():
    source=(ROOT/"app/src/main/jni/native"/name).read_text(encoding="utf-8")
    assert expected in source, f"{name} does not use the native vector font"
    assert "for(int bit=0;bit<15;bit++)" not in source
    assert "for(int i=0;i<15;" not in source
    assert "for(int p=0;p<15;" not in source
    assert "const char *Glyph(" not in source
    assert "const char* Glyph(" not in source
art=(ROOT/"app/src/main/jni/native/classic_vector_art.h").read_text(
    encoding="utf-8")
assert "flatmesh::Segment" in art
assert "void ChessPiece(" in art
assert "case 39:" in art  # Correctly escaped apostrophe char
print("PASS Breakwater Lato atlas, three other stroke renderers and chess symbols")
