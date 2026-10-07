# ReverieVR Procedural Content and Compact Asset Standard

**Status:** provisional native-game standard built from implemented Test Chamber and Red Ledger techniques. Hardware energy/thermal benefit remains evidence-driven.

## Purpose

ReverieVR deliberately treats recipes, seeds, compact tables, and generation code as first-class game assets.

The inspiration is the same broad discipline that made tiny procedural productions possible: store the *instructions for making the asset* when those instructions are smaller and cheaper than storing the expanded asset.

The goal is not to reproduce any specific historical engine. The goal is to build a ReverieVR-native content pipeline suited to small VR games on constrained Android hardware.

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

The current atlas is 128x128 and packs four 64x64 tiles. Red Ledger now generates the final atlas directly as RGB565, reducing the GPU payload to 32 KiB from the earlier 64 KiB RGBA8 implementation.

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

Red Ledger currently demonstrates the pattern by baking 11 static room/furniture cubes into one world-space VBO: 396 vertices and a 15,840-byte buffer, rendered in one static-room draw per eye. Interactive/hoverable props remain separate.

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
