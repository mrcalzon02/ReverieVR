# Omega Front — Individual terrain PNGs

Each file is ONE top-down terrain tile, not an atlas. Ten original 1254×1254 generated PNGs are preserved byte-for-byte under `source/`; matching quantized 64×64 one-cell images are under `game_64/`. See `manifest.json` for individual Git blob hashes and dimensions. The original art direction is the Omega Front wasteland / frontier concept art. Visible grid seams are intentional and acceptable. No facility sprites have been stretched from this set.

These are repository art assets, not evidence of runtime integration: the Omega Front native tactical renderer still uses colored flatmesh rectangles. Texture sampling and asset loading are separate future game work. Do not report the APK as rendering these images until that code is implemented and checked on device.
