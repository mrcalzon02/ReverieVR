# Native stroke-text adoption — bounded source slice

**Source-only; updated 2026-10-17. Signed APK and device acceptance pending.**

The common `reverie_native_vector_text.h` renderer exposes connected-stroke `StrokeText` glyph paths from `classic_vector_art.h` through existing GLES2 XYZRGB flat-mesh triangles. This is not a font-file loader or a texture/shader dependency.

The connected-stroke layer remains used by **Omega Front**, **both reconnaissance previews**, and the migrated native classic games. **Breakwater Battery no longer uses the 5×7 stroke-text renderer**: its game-private `breakwater_font_atlas.h` decodes a proportional, antialiased Lato Semibold font (SIL OFL 1.1) once per GLES2 context, draws alpha glyph quads in a dedicated 8-float-per-vertex batch, and fits actual glyph advances to shop cards. The atlas is a derived offline source blob with a bundled license, not a system-font runtime dependency. Red Ledger retains its independent static signage.

**Unfinished:** Gumball Armada still uses its existing block-glyph renderer on the current authoritative main branch. Its active concurrent gameplay source was deliberately not overwritten. Source-level regression `scripts/verify-native-vector-font.py` now checks only actually migrated files. Android NDK builds, actual S9 letter legibility, stereo, thermal/frame-time and installed APK are unverified. No GitHub Actions.
