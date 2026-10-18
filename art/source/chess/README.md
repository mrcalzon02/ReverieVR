# Livius chess SVG source art — CC0

Author: Martin Sedlák (2026). Snapshot: https://github.com/kmar/chess_svg_piece_sets at commit `af40ea51e87eddee1ae7ee35ae312893a1271233`.

Files `wk.svg` through `bp.svg` are the twelve unmodified 64×64 original chess vectors. Original CC0 public-domain dedication is stored alongside them as `LICENSE`. Every SVG byte was imported with the exact matching upstream Git blob SHA.

**Runtime integration (2026-10-17):** `app/src/main/jni/native/classic_chess_sprite_atlas.h` now contains grayscale/alpha RLE image tiles rasterized from all twelve source SVG pieces. The native Chess module decodes them once per GLES2 context, uploads a 384×128 luminance-alpha atlas, and draws correctly mapped piece quads for both eyes. The original SVGs remain unchanged here as provenance. `classic_games_test.cpp` verifies the atlas decodes, contains all twelve nonempty identities, and the knights retain filled contours. The source conversion and GPU code are committed, but Android compilation, signed APK publication, and physical Galaxy S9/Daydream binocular/frame-time acceptance are separate unverified gates. No device-side SVG parser or network fetch is needed.
