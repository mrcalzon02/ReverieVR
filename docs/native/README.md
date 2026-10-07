# ReverieVR Native Game Developer Handbook

**Status:** living platform standard. Native ABI v1 is implemented; higher-level SDK conventions remain intentionally evolvable until they survive repeated game use and reference-device validation.

**Purpose:** define how ReverieVR-native games are designed, built, integrated, tested, optimized, documented, and eventually exposed through a stable game-facing API. This handbook is the canonical entry point for native-game development. It does not replace project doctrine, the execution contract, architecture records, or the ABI header.

## Authority and source of truth

Native-game work remains subordinate to:

- `docs/project/INSTRUCTIONS.md`;
- `docs/project/EXECUTION_CONTRACT.md`;
- `docs/project/BACKLOG.md`;
- `docs/project/ACCEPTANCE_LEDGER.md`;
- accepted records under `docs/records/`;
- the actual ABI declaration in `app/src/main/jni/native/reverie_native_module.h`.

For ABI field names, sizes, constants, and callback signatures, the header is authoritative. This handbook defines the engineering contract around that ABI.

## Active-truth requirement

This handbook describes the current platform, not an aspirational snapshot. `docs/native/CONTRACT_INDEX.md` maps normative documentation to authoritative implementation and defines the same-change synchronization rule.

When a documented native function, service, utility, lifecycle rule, input contract, persistence behavior, generator, or reusable technique changes, the corresponding documentation must be reviewed and updated in the same scoped change. If implementation evidence disproves a statement, correct or downgrade the statement immediately rather than leaving known-stale guidance.

Run `python3 scripts/verify-native-doc-sync.py` for factual source/document anchors, and use `--staged` or `--base HEAD^` to enforce mapped source/document co-change. Run `python3 scripts/verify-native-abi-contract.py` whenever ABI layout, prefix validation, optional tails, descriptor admission, or shared ABI helpers change; it compiles and executes the dependency-free host C++ contract test. Machine checks supplement semantic review; they do not excuse it.

## Goal

ReverieVR is not trying to become a general-purpose modern engine. The target is a compact native environment for deliberately restrained VR games that can run well on Daydream-era hardware.

By the time the native-game layer is considered mature, a new game should be able to start from a documented ReverieVR SDK contract rather than reverse-engineering Red Ledger, Test Chamber, Java shell code, or controller-provider internals.

The desired developer experience is:

1. define a game module and descriptor;
2. implement deterministic game state;
3. consume normalized host input and shell-owned platform services;
4. create compact GPU resources through documented lifecycle hooks;
5. render both eyes without mutating simulation state;
6. persist bounded module state through host save slots;
7. use generated or compact authored content according to the asset standard;
8. pass deterministic host tests;
9. pass reference-device interaction, stereo, frame-pacing, battery, and thermal gates;
10. graduate reusable techniques into the shared native SDK instead of copying them game-to-game.

## Platform ownership boundary

| Concern | ReverieVR shell/host owns | Native module owns |
|---|---|---|
| Head tracking and eye transforms | Yes | Consumes matrices only |
| Controller transport/calibration | Yes | Consumes normalized actions/rays |
| Global recenter/recovery | Yes | Must not trap or replace it |
| Global HUD/settings/comfort | Yes | Consumes platform policy |
| Activity lifecycle | Yes | Responds through ABI callbacks |
| Module trust/loading | Yes | Packaged allowlisted library only |
| Save isolation | Yes | Defines versioned game state within bounded slots |
| Game rules/simulation | No | Yes |
| Game-specific interaction targets | No | Yes |
| Game-specific geometry/material choices | No | Yes, within platform budgets |
| Cross-game reusable helpers | Shared SDK candidate | Must be promoted rather than cloned indefinitely |

The shell remains a platform around the game. A module is not allowed to become its own alternate headset runtime.

## Core native-game rules

### 1. Deterministic simulation first

Game state should be separable from rendering whenever practical. Inputs advance state during update; rendering observes state. Randomness should use explicit seeds or serialized state when it affects persistent outcomes.

This makes save/load, replayable tests, corruption checks, and stereo safety substantially easier.

### 2. One simulation update, two eye renders

Per-eye rendering must not advance timers, RNG, AI, economics, animation state, interaction state, or persistent state.

Any animation value shared by both eyes should be computed during update or from a stable frame value, then rendered identically for both eyes. Red Ledger's flicker migration from per-eye calculation to once-per-update is the reference pattern.

### 3. Host-normalized input, not device code

Modules consume actions and world-space pointer data supplied by the host. They do not parse Android input events, BLE packets, raw Daydream quaternions, or gamepad key codes.

ABI v1 still exposes movement axes for compatibility/game logic. If the shell has already applied locomotion to the shared camera/pointer frame, a module must not apply the same movement a second time to its view transform.

### 4. Shell-owned recovery always survives

A game must not consume or hide the only route to Quick Menu, Home, recenter, pause/exit, or Stage A recovery. Host-reserved controls remain host-reserved.

### 5. Create expensive resources outside the frame loop

Texture synthesis, static mesh generation, immutable batching, shader compilation, and similar setup work belongs in GL-context creation or another explicit bounded setup stage.

No per-frame procedural texture generation. No rebuilding immutable geometry per eye. No transient allocations in hot paths without evidence that they are harmless.

### 6. Treat GL context loss as normal

All GL objects are disposable. Modules create them in `on_gl_context_created`, release them in `release_gl_context`, and must survive recreation without corrupting game state.

Game state and GPU state are different things.

### 7. Restore host GL state

The module renders inside a shell-owned GL environment. It must not leave incompatible program, buffer, texture, blend, depth, cull, scissor, viewport, or attribute state behind.

Use the shared `reverie_native_gl_state.h` guard for the state it covers, including full vertex-attribute configuration/current values rather than only enable flags. If a module starts mutating additional GL state, restore that state explicitly or extend the shared guard and `GL_RENDERING_STANDARD.md` in the same scoped change.

Where the host contract already owns viewport/scissor or eye isolation, the module must not fight it.

### 8. Performance is correctness

A visually correct module can still be rejected for frame pacing, latency, battery drain, sustained heat, or stereo errors.

Compactness is not measured only in APK bytes. The goal is small code/data *and* low runtime work.

### 9. Procedural-first, not procedural-at-all-costs

A seed plus a tiny recipe is preferred when it is smaller, deterministic, visually adequate, and cheaper overall than shipping a large asset.

Authored assets remain valid when they are the more efficient representation. Device measurements decide energy claims.

### 10. No inert controls or fake capabilities

A game-facing button, switch, menu item, or setting must reach implemented behavior. Planned features remain in documentation/backlog until wired.

## Reusable-technique graduation rule

Every native game is expected to discover techniques that belong to the platform.

A game-specific technique should be promoted into the shared standard when:

- at least one real module demonstrates it;
- the behavior can be expressed without leaking that game's private rules;
- deterministic tests exist;
- lifecycle and error behavior are understood;
- reference-device cost is measured when performance-sensitive;
- a second module would otherwise need to duplicate it.

Promotion may take the form of a shared C/C++ helper, a host service, a recipe format, a documented convention, or a new ABI version. Promotion must not silently break ABI v1.

## Native module development sequence

### Phase A — simulation and contract

Define module id, display name, state model, input needs, persistence schema, and minimum graphics requirement. Start from `reverie_native_sdk.h` and require only the smallest host/input capability actually consumed. Build the core simulation so it can be tested without a headset where practical.

### Phase B — interaction

Use normalized host actions and pointer rays. Keep target selection deterministic and bounded. Prefer simple geometric hit tests before introducing heavier collision systems.

### Phase C — generated content and rendering

Generate immutable material/mesh resources once, batch static content, keep dynamic content deliberately small, and render against the host-provided eye matrices.

### Phase D — lifecycle and recovery

Exercise create/destroy, GL context create/release, pause/resume, module exit, Quick Menu suppression of guest input, save/load, and bad/corrupt state handling.

### Phase E — validation

Run `verify-native-doc-sync.py` and `verify-native-abi-contract.py`, then the relevant native/JVM deterministic checks, Android/NDK packaging checks, stereo-isolation verification, and finally real Galaxy S9 + Daydream View interaction and sustained performance/thermal testing.

### Phase F — standardization

Anything reusable learned from the module is either promoted to shared SDK documentation/helpers or explicitly recorded as game-specific.

## Documentation map

- `docs/native/CONTRACT_INDEX.md` — authoritative source-to-document map and synchronization rules.
- `docs/native/SYSTEMS_CATALOG.md` — discoverability index for implemented ABI/services/helpers/generators, reference techniques, game-private methods, and planned systems with explicit maturity labels.
- `docs/native/API_V1.md` — current implemented native C ABI and compatibility rules.
- `docs/native/SDK_HELPERS.md` — shared header-only capability/prefix helpers and canonical module-start pattern.
- `docs/native/GL_RENDERING_STANDARD.md` — shared GL-state isolation guard and renderer guest-state rules.
- `docs/native/CORE_UTILITIES.md` — proven cross-game matrix/shader helpers and utility graduation rules.
- `docs/native/RUNTIME_SERVICES.md` — shell-owned native runtime behavior including locomotion, pointer/recovery ownership, lifecycle, and persistence boundaries.
- `docs/native/PROCEDURAL_CONTENT_STANDARD.md` — generated materials, geometry, model-recipe direction, and asset/performance rules.
- `scripts/verify-native-doc-sync.py` — dependency-free factual/drift guard for the native documentation contract.
- `scripts/verify-native-abi-contract.py` + `app/src/test/native/native_abi_contract_test.cpp` — host-buildable executable regression gate for ABI prefixes, descriptor freeze, optional tails, capability checks, and GLES admission.
- `docs/project/NATIVE_GAME_PROJECT_POINTERS.md` — game-specific concept and scope pointers.
- `docs/project/BACKLOG.md` — implementation intent and acceptance gates.
- `docs/records/` — durable architectural decisions and tradeoffs.

## Maturity vocabulary

**Implemented** means the capability exists in authoritative source.

**Static accepted** means deterministic/source/build gates appropriate to the item have passed, but hardware validation may still remain.

**Device accepted** means the relevant Galaxy S9/Daydream runtime gate has passed.

**Provisional standard** means documented for repeated use but still allowed to evolve inside the current ABI.

**Stable API** means compatibility expectations are explicit and changes require an additive extension or a new ABI version.

The project should prefer a small stable surface over a large speculative one.
