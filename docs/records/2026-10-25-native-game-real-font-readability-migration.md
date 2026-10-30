# ReverieVR native game readable-font migration

**User directive (2026-10-25):** Replace gameplay/menu labels fabricated from 3D shapes, connected strokes, and placeholder character meshes with **actual readable glyphs from a font**. Migrate one game at a time and inspect real installed Android captures before deciding the next conversion. Do not confuse intentionally decorative 3D objects/logos with ordinary UI labels. Preserve gameplay, saves, input, real game identity, and GL state.

**Source authority:** `mrcalzon02/ReverieVR`, `main`, `app/src/main/jni/native/`.
**Image authority:** real Android 10 run [40286810374](https://github.com/mrcalzon02/ReverieVR/actions/runs/40286810374), [complete owner archive](https://drive.google.com/file/d/1fwauYd8EtmHcGPAp7Jn3QV7rsssqDfYa/view) and [overview](https://drive.google.com/file/d/11QpNGV8lcK45ek3eEn8skgPER31YmPy4/view). This is a square FOV 80° GLES2 offscreen view, **not** actual S9 lens imagery.

## Actual implementation inventory

| Native modules/area | Present rendering in source | Font migration state |
| --- | --- | --- |
| Breakwater Battery | Licensed antialiased proportional Lato Semibold (SIL OFL 1.1), offline GL alpha atlas in `breakwater_font_atlas.h` | **Existing actual font; preserve and audit menu fits on-device** |
| Classic Chess | Existing same Lato atlas, alpha quads + dedicated static/dynamic shaders/buffers; separate chess art/wood textures | **Existing actual font; preserve** |
| Classic Checkers | **First phase changed:** `classic_games_module.cpp` kind 3 reuses the already proven Lato atlas, glyph metrics, GL shader and static/dynamic font quads; kind 5 Chess still uses same logic | **Source committed; Android screenshot acceptance pending** |
| Classic Go, Reversi | `classic_games_module.cpp` `classicart::StrokeText` connected flat-mesh segments | **Pending; one title per actual-capture review** |
| Classic Pong, Snake, Breakout, Chutes & Ladders | `classic_games_module.cpp` connected `StrokeText` geometry | **Pending; one title per actual-capture review** |
| Omega Front | `omega_front_module.cpp` `nativeart::StrokeText` geometry for strategy/tactical HUD | **Pending; especially small strategic labels** |
| Gumball Armada | `gumball_armada_module.cpp` `nativeart::StrokeText` geometry | **Pending; shipyard/loadout/menu input and truncation need focused screenshots** |
| Ministry Lantern Desk, Forward Detachment | `recon_projects_module.cpp` `nativeart::StrokeText` geometry | **Pending; module-specific small text and caption panes** |
| Red Ledger | `red_ledger_module.cpp` bakes geometric room signage/information; material textures coexist | **Needs classification of UI vs decorative signage before replacing** |
| Procedural Test Chamber | Current module does not call shared text helpers in main module source | **No generic text migration without demonstrated lettering usage** |
| Separate DOS archival classics | DOS executable engine/framebuffers, not these native font helpers | **Do not replace original game fonts with new UI overlays; capture DOS screens separately** |
| ReverieVR Home/shell | Java Android Canvas and in-headset world UI | **Already uses actual Android font rasterization; separate size/projection/contrast audit** |

## First engineering slice: Checkers

**Implementation:** Checkers conditionally shares exactly the Lato shader, texture decode, atlas upload, static/dynamic quad vertices and draw path previously used only by Chess. Chess-only sprites, book/panel transforms and gameplay remain inside kind 5. Checkers uses its existing title, rules, AI-difficulty control hitbox, claim-draw control, statuses, selection geometry and gameplay state; only `Text()` routes to actual atlas glyphs instead of 3D stroke segments. Context loss explicitly destroys glyph shader/texture/VBO and recreates through the original lifecycle. No new font file or third-party runtime dependency.

**Font source/license:** the project already embeds a *derived, compressed* Lato Semibold alpha atlas (`breakwater_font_atlas.h`) and includes `LICENSES/OFL-1.1.txt`. No redistribution of original font binaries or new licensing assumptions.

**Capturing the change:** `.github/workflows/reverie-android-recovery-simulation.yml` now watches the real game implementation and font atlas, builds and installs two x86_64 debug APKs, and saves `native-menu-classic-checkers.png` alongside all 14 other original in-game GLES2 frames and their SHA-256 / CRC-32 manifest. Compare Checkers' previous frame from run 40286810374 with the new screenshot **visually**, including title, right AI button, bottom instructions, status and any crowned K discs. Do not declare readability improved from compilation alone.

## Acceptance checklist for EACH subsequent module

1. **Before:** record old real emulator PNG with native module ID, Git SHA and camera; reproduce unreadable labels and identify affected states, menus, controls, long strings, partial text truncation and projection clipping. Do not diagnose layout solely from first-frame identity-view screenshot.
2. **Implement:** reuse licensed proportional font glyphs (atlas or Android-managed UI texture); keep alpha blending, depth ordering, state capture/restore, two-eye no-update rule, context-loss rebuild, game-specific UI geometry and hitboxes. No lazy rasterization per frame, no giant new dependencies.
3. **Text quality:** inspect uppercase/lowercase and digits required by the game, unknown glyph fallback, punctuation, original line breaks, metrics/advance, limited-area fitting, on-screen contrast and projection FOV. Avoid cropping or shrinking text so small it becomes illegible.
4. **Functionality:** game launches, controls function, board/view remains, saved games/load preserved, UI focus and failure feedback intact, GLES2 returns no error; validate existing rule/AI tests and game-specific visuals.
5. **After:** capture same game via actual Android JNI/GLES2 and compare before/after in pixel-aligned image; record evidence and either accept or repair. Do not modify other games simultaneously.
6. **Final:** S9 Daydream stereo and head/controller readability test independent of emulator.

**Global cross-project principle:** apply this audit progressively to all user game repos (not by mass-changing unrelated repositories). 3D letters can remain as scenic signage or intentionally artistic titles, but normal functional menus/HUD/dialog/tutorials should use real licensed font glyphs with legible scaling and coherent localization. Each repository keeps its own real runtime evidence and art/font license inventory; no synthetic screenshot may serve as proof.

**CI status:** `e7f881c9cf24998d820a9be18fc625d81049b15f` implemented the Checkers font conversion, `f076883732c96212e1f599117b4e4391fe8f53a2` added the source-triggered Android capture gate, `adedea43f5a4c70a40877df1c2e2475f0b35a68f` synchronized the native font standard. [Android run 40289492016](https://github.com/mrcalzon02/ReverieVR/actions/runs/40289492016) is the initial verification run; **do not upgrade status until the completed outcome and actual Checkers frame are inspected.**

## Second migration pass — 2026-10-29

The six pending native Classics (Go, Reversi, Pong, Snake, Breakout and Chutes & Ladders) now invoke the same existing real Lato Semibold atlas as Checkers and Chess. The Android build links zlib for all eight modules; source regression scripts reject `StrokeText` and confirm every Classic game has the font implementation and required linker dependency. Dynamic glyph buffers are cleared for every non-board state rebuild, and fixed glyphs receive an immutable static buffer. The `reverie-all-integrated-games-ci.yml` diagnostic workflow runs this contract. Refreshed native GLES2 emulator menu screenshots are generated by `native-game-menu-archive.yml` when the font module changes, replacing captured art rather than manually editing the previous archive. Source/CI passes are not automatically hardware readability acceptance. Remaining Red Ledger physical status-board pixel glyphs are a separate, acknowledged migration scope, not falsely counted as converted.
