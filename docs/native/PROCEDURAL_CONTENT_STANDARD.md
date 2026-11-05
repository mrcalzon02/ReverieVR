# ReverieVR Procedural Content and Compact Asset Standard

**Status:** provisional native-game standard built from implemented Test Chamber and Red Ledger techniques. Hardware energy/thermal benefit remains evidence-driven.

## Purpose

This file is governed by `docs/native/CONTRACT_INDEX.md`. Changes to the mapped procedural-material or static-geometry utilities require same-change review/update of this standard and should pass `python3 scripts/verify-native-doc-sync.py`.

ReverieVR deliberately treats recipes, seeds, compact tables, and generation code as first-class game assets.

The inspiration is the same broad discipline that made tiny procedural productions possible: store the *instructions for making the asset* when those instructions are smaller and cheaper than storing the expanded asset.

The goal is not to reproduce any specific historical engine. The goal is to build a ReverieVR-native content pipeline suited to small VR games on constrained Android hardware.

## Governing art direction — PS1 geometry and .kkrieger-style recipe economy

ReverieVR's visual target is primitive-era **PlayStation 3D**, not high-resolution
modern realism. This applies across shell homerooms, equipment, characters,
controller twins, and native games: purposeful silhouettes, low-polygon models,
limited flat/faceted color, tiny nearest-filtered material patterns only where
they make a visual difference, and restrained CPU/GPU state changes. Deliberate
chunkiness must not corrupt stereo projections or click-target alignment.

.kkrieger is a **conceptual example** of saving compact generation rules rather
than expanded asset files, not a library dependency, copied algorithm, or a
claim that boot-time synthesis automatically makes an APK faster. The current
Java shell home uses generated color-only triangle meshes and RGB555 palette
recipes without any new texture atlas; that is already the cheaper path for
its scene detail. The native Red Ledger RGB565 atlas demonstrates the different
case where a tiny procedural texture genuinely helps. Always measure startup
CPU cost, GPU upload size, draw calls, frame pacing and battery/thermal
separately. Do not synthesize textures in the per-eye draw loop.

## Governing rule

A generated representation is preferred when it is:

- deterministic;
- compact;
- fast enough to build outside the hot frame loop;
- visually sufficient for the intended art direction;
- safe across GL context recreation;
- measurably no worse than an authored alternative for runtime battery/thermal behavior.

APK size alone does not prove efficiency.

## Determinism

Every procedural asset with visible or persistent significance should be reproducible from explicit inputs.

Use:

- stable recipe/version identifiers;
- explicit integer seeds;
- bounded algorithms;
- fixed output dimensions/layouts where practical;
- tests that compare repeated generation and buffer bounds.

Avoid hidden process-time randomness, clock-based seeds, device-specific undefined behavior, or eye-dependent generation.

## Generation lifecycle

The preferred runtime sequence is:

1. module state is created without assuming GL;
2. GL context becomes available;
3. compact recipes generate temporary CPU data if needed;
4. GPU resources are allocated and uploaded once;
5. temporary CPU expansion buffers are released;
6. both eyes reuse the same immutable resources;
7. context loss deletes GPU objects;
8. recreation deterministically rebuilds them from the same recipe/seed.

Do not synthesize textures or immutable meshes every frame.

## Material system

### Current reference

Red Ledger uses four deterministic material families:

- stone;
- wood;
- metal;
- paper.

They are generated from a tiny hash/recipe function rather than bitmap files.

The current atlas is 128x128 and packs four 64x64 tiles. The authoritative utility exposes `GenerateMaterialAtlas` for the RGBA8 reference path and `GenerateMaterialAtlasRgb565` for direct packed generation. Red Ledger now uses the RGB565 path, reducing the GPU payload to 32 KiB from the earlier 64 KiB RGBA8 implementation.

This exact atlas size and format are not mandatory for every game. The reusable lesson is:

- group low-cost materials into a single atlas;
- prefer one texture binding over many small textures;
- generate once per GL context;
- reuse it for both eyes;
- choose the smallest format that preserves the intended look;
- validate quantization visually on the reference headset before generalizing it.

Nearest filtering is appropriate for deliberately crunchy art. Other filtering must justify its visual and performance cost.

## Material recipe design

A material recipe should describe visual structure rather than a baked image whenever practical.

Useful recipe ingredients include:

- seed/hash function;
- base value or palette;
- grain/stripe/brick/rule spacing;
- seams and borders;
- sparse flecks/noise;
- knots/rivets/marks;
- tile-local coordinates;
- optional tint supplied by vertex/material parameters.

Recipes should remain simple enough to test and reason about. A 200-line procedural shader that saves a 4 KiB texture is not automatically a win.

## Geometry and model generation

### Immutable geometry

Geometry that never changes during gameplay should be generated or assembled once and uploaded as immutable GPU data.

Red Ledger now bakes **three** separate 11-cube room batches (bar, stockroom and living quarters) in one world-space VBO: 1,188 vertices/47,520 bytes total, with only the current 396-vertex room drawn per eye. The original bar counter remains lowered with aligned interaction targets. Its game-private `red_ledger_signage.h` emits real uppercase lettering into one grouped immutable VBO, drawing only the current room's signs. An additional small status VBO is regenerated and uploaded **only after gameplay state changes** for customer requests, selected stock and ledger figures, reusing the same GLES2 program; it is not recomputed for both eyes or each frame. Interactive props are still separate. These are game-private implementations, not newly promoted native SDK APIs. Exact device legibility, GL performance and thermal proof remain pending.

The Red Ledger model pass also introduced a 180-vertex, 12-sided reusable hollow mug mesh (rolled lip and base) in the game-private `red_ledger_mug_mesh.h`, generated and uploaded once per GL context. The same VBO serves clean, dirty, carried and filled mugs, with a minimal handle and inserted liquid surface as needed. It uses the existing material shader and does not establish a public SDK mesh API; Galaxy S9 draw cost, stereo appearance and sustained thermal behavior remain unmeasured.

The standard lesson is to separate:

- static environment geometry;
- dynamic interactive geometry;
- transient effects/UI.

Do not pay per-object setup costs for immutable primitives when they can be safely batched.

### Primitive-first model language

The provisional shared model direction is a tiny recipe language built from primitives and transforms rather than a dependency on large general model formats for every asset.

A future shared model recipe should be able to express, at minimum:

- primitive type;
- transform;
- material/tile id;
- per-vertex or per-object tint;
- interaction/collision tag if needed;
- static versus dynamic classification.

The recipe should compile into tightly packed runtime geometry once. This is a design direction, not yet a stable public API.

Complex authored meshes remain valid when they are smaller, clearer, or materially better than a generated primitive approximation.

### Shared flat-colored primitive builder

`app/src/main/jni/native/reverie_native_flat_mesh.h` is a provisional C++17 CPU-only primitive builder used by all six classics and both recon boards. It emits deterministic XYZ/RGB triangle recipes; the caller owns GL upload, context recovery and update cadence. Snake and Chutes and Ladders now skip rebuilding/uploading when their visible state is unchanged. This is not a generalized serialized model system, and Galaxy S9 performance remains unmeasured.

### Classic layered geometry reference

The six packaged classics now construct immutable board/grid/title recipes once at GLES context creation through the existing flat-geometry helper and upload a `GL_STATIC_DRAW` buffer. The module releases expanded static CPU vertices and rebuilds them only after GL context loss. Moving game elements use an independent `GL_DYNAMIC_DRAW` buffer rebuilt when visible state changes. Each eye draws both unchanged buffers without stepping gameplay. This provisional, game-owned technique trades an additional draw call for reduced CPU work and GPU traffic; S9 thermal/frame-pacing acceptance remains open. It does not introduce a new native ABI.

## Interaction geometry

Game interaction does not require production-render mesh precision by default.

For work surfaces and low-complexity props, use simple analytic volumes such as AABBs, planes, spheres, capsules, or intentionally coarse convex regions when they provide stable pointing behavior.

Keep visual geometry and interaction geometry separately tunable so headset reach/comfort calibration does not require remodeling the art.

## Animation and procedural motion

Calculate shared animation state once per simulation update whenever both eyes should see the same result.

Use lightweight periodic functions, small state machines, or compact key tables before adding skeletal systems that the content does not need.

Red Ledger's light flicker is the current reference: calculate the illumination value once during update, then reuse it for both eye renders.

## Audio generation

There is not yet a standardized ReverieVR procedural-audio generator.

When one is introduced, it should follow the same principles:

- deterministic or explicitly event-driven;
- generated/cached outside timing-critical render work when possible;
- bounded memory;
- no duplicate per-eye work;
- shell-owned lifecycle/mixing respected;
- measured against small encoded samples rather than assumed to be cheaper.

Until then, procedural audio is a future SDK candidate, not a claimed platform feature.

## Content source hierarchy

Prefer, in order of practical efficiency rather than ideology:

1. tiny deterministic recipe;
2. compact static table or atlas;
3. deliberately small authored asset;
4. larger asset only when visual/production value justifies its cost.

The smallest *source file* is not always the cheapest runtime representation.

## Testing contract

A reusable generator should have host-side tests where practical for:

- identical seed -> identical bytes/vertices;
- changed seed -> intentional variation;
- buffer-size rejection before writes;
- index/UV bounds;
- finite coordinates;
- stable vertex counts;
- serialization/version compatibility where recipes persist;
- quantization/reference comparison for packed color formats.

Android/NDK build proof is necessary but does not replace headset validation.

## Energy and thermal contract

Never claim that procedural generation saves battery merely because it saves APK space.

For runtime-sensitive changes compare:

- CPU work during setup;
- allocations;
- GPU upload size;
- draw count;
- state changes;
- frame pacing;
- battery percentage trend over sustained runs;
- available thermal signals on the Galaxy S9.

Generation that happens once and removes recurring frame work is generally promising. Generation that saves bytes but adds continuous CPU/GPU work may be a loss.

## Standardization path

Test Chamber and Red Ledger are reference implementations, not permanent templates to copy line-for-line.

As Between Deliveries, Ministry of Intelligence, Iron Sight, and future native projects reuse these concepts, common recipe structures and helpers should move into a shared native SDK layer.

The end state should be that a game declares compact content through documented ReverieVR structures/functions rather than each module privately inventing another atlas builder, cube baker, material enum, save wrapper, or pointer adapter.

## Shared immutable game assets — 2026-11-04

Implemented **APK-local common asset namespace**: nineteen approved assets under `native/shared/v1/` (six original plus ten expanded owner-generated coastal textures and three compact, curated BWM1 Beckenburg prop derivatives), resolved through the unchanged host `read_asset` callback and `reverie_native_shared_asset_catalog.h`. Exact-path, module-ID grants enforce isolation; no directory traversal or arbitrary sibling-game resource access. Breakwater uses all nineteen and Omega Front uses the concrete/steel textures from the same packaged bytes. The associated source manifest is `shared-assets/catalog-v1.json` (Git blob IDs, available SHA-256, origin/usage and license cautions). A stdlib validator checks source bytes and that deprecated APK copies no longer exist. Legacy private read paths remain only as fallback for previously verified Breakwater content packs, not as duplicate packaged files.

For cross-*repository* development, consumers should pin an exact Git commit and blob hash, import only selected approved assets in their build pipeline, and preserve author/license attribution; copied source imports are **not** installed-runtime deduplication. A shared install-once content cache, reference-counted uninstall, signature-verified shared pack, and cross-package Android broker are **not implemented**. As of current one-APK policy, sharing prevents redundant bytes *inside that APK*, with no new network requirement; actual signed APK size and S9 runtime performance are unmeasured.


## Sitka game-private waterfront massing (source-only, 2026-11-04)

`sitka_shore_geometry.h` parses bounded `SITKA-SHORE-1` records and appends a one-time static shore mesh to the existing `.sitkam` terrain before the existing GLES2 batches upload. `P` records extrude each independently authored, simple polygon (including concave footprints) with stable feature IDs; they do not substitute a bounding-box lot for an irregular building. `B` records are restricted to rock/forest shore blocks, wharf/pontoon/pile, small harbor shack and bridge deck, while `R` is a sloped bridge-approach prism. Fixed three-tone vertex colors are an early PSX-style proxy, not a claim of textures, completed landmarks, public access, collision, tidal adjustment or geographic verification. Per-file limits: 65,536 UTF-8 bytes, 256 features, 32 exterior vertices per building; malformed/duplicate/self-intersecting records fail instead of producing plausible-looking fictional city blocks. No runtime random placement, per-eye mesh construction, or seafloor cache mutation occurs. The real-city source/provenance, chunked LOD and texture atlas remain separate pending work, documented in `docs/project/SITKA_IRREGULAR_CITY_AND_CUSTOM_BUILDINGS.md`.

## Sitka resident shoreline LOD overlay (source-only, 2026-11-04)

The game-private `sitka_shore_stream.h` maintains bounded `SITKA-SHORE-1`
feature metadata while selecting near P1 detail (80/112 m hysteresis), distant
P0 massing (440/490 m), or hidden by footprint-aware distance. Source building
IDs and irregular polygons are unchanged. The optional Sitka shore GPU layer
uses `TerrainGl::CreateOverlay` with no permanent base geometry and the same
transactional replacement used by the independent seafloor, rather than
reuploading terrain/boat geometry or compiling shaders in each eye. The host's
once-before-both-eyes `prepare_frame` callback commits visual tiers only
after successful upload; source failures retain previous visual shore batches
with 30-frame backoff. Full city asset paging, upload-time budgets, viewport
culling, collision/navigation, approved real footprint assets and S9 thermal/
frame-time validation remain **unimplemented/unverified**. The standalone
C++17 smoke test is `app/src/test/native/sitka_shore_stream_test.cpp`.
