# Livius chess SVG source art — CC0

Author: Martin Sedlák (2026). Snapshot: https://github.com/kmar/chess_svg_piece_sets at commit `af40ea51e87eddee1ae7ee35ae312893a1271233`.

Files `wk.svg` through `bp.svg` are the twelve unmodified 64×64 original chess vectors. Original CC0 public-domain dedication is stored alongside them as `LICENSE`. Every SVG byte was imported with the exact matching upstream Git blob SHA.

**Runtime truth:** The current native Chess module renders separate original flat-mesh glyph silhouettes, *not* these imported external SVG strokes/fills. This folder is an authenticated source family for a future deterministic offline raster atlas/GLES2 sprite implementation, not an APK asset list and not a claim that in-headset SVG textures are functional. Any future converter must preserve the 12 identities, alpha/contrast, source integrity, no Android-time network requirements and dual-eye/performance validation.
