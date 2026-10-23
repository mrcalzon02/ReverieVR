# Gumball Armada — approved Candy Cosmos + escort portraits (2026-10-23)

**Authority:** ReverieVR `main`, native Gumball Armada only. The shared host's existing
game-scoped asset allowlist is amended solely for Gumball; no unrelated modules,
global navigation, Android ABI, saved data, or public shader SDK changes.
**No GitHub Actions run.**

## Intent and approved sources

The canonical Drive folder was enumerated live and retained 14 PNGs. Existing
full-size repo source masters were independently compared with the Drive files,
with identical byte-for-byte Git blob hashes:

| Source | Repo/Drive Git blob SHA |
| --- | --- |
| `candy_cosmos_asset_atlas.png` | `4a031f9d0c2f633e85f1515c6c3b8fac69c6810f` |
| `pip_parfait_cherry_whiz.png` | `354a4e41111a9f59e0a28433cd204c6bab05634f` |
| `lolly_wren_soft_landing.png` | `6ece767c69ce8edc82647cb25722d45064d4116f` |

The user rejected earlier generic/human Gumbo concept; **no Gumbo image** is
imported. Only three already-existing vetted source masters participate in
this slice; rejected/deleted Drive designs were neither retrieved nor restored.

## Execute

`app/build.gradle` now uses the existing Android Components generated-assets
API to register a `StageGumballRouteArtTask` for every APK variant. The task
reads the approved 1672×941 eight-column/five-row Candy Cosmos atlas and
generates eight deterministic 96×86 RGB565 tiles: gummy_world,
chocolate_world, ice_candy_world, citrus_world, gas_giant, ring_world,
outpost_station, and crystal_field. Two additional canonical 1536×1024
Pip Parfait/Lolly Wren studies supply reviewed character portrait rectangles
`(x=1150,y=5,w=128,h=118)`. The 10 generated game-only assets are stored
under `native/gumball/nodes` and `native/gumball/allies`.

Each asset is **16,520 bytes** = `GAI1`, LE dimensions 96×86, plus
little-endian RGB565 pixels. Ten assets total **165,200 bytes** of generated
asset payload. No full-size PNG reference sheet is placed in APK assets.

`reverie_native_asset_scope.h` admits only those ten exact asset names
when `id == gumball-armada`. `gumball_armada_module.cpp` uses the existing
host `read_asset` optional service to validate asset size/GAI1 metadata
on GL-context creation, upload them as GLES2 RGB565 textures, render the
matching illustration for each actual Chart lane ID, and display the Pip or
Lolly portrait only while that companion is actually active in a sortie.
Original procedural physical escort/ship visuals and gameplay remain.
Texture VBO/programs/handles are released and re-created with the GL context;
the new portrait is an encounter UI element, not a 3D ship replacement.

## Observe / verify

- `reverie_native_asset_scope.h` exact GitHub blob SHA
  `d1cb3ec2d5c8b304a66484fec16f86c10769d588` matched the local header byte-for-byte.
- `native_asset_scope_test.cpp` exact GitHub blob SHA
  `504abe994a7a3165a9d8812e991f749e3649b599` matched the local test
  byte-for-byte. The **exact committed** regression passed host strict GCC C++17,
  strict Clang C++17, and Clang AddressSanitizer+UndefinedBehaviorSanitizer.
- Headless Java ImageIO validated each of the ten atlas/sheet rectangles
  using the original approved Drive bytes and generated ten GAI1 files with
  the exact 16,520-byte schema. Python visual-preview extraction was also
  performed. This is a **standalone source/transformation proof**, not
  execution of `StageGumballRouteArtTask` inside an Android Gradle build.
- Android Gradle execution, APK asset inspection, native GLES2 module
  compile/link for ARM32/ARM64, actual runtime texture read/upload/draw,
  stereo screenshot, Galaxy S9/Daydream gameplay and thermal checks
  are **not performed**. Do not mark gameplay/device integration accepted
  until those gates have direct evidence.

## Next

Run the normal Android Gradle asset staging and authoritative Gumball NDK
build, verify 10 generated files in the actual APK, inspect packaged
`reverie_module_gumball_armada`, and test both chart and Pip/Lolly encounter
portraits on the physical S9. Next art pass should extract named Sour rival
and vendor imagery and bind each to a real gameplay/menu consumer before
packaging. The 7×7 connected editor still requires separately staged X/Y/Z
craft construction, 3D preview, migration, and gameplay model matching.

**Commit/push:** Direct GitHub contents updates on existing `main` with
remote blob readback. No branch, PR, release, or GitHub Action.
