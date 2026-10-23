# Gumball art and runtime asset gates

The source directory contains **retained, approved concept art**. It is intentionally
outside `app/src/main/assets` so full-size reference boards do not bloat the
Android APK or masquerade as usable gameplay textures.

* `candy_cosmos_asset_atlas.png` — approved text-free 8 × 5 planet/station/hazard
  board. `node_atlas.json` defines all 40 cells in reading order.
* `pip_parfait_cherry_whiz.png`, `lolly_wren_soft_landing.png` — character sheets
  for existing named sortie allies; **not** individual portraits or sprites.
* Remaining character/source sheets stay in the user-curated Drive folder
  until a real in-game binding is assigned, avoiding reimport of rejected art.

Run `python3 scripts/gumball_prepare_art.py` (Pillow installed) to generate
40 deterministic 160-pixel maximum node tiles in
`app/src/main/assets/gumball/nodes`. The script refuses missing, wrong-size,
or mislabeled source; `--verify` compares exact RGBA pixels for all tiles.

**October 23 production Chart slice:** `app/build.gradle` now registers
`stageGumballRouteArt` as an Android variant-owned generated asset source.
It crops the approved 1672x941 Candy Cosmos sheet directly with Java ImageIO
(no runtime PNG decoding or Pillow dependency), resizes exactly eight
game-bound cells to 96x86, and writes 16,520-byte `GAI1` RGB565 assets under
`native/gumball/nodes/`. `gumball_armada_module.cpp` requests those exact
files through the native host's scoped asset reader at GL-context creation,
uploads GLES2 textures, and draws them on real Chart lane cards selected by
deterministic lane ID. Assets are destroyed/reloaded across context changes.
Source art remains outside APK assets. Missing runtime files are logged, not
misrepresented as loaded.

Two additional 96x86 derivatives are cropped from the canonical
`art/gumball/source/pip_parfait_cherry_whiz.png` and
`art/gumball/source/lolly_wren_soft_landing.png` studies (original
1536x1024, source rectangle x=1150,y=5,w=128,h=118). These are portraits,
not replacement ship sprites. When the existing Pip/Lolly escort is active,
the actual GLES2 flight HUD selects the corresponding character portrait;
both existing ally flight actors remain unchanged. Approved Drive originals
are retained and never bundled directly. The ten total derivatives require
165,200 bytes of raw RGB565+header data.

**Current evidence:** Source-sheet crop contents were visually inspected and
ten format-correct derivative assets generated locally from live Drive originals.
Those local outputs are **not** committed binaries and are not proof that
Gradle has executed its own converter or that the Android device has loaded them.

**Current evidence:** source changes and GitHub readback verified; exact asset
allowlist header passes strict host GCC, Clang and sanitizer checks. The build
task has **not** been executed against the real atlas with Gradle, and there is
no Android APK/GLES2/S9 screenshot, hence actual packaging, runtime loads and
on-headset display are still pending. This is route-card artwork, not the
4096-node navigable galaxy, character sprite integration, or 3D Shipyard.

**Acceptance:** These generated PNGs are not proof of gameplay integration.
The current Gumball native GLES2 module uses procedural flat meshes and has
no asset-to-texture renderer. A runtime loader, GLES2 texture lifecycle, and
node/character binding must be implemented and verified on Android before
claiming that any new PNG appears in game. In particular, 4096 actual node
types and the 3D player ship are not implemented by this extraction script.
Do not call a concept-sheet grid a sprite sheet or display fictitious shops.
