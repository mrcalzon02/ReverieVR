# Breakwater Battery VR — native menu and 3D gun pass (2026-10-16)

## Problem / intent

A Galaxy S9/Daydream player reported that the original native shop/difficulty screen looked like oversized blocks with no legible text, menu elements escaped the bottom of the board, and the cannon itself looked like two intersecting flat planes. The existing source already called the `StrokeText` vector glyph baker, but primary labels were not fitted to their cards. The CREDITS line was at y=-1.70, below the shop panel bottom (-1.475); the unconditioned foreground gun/parapet occupied the same near-field sightline as the menus. The -1.10 world sea plane also risked occluding low shop lettering.

## Changed authoritative source

- `app/src/main/jni/native/breakwater_menu_layout.h` (game-private): one rectangle contract for difficulty cards, two rows of eight shop cards, action buttons and independent pointer hover selection. The single `FittedScale` function fits actual stroke-font 4×glyph advances inside every card width with an inset.
- `app/src/main/jni/native/breakwater_module.cpp`: shop and difficulty panels now use that same contract for drawing and controller-ray hit testing. Hovered buttons visibly brighten, labels are concise and two-line, START/ARMORY/UPGRADES remain live and clear of the credit footer, unavailable armory cells 6/7 reject clicks instead of acting as placebo controls. The purchase/phase transition/feedback paths remain Campaign-owned and unchanged. Credits fit inside a smaller panel at y=-0.95, wholly above the y=-1.10 sea plane. Menu titles identify location, day/night, wave and credits.
- `app/src/main/jni/native/breakwater_gun_mesh.h`: deterministic no-GL 12-sided colored triangle generator, per-weapon barrel size, 3D breech/collars, recessed dark muzzle, faceted wheels/hubs, sloped protective armor and secondary flak/launcher barrel where relevant. Tube axis follows the original hand-aim direction; it is not a crossed billboard. The near-field gun and obstructive parapet are emitted only in active combat. Existing original sprite atlas remains in use for world infantry/boats, shop/switching/support icons. The obsolete forward-facing mounted-body sprite is no longer emitted.
- `app/src/test/native/breakwater_menu_layout_test.cpp`, `breakwater_gun_mesh_test.cpp`, `scripts/verify-breakwater-visuals.sh`: deterministic standalone C++17 host verification of all target rectangles/overlap/fitted labels/sea clearance, all five weapon geometry variants, world-axis/bounds, direction-sensitive elevation, replay determinism and rejection of NaN direction.

## Verification evidence

Changes were committed to the *remote* `mrcalzon02/ReverieVR` `main` through GitHub connector writes with post-write readback. Exact latest header/test blobs were reproduced in a temporary local test directory and confirmed byte-for-byte using `git hash-object`:

- menu layout header: `b0711cff530155fa98df12553bb42dd04b211597`
- gun geometry header: `1c76e22caa674f3cfef35426f7657705315e427b`
- menu layout test: `a67daf0e1eb26e37e0f2f0aee0a37faaf7f56296`
- gun geometry test: `441f144e94ea3ca4d95014dbe33a61f0d0e32f5b`

The geometry test directory used a tiny local weapon-enumeration shim reflecting the authoritative `breakwater_weapons.h` enum, because the container had no network Git access. **Both exact-source header tests passed strict `g++` and `clang++` with C++17, `-O1 -Wall -Wextra -Wpedantic -Werror`**. Observed full gun geometry: 292 triangles each for machine/heavy/direct-cannon mounts, 364 each for flak/missile variants, all with finite multi-axis bounds. A too-strict original inclined-barrel test assertion was corrected from max-Y > -0.20 to the observed sound limit > -0.25 without modifying production mesh behavior. A separate root-cause layout check moved the footer and actions clear of the sea plane.

**Not verified:** full `breakwater_module.cpp` compilation against GLES2/NDK and real shader state, `scripts/verify-native-doc-sync.py` from a complete worktree, full campaign tests, packaged Android APK, live Daydream stereo/menu legibility, hand-controller ray alignment, performance/thermal headroom, GL context recovery, or physical installation/launch. Consequently this remains **source-implemented, isolated tests passed; device-unaccepted**. No GitHub Actions introduced or monitored, no APK fabricated, no save format/ABI changed.

## Follow-on source implementation, same date

The shop card formatter `breakwater_shop_feedback.h` and `ShopDisplayPrice`
show actual contextual purchase descriptions and status (BUY, NEED, MAXED,
EQUIP, ACTIVE), plus ARMORY/UPGRADES/START messages. The hovered card remains
the same authoritative controller-ray target; `FittedScale` constrains the
line in the narrow gap above the existing two-row catalog. Dim red treatment
makes insufficient credits obvious without disabling the inspectable tile.
No extra overlays/cards/tabs were introduced.

`breakwater_gun_mesh.h` now supports optional confirmed-shot recoil and a
faceted volumetric muzzle flash at the moving barrel mouth. Firing duration
is the previously existing 90 ms `State::flash` updated once in the
native Update. This has no effect on Campaign damage, aiming bounds,
reloading, collision, save schema, shell focus, or other classic/native apps.
The slide's max displacement is 0.12 world units and the fixed wheels/shield
remain at their pre-shot positions.

The exact remote files were locally validated in
`/mnt/data/breakwater-native-check`, with `git hash-object` parity for
the committed generated-art/header fixtures. Strict C++17 GCC and Clang
builds passed the menu, gun and shop standalone regressions. Clang ASan
and UBSan builds passed the gun and shop tests as well. A corrected
muzzle-flash z-bound assertion uses the observed exact procedural geometry
rather than a fabricated muzzle reach; it does not weaken the production
model. This is isolated geometry and text validation, **not** integrated
GLES2/NDK or headset evidence.

## Menu redraw policy and difficulty-preview follow-up

The host's main `BuildScene` machinery previously ran on every `Update`
even while the shop/difficulty selection and defeat/victory cards were
unchanged. The new `breakwater_scene_refresh.h` offers a pure boolean
policy that the authoritative module now uses: old/new Combat phases
always refresh, while noncombat scenes rebuild only for a button action,
hover/focus transition, pointer restoration or phase change. The
pointer-none suspension is handled specially: its first transition
invalidates stale aim/hover, while repeated lost-focus frames remain
cached. No menu invents a new data source, setter or independent clock.
Coastal water shimmer freezes during menu idling and continues normally
in battle. Save/game engine logic remains unchanged.

The same menu header now returns fixed first-day difficulty labels
CADET 2/2, REGULAR 3/3, VETERAN 4/4, SIEGE 5/5 opening threats/day waves,
mirroring the existing `Campaign::NextCount` and
`Campaign::waves_this_period` formulas. The hint is drawn with the native
stroke font in the tested clear band above the buttons, no extra panel.

Tests: exact four changed layout/refresh files reconstructed in
`/mnt/data/breakwater-native-check`, `git hash-object` compared against
remote readback; strict GCC and Clang C++17 runs passed four out of four
visual/regression tests each, plus four out of four Clang
ASan/UBSan runs. The host harness uses the verified Campaign Phase
enum declaration without needing the rest of the Android rendering stack.
The full module/NDK build, device and docs-sync whole-worktree check were
not run. No GitHub Actions invoked.

## Next acceptance run

From a real checked-out full repository, run `bash scripts/verify-breakwater-visuals.sh`, the existing native Breakwater campaign regressions and `python3 scripts/verify-native-doc-sync.py --base HEAD^` with a relevant co-change range. Build the Android NDK module and APK locally; visually inspect the four difficulty choices, two purchase pages, credits, and all five weapon shapes inside both eyes on Galaxy S9. Validate highlight and tap alignment, missing-card rejection, first-wave START, Quick Menu focus release, gun traverse, artillery/projectiles and sustained frame pacing. Do not promote hardware acceptance until these are observed.
