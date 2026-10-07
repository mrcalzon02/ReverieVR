# ReverieVR Native Systems Catalog

**Status:** living discoverability index.

**Purpose:** answer a practical question for every new ReverieVR game: *what platform method, service, helper, generator, standard, or reference implementation already exists, and how mature is it?*

This catalog is intentionally broader than the public C ABI. It includes implemented shell services and proven reference techniques when they are useful examples, but it labels them separately so a reference implementation is never mistaken for a stable game-facing API.

## Maturity classes

- **ABI v1** — binary contract implemented now; compatibility rules are explicit.
- **Shared SDK / provisional** — reusable implementation consumed by multiple native modules, but still allowed to evolve compatibly.
- **Runtime service / provisional** — shell-owned behavior available to admitted native modules.
- **Implemented reference** — real production/reference code demonstrating a method, not yet a shared game-facing API.
- **Shell-only reference** — implemented ReverieVR shell technique that native games cannot call directly.
- **Game-private** — implemented inside one game; do not treat as platform API.
- **Planned** — design direction only; must not be exposed as an implemented capability.

## 1. ABI, admission, and compatibility

| System | Maturity | Authoritative source | Standard/reference | What it provides |
|---|---|---|---|---|
| Native module ABI v1 | **ABI v1** | `app/src/main/jni/native/reverie_native_module.h` | `docs/native/API_V1.md` | Module descriptor, lifecycle callbacks, input/eye structures, host services, optional capability tail. |
| Safe ABI consumption helpers | **Shared SDK / provisional** | `app/src/main/jni/native/reverie_native_sdk.h` | `docs/native/SDK_HELPERS.md` | Prefix/capability checks, safe host logging, exact frozen descriptor validation, GLES requirement comparison, safe optional feedback/capability access and overflow-safe finite pointer-ray sanitization shared by JNI and Red Ledger. |
| Native host / packaged loader | **Runtime service / provisional** | `app/src/main/jni/native/reverie_native_host.cpp` | `docs/native/API_V1.md`, `docs/native/RUNTIME_SERVICES.md` | Compile-time allowlist, packaged library loading, module-private save root, logging, feedback cue queue, JNI bridge. |
| Java/native module runtime | **Runtime service / provisional** | `app/src/main/java/io/github/mrcalzon02/reverievr/NativeModuleRuntime.java` | `docs/native/API_V1.md` | Java-side lifetime, pointer/update bridge, cached module capabilities, feedback-mask return path. |
| Executable ABI regression gate | **Implemented validation** | `app/src/test/native/native_abi_contract_test.cpp`, `scripts/verify-native-abi-contract.py` | `docs/native/CONTRACT_INDEX.md` | Host-buildable proof of prefix behavior, descriptor freeze, optional tails, feedback sanitization, capability gating, GLES admission, and malformed/overflowing pointer-ray rejection. |

### Current ABI-safe extension points

- append optional host services at the tail of `ReverieNativeHostV1` and expose a named minimum prefix;
- append optional fields at the tail of pointer-passed input/eye/capability structures when old consumers remain safe;
- append optional metadata at the tail of `ReverieNativeModuleApiV1`;
- **do not extend `ReverieNativeModuleDescriptorV1` under ABI v1** because it is embedded before callback pointers.

## 2. Host services available to native modules

| Service | Maturity | API surface | Current implementation |
|---|---|---|---|
| Logging | **ABI v1** | `host->log` | Android log bridge; bounded module diagnostics. |
| Module-private persistence | **ABI v1** | `read_save`, `write_save` | Bounded safe slot names, 64 KiB payload ceiling, atomic replace on write. |
| Shell feedback cues | **ABI v1 optional host tail** | `request_feedback` + `REVERIE_NATIVE_FEEDBACK_*` | Focus/activation/failure requests are sanitized, coalesced during update, and mapped to existing shell audio cues. |
| Arbitrary PCM/spatial audio | **Planned** | None | Not implemented. |
| Haptics | **Planned** | None | Not implemented. |

## 3. Module-declared capabilities

| Capability | Maturity | Source | Behavior |
|---|---|---|---|
| Shell-owned bounded locomotion | **ABI v1 optional capability** | `ReverieNativeModuleCapabilitiesV1`, `REVERIE_NATIVE_CAPABILITY_SHELL_LOCOMOTION` | Module declares X/Z envelope; host validates/caches it; shell applies one shared camera/pointer translation. |
| Per-module speed/deadzone | **Planned** | None | Current speed/deadzone remain shell policy. |
| Comfort-profile query | **Planned** | None | Global comfort preference work remains separate. |
| Mounted/world-anchored tool primitive | **Planned** | `docs/project/BACKLOG.md` RV-0220 | Intended for turrets, yokes, mounted guns, fixed levers; not a current SDK primitive. |

## 4. Input, pointer, recovery, and locomotion methods

| System | Maturity | Authoritative source | Standard |
|---|---|---|---|
| Base/pointer input prefixes | **ABI v1** | `app/src/main/jni/native/reverie_native_module.h` | `docs/native/API_V1.md` |
| Bounded view-relative locomotion | **Runtime service / provisional** | `app/src/main/java/io/github/mrcalzon02/reverievr/BoundedViewRelativeLocomotion.java` | `docs/native/RUNTIME_SERVICES.md` |
| Gesture re-arm gate | **Runtime service / provisional** | `app/src/main/java/io/github/mrcalzon02/reverievr/TouchpadLocomotionGate.java` | `docs/native/RUNTIME_SERVICES.md` |
| Shell integration / translated pointer frame | **Runtime integration** | `app/src/main/java/io/github/mrcalzon02/reverievr/VrShellRenderer.java` | `docs/native/RUNTIME_SERVICES.md` |
| Standard native hosted input profile | **Runtime service / provisional** | `app/src/main/java/io/github/mrcalzon02/reverievr/BuiltInBindingProfiles.java`, `app/src/main/java/io/github/mrcalzon02/reverievr/VrActivity.java` | `docs/native/RUNTIME_SERVICES.md` |
| Recovery ownership | **Platform rule** | shell lifecycle/input routing | `docs/native/README.md`, `docs/native/RUNTIME_SERVICES.md` |

Native modules consume normalized actions/rays. They do not parse BLE packets, Android key codes, or raw Daydream quaternions.

## 5. Rendering and graphics utilities

| System | Maturity | Authoritative source | Standard |
|---|---|---|---|
| Guest GL state isolation | **Shared SDK / provisional** | `app/src/main/jni/native/reverie_native_gl_state.h` | `docs/native/GL_RENDERING_STANDARD.md` |
| Column-major 4×4 multiply | **Shared SDK / provisional** | `app/src/main/jni/native/reverie_native_math.h` | `docs/native/CORE_UTILITIES.md` |
| GLES shader compile/program-link helpers | **Shared SDK / provisional** | `app/src/main/jni/native/reverie_native_gl_utils.h` | `docs/native/CORE_UTILITIES.md` | Shader compile plus caller-declared attribute bindings and fail-closed program linking; shader text/policy remain game-owned. |
| Headset/eye wireframe and collider probe | **Shell-only reference** | `app/src/main/java/io/github/mrcalzon02/reverievr/PlayerHeadRig.java` | `docs/records/PLAYER-HEAD-RIG-CONTRACT.md` | Center-head pose, IPD-separated eye markers, oriented-box collision and mirror-ready lines; no mirror renderer or game-facing ABI yet. |
| One update / two eye renders | **Platform rule** | ABI lifecycle + reference modules | `docs/native/README.md`, `docs/native/GL_RENDERING_STANDARD.md` |
| OpenGL ES admission | **ABI v1** | `ReverieNativeGlesRequirementSupportedV1` | Current host supports GLES 2.0 and rejects higher requirements before create. |

The GL state guard covers only state first-party modules actually mutate. It is not a blanket snapshot of the entire OpenGL state machine.

## 6. Materials and texture generation

| System | Maturity | Authoritative source | Standard |
|---|---|---|---|
| Deterministic material atlas | **Implemented reference / provisional generator** | `app/src/main/jni/native/procedural_material_atlas.h`, `app/src/main/jni/native/procedural_material_atlas.cpp` | `docs/native/PROCEDURAL_CONTENT_STANDARD.md` |
| RGBA8 material expansion | **Implemented reference** | `GenerateMaterialAtlas` | Deterministic CPU reference path. |
| Direct RGB565 material expansion | **Implemented reference** | `GenerateMaterialAtlasRgb565` | Current Red Ledger path; 128×128 atlas, 32 KiB GPU payload. |
| Material recipe language | **Provisional method** | stone/wood/metal/paper reference recipes | Seed + palette/value + structural marks/noise; not yet a generic serialized SDK format. |

### Material method

Prefer a compact deterministic recipe when the recipe is smaller, bounded, visually adequate, and cheaper overall than a shipped image. Generate outside the hot frame loop, upload once per GL context, release temporary CPU expansion data, and reuse the same GPU resource for both eyes.

## 7. Geometry and model generation

| System | Maturity | Authoritative source | Standard |
|---|---|---|---|
| Static cuboid world baker | **Game-private implemented reference** | `app/src/main/jni/native/red_ledger_static_geometry.h`, `app/src/main/jni/native/red_ledger_static_geometry.cpp` | `docs/native/PROCEDURAL_CONTENT_STANDARD.md` |
| Primitive-first model recipe direction | **Planned shared API** | No shared source yet | `docs/native/PROCEDURAL_CONTENT_STANDARD.md` |
| Analytic interaction volumes | **Provisional method** | Red Ledger target bounds/ray tests | `docs/native/PROCEDURAL_CONTENT_STANDARD.md` |
| Controller ghost spring geometry | **Shell-only reference** | `app/src/main/java/io/github/mrcalzon02/reverievr/ControllerGhostTether.java` | Allocation-free generated 65-vertex stereo-shared coil; not a native-game API. |

Red Ledger's current static baker emits 11 immutable cubes into one 396-vertex, 15,840-byte world-space VBO. That proves the batching method; it does **not** make Red Ledger's room recipe a reusable SDK model format.

The planned shared primitive model language should eventually express primitive type, transform, material/tile id, tint, interaction/collision tag, and static/dynamic classification. Until an actual shared structure/helper exists, keep it labeled planned.

## 8. Procedural motion and generated audio references

| System | Maturity | Authoritative source | Status |
|---|---|---|---|
| Update-time shared animation state | **Platform method** | Red Ledger flicker reference | Compute once per simulation update; render identical state to both eyes. |
| Generated controller spring waveform | **Shell-only reference** | `app/src/main/java/io/github/mrcalzon02/reverievr/ControllerSpringWaveform.java` | Deterministic 22,050 Hz / 460 ms PCM cue generated without an asset. |
| General native procedural-audio generator | **Planned** | None | Not standardized and not exposed to native modules. |

The existence of the controller spring waveform does not make procedural audio a native SDK service. It is evidence for a technique that may graduate later if real native games need the same facility.

## 9. Validation methods

| Gate | Maturity | Command/source | Purpose |
|---|---|---|---|
| Documentation/source drift | **Required for mapped SDK changes** | `python3 scripts/verify-native-doc-sync.py` | Factual anchors and same-change documentation mapping. |
| ABI executable regression | **Required for ABI/helper changes** | `python3 scripts/verify-native-abi-contract.py` | Compile/run structural compatibility checks with host C++17 compiler. |
| Native deterministic tests | **Implemented where applicable** | `app/src/test/native/` | Simulation, material, geometry, ABI checks. |
| JVM deterministic tests | **Implemented where applicable** | `app/src/test/java/` | Shell motion/input/controller behavior. |
| Android/NDK package build | **Required before static acceptance when affected** | project build/release process | Target ABI compilation/link/package proof. |
| Galaxy S9 + Daydream runtime | **Required for device acceptance** | physical reference hardware | Stereo, comfort, interaction, frame pacing, battery/thermal evidence. |

## 10. Promotion rule: reference → shared SDK

Promote a technique only when:

1. at least one real implementation proves it;
2. a second consumer would otherwise duplicate the same platform behavior, or the behavior is intrinsically host-owned;
3. the smallest common contract can be stated without leaking one game's rules;
4. lifecycle/error behavior is understood;
5. deterministic validation exists;
6. performance-sensitive behavior has appropriate device evidence before performance claims become normative;
7. implementation, documentation, catalog entry, and drift checks move together.

Examples already promoted by this rule include ABI prefix helpers, safe host logging, GLES admission, matrix multiplication, shader compilation/program linking, GL state isolation, module-declared locomotion, and shell feedback cues.

Examples **not** yet promoted include Red Ledger work-target rules, its exact cuboid room recipe, arbitrary audio mixing, haptics, and the planned generic primitive model recipe.

## 11. Source-of-truth rule

This catalog is an index, not a substitute for the authoritative files linked above.

When an implementation is removed, renamed, promoted, or downgraded, update its normative standard and this catalog in the same standardization slice. A new shared native utility is not considered standardized until it appears in:

- authoritative implementation;
- its normative documentation;
- `docs/native/CONTRACT_INDEX.md`;
- this catalog;
- the machine drift guard where the fact can be checked.
