## Gumball pointer focus priority correction — 2026-10-16

The game-private `gumball_menu_hit.h` `MenuPointerFocus` fixes incremental ray motion failing to reclaim D-pad focus. It is consumed by the existing Gumball module without changing the shell input ABI or adding a second UI. The exact helper and extended menu-hit regression passed strict host GCC/Clang and Clang sanitizer; game GLES2/APK/device remains draft.

## Gumball native clickable vector UI and atomic shipyard repairs — 2026-10-16

Game-private `gumball_shipyard_menu.h` and `gumball_menu_hit.h` model controller/palette focus, exact rendered hit rectangles and click-target IDs without Android dependencies; `gumball_shipyard.h` implements connected and stock-preserving atomic part replacements. Candy game's native module now uses the preexisting `reverie_native_vector_text.h` stroke font and actual pointer-ray hit targets, with all UI targets reachable via pad fallback. The strict isolated C++17/sanitizer regression and source-parity checks passed; complete GLES2 module, S9 binocular reading, controls and sustained frame budget remain unaccepted.

## Gumball automatic emergency consumable reference — 2026-10-16

Game-private `SupportContracts` demonstrates two cheap, one-use, non-stackable repair/tonic purchases with real cash cost, condition-triggered effects and limited V3 campaign-state serialization. The native Gumball Support UI controls real purchases; flight automatically deploys at a measurable damage threshold and attempts an immediate existing-slot save on consumption. Helper/test source byte-matches a locally GCC/Clang/ASan/UBSan-passing test. Complete game/Android/Daydream integration remains **Development-only, not device verified**. This does not extend the shared native module SDK. See `docs/project/GUMBALL_ARMADA_ROUTES_AND_SUPPORT.md`.

## Gumball Armada paid allied contract and lane-rival showcase — 2026-10-16

Game-private `gumball_support_contracts.h` and `gumball_support_contracts_test.cpp` illustrate independent capped ally progression (two trees per hired pilot), a limited per-sortie booking and cost debit recorded in a V3 campaign snapshot with V1/V2 restoration. The Gumball module source uses it for actual paid Éclair allied fire and Macaron hull/shield/salvage rescue, alongside Cinnamon zoned warning fire, Sydney alternating control-rate halves and Jawbreaker Jules heavy counterable volleys. Chart destination warnings derive from the same seeded selector used in combat. Exact standalone support header/test GCC/Clang/ASan/UBSan verified; complete native game and Daydream acceptance remain pending. This is **game-private draft code**, not a new shared ABI or finished portrait/voice pack. See `docs/project/GUMBALL_ARMADA_ROUTES_AND_SUPPORT.md`.

## Gumball Armada named encounter extension — 2026-10-16

The game-private cast scheduler now selects Nib's mine-layer or Biff and Baff's paired bomb ships after progression, in addition to earlier encounters. Their effects use the existing fixed enemy and projectile pools. The dual-destruction reward contract has passed exact-source local C++17 compiler and sanitizer checks. Integrated APK, headset controls, artwork and voice remain unverified. See `docs/project/GUMBALL_ARMADA_CAST_BIBLE.md`.

# ReverieVR Native Systems Catalog

**Status:** living discoverability index.

**Purpose:** answer a practical question for every new ReverieVR game: *what platform method, service, helper, generator, standard, or reference implementation already exists, and how mature is it?*

This catalog is intentionally broader than the public C ABI. It includes implemented shell services and proven reference techniques when they are useful examples, but it labels them separately so a reference implementation is never mistaken for a stable game-facing API.

## Omega Front tactical module example — 2026-10-15

`omega_front_tactics.h`, `omega_front_controls.h` and `omega_front_module.cpp` form a Development-only game-private source example. The native host and Android.mk allowlist/target plus Daydream `native-classics` touchpad routing are source-wired. These are not new shared SDK facilities, and real APK/device verification remains pending. See `docs/native/OMEGA_FRONT_TACTICS.md`.

## Gumball Armada bounded candy ship construction — 2026-10-16

The game-private `gumball_shipyard.h` and native shipyard view demonstrate a 7×7 connected component lattice with finite starter inventory, structural disconnection prevention, per-part mass/power and launch readiness, independently checksummed `shipyard.v1` persistence, and actual weapon mount origins and hull/shield/thrust/cargo effects on arcade simulation. `gumball_shipyard_test.cpp` runs strict portable GCC/Clang and Clang sanitizers in `scripts/verify-gumball-armada.sh`. **Source integrated / local helper verified only; APK/Galaxy S9 acceptance pending.** It is not a shared platform SDK, nor a fully implemented purchased ship parts economy. See `docs/project/GUMBALL_ARMADA.md`.

## Gumball Armada first cast encounter scheduler — 2026-10-16

Source-only game-private `gumball_cast_encounters.h` + `gumball_cast_encounters_test.cpp` provides deterministic capped guest selection and a five-pulse companion budget. Gumball's simulation and GLES2 module source now consume it for Pip's actual projectiles, Lolly's real shield/consumed salvage assistance and Blort's counterable bomb volleys, without ABI change or new heap pools. **Local isolated C++17/sanitizer kernel tests passed; full integrated module and Galaxy S9 acceptance pending.** See `docs/project/GUMBALL_ARMADA_CAST_BIBLE.md` for the remaining planned roster/portraits.

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
| Safe ABI consumption helpers | **Shared SDK / provisional** | `app/src/main/jni/native/reverie_native_sdk.h` | `docs/native/SDK_HELPERS.md` | Prefix/capability checks, safe host logging, exact frozen descriptor validation, GLES requirement comparison, safe optional feedback/capability access, finite base-input sanitization (time/axes/buttons), overflow-safe pointer-ray sanitization, and finite/indexed stereo-eye validation shared by JNI and both reference games. |
| Native host / packaged loader | **Runtime service / provisional** | `app/src/main/jni/native/reverie_native_host.cpp` | `docs/native/API_V1.md`, `docs/native/RUNTIME_SERVICES.md` | Compile-time allowlist, packaged library loading, module-private save root, logging, feedback cue queue, JNI bridge. |
| Java/native module runtime | **Runtime service / provisional** | `app/src/main/java/io/github/mrcalzon02/reverievr/NativeModuleRuntime.java` | `docs/native/API_V1.md` | Java-side lifetime, pointer/update bridge, cached module capabilities, feedback-mask return path. |
| Executable ABI regression gate | **Implemented validation** | `app/src/test/native/native_abi_contract_test.cpp`, `scripts/verify-native-abi-contract.py` | `docs/native/CONTRACT_INDEX.md` | Host-buildable proof of prefix behavior, descriptor freeze, optional tails, feedback sanitization, capability gating, GLES admission, malformed/overflowing pointer-ray rejection, non-finite/overrange base-input rejection, and invalid stereo-eye index/matrix rejection. |

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
| Shell feedback cues | **ABI v1 optional host tail** | `request_feedback` + `REVERIE_NATIVE_FEEDBACK_*` | Focus/activation/failure plus fixed weapon-fire/reload-start/reload-ready cues, coalesced and delivered to shell audio. |
| Generated gameplay sound bank | **Shell implementation / provisional** | `app/src/main/java/io/github/mrcalzon02/reverievr/NativeCombatWaveform.java`, `app/src/main/java/io/github/mrcalzon02/reverievr/NativeCombatSound.java` | Three deterministic mono 22,050 Hz PCM samples reused through static AudioTracks; no arbitrary audio/PCM API. |
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
| Flat colored mesh recipes | **Shared SDK / provisional** | `app/src/main/jni/native/reverie_native_flat_mesh.h` | `docs/native/CORE_UTILITIES.md` | Header-only parameterized XYZ/RGB triangles, rectangles, segments and discs used in the six classics and two recon boards. |
| Column-major 4×4 multiply | **Shared SDK / provisional** | `app/src/main/jni/native/reverie_native_math.h` | `docs/native/CORE_UTILITIES.md` |
| GLES shader compile/program-link helpers | **Shared SDK / provisional** | `app/src/main/jni/native/reverie_native_gl_utils.h` | `docs/native/CORE_UTILITIES.md` | Shader compile plus caller-declared attribute bindings and fail-closed program linking; shader text/policy remain game-owned. |
| Headset/eye wireframe and collider probe | **Shell-only reference** | `app/src/main/java/io/github/mrcalzon02/reverievr/PlayerHeadRig.java` | `docs/records/PLAYER-HEAD-RIG-CONTRACT.md` | Center-head pose, IPD-separated eye markers and oriented-box collision; white-room diagnostic mirror uses `HeadsetMirrorProjection` to reflect the headset and handset proxy in stereo, not full scene reflections or a game-facing ABI. |
| One update / two eye renders | **Platform rule** | ABI lifecycle + reference modules | `docs/native/README.md`, `docs/native/GL_RENDERING_STANDARD.md` |
| OpenGL ES admission | **ABI v1** | `ReverieNativeGlesRequirementSupportedV1` | Current host supports GLES 2.0, rejects invalid zero-major versions and higher requirements before create. |

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
| Lantern Desk evidence/camera kernel | **Game-private; host-tested proof only** | `app/src/main/jni/native/lantern_desk_simulation.{h,cpp}`, `app/src/test/native/lantern_desk_simulation_test.cpp` | `docs/project/NATIVE_GAME_PROJECT_POINTERS.md` | 16×16 deterministic recon pixels, three sector variants and private adjudication; now also linked into a Development-only GLES2 evidence board module (source only; Android/S9 proof pending). |
| Forward Detachment patrol kernel | **Game-private; host-tested proof only** | `app/src/main/jni/native/forward_detachment_simulation.{h,cpp}`, `app/src/test/native/forward_detachment_simulation_test.cpp` | `docs/project/NATIVE_GAME_PROJECT_POINTERS.md` | Two-route resource/risk loop, ambiguous contact evidence, decision consequences and debrief; now also linked into a Development-only GLES2 decision board module (source only; Android/S9 proof pending). |
| Controller ghost spring geometry | **Shell-only reference** | `app/src/main/java/io/github/mrcalzon02/reverievr/ControllerGhostTether.java` | Allocation-free generated 65-vertex stereo-shared coil; not a native-game API. |

Red Ledger's current static baker emits 11 immutable cubes into one 396-vertex, 15,840-byte world-space VBO. That proves the batching method; it does **not** make Red Ledger's room recipe a reusable SDK model format.

The planned shared primitive model language should eventually express primitive type, transform, material/tile id, tint, interaction/collision tag, and static/dynamic classification. Until an actual shared structure/helper exists, keep it labeled planned.

| Breakwater wave/landing economy | **Game-private native prototype** | `app/src/main/jni/native/breakwater_campaign.{h,cpp}`, `app/src/test/native/breakwater_campaign_test.cpp` | `docs/project/BREAKWATER_BATTERY_VR.md` | Seeded rowboats and landing craft, infantry interception, late aircraft, wave clock, shop and autonomous turret; not a shared engine utility. |
| Gumball Armada mission and salvage rules | **Game-private Development source** | `app/src/main/jni/native/gumball_arcade_objectives.h`, `app/src/test/native/gumball_arcade_objectives_test.cpp` | `docs/project/GUMBALL_ARMADA.md` | Three executable route objectives, separate interception deadline/escape limits, bounded 24-slot falling gumdrop cash pickups and integrity-free transient sortie state; GCC/Clang/Clang sanitizer local helper tests passed, integrated APK and S9 proof pending. |
| Gumball Armada deterministic flight campaign | **Game-private Development module** | `app/src/main/jni/native/gumball_armada_simulation.h`, `app/src/main/jni/native/gumball_armada_module.cpp`, `app/src/test/native/gumball_armada_test.cpp` | `docs/project/GUMBALL_ARMADA.md` | Seeded 4,096-node candy-transit chart, finite enemy/projectile arrays, bounded flight/combos, currency, 4 upgrades, fortress bosses and checksum-validated saves; GLES2 source stage, device acceptance pending. |
| Breakwater procedural GLES2 gun scene | **Game-private Development module** | `app/src/main/jni/native/breakwater_module.cpp` | `docs/project/BREAKWATER_BATTERY_VR.md` | Generated sea and low-poly boats, glyph shop, aim/fire interactions; physical acceptance pending. |

## 8. Procedural motion and generated audio references

| System | Maturity | Authoritative source | Status |
|---|---|---|---|
| Update-time shared animation state | **Platform method** | Red Ledger flicker reference | Compute once per simulation update; render identical state to both eyes. |
| Generated controller spring waveform | **Shell-only reference** | `app/src/main/java/io/github/mrcalzon02/reverievr/ControllerSpringWaveform.java` | Deterministic 22,050 Hz / 460 ms PCM cue generated without an asset. |
| General native procedural-audio generator | **Planned** | None | Not standardized and not exposed to native modules. |
| Original MIDI-era sequence/synth | **Planned cross-game service** | `docs/native/MIDI_MUSIC_STANDARD.md` | Composed note events and a future host-owned synth; **no current native-game audible MIDI**. |

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
