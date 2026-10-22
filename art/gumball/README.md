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

**Acceptance:** These generated PNGs are not proof of gameplay integration.
The current Gumball native GLES2 module uses procedural flat meshes and has
no asset-to-texture renderer. A runtime loader, GLES2 texture lifecycle, and
node/character binding must be implemented and verified on Android before
claiming that any new PNG appears in game. In particular, 4096 actual node
types and the 3D player ship are not implemented by this extraction script.
Do not call a concept-sheet grid a sprite sheet or display fictitious shops.
