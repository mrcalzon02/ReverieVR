## Test Chamber reusable wardrobe mesh adoption — 2026-10-26

The original optional `ReverieNativeHostV1.read_avatar` ABI tail is
unchanged. The Test Chamber now additionally calls the shared
`ReverieNativeWriteAvatarWardrobeV1` before its GLES2 vertex-buffer
upload; it uses the returned visibility mask and material tint
function to render garment/costume parts on the same selected global
avatar. No new host ABI fields or game save schema are introduced.
This is source-level adoption pending full JNI/NDK/APK and headset
acceptance.

## Shared wardrobe avatar host service and ellipsoid body — 2026-10-24

**Implemented in source; Android/S9 acceptance pending.** The shell now hands
its saved Home avatar to every **hub-hosted** native module by a read-only,
optional tail callback `read_avatar` on `ReverieNativeHostV1`. It follows
`read_asset` and preserves all preceding ABI-v1 offsets and existing module
entries. The stable `ReverieNativeAvatarV1` is 2 headers plus 15 ordered
`uint32_t` indices: theme, accent, hat, accessory, eyes, presentation,
physique, height, shoulders, waist, hips, musculature, outfit, hairstyle,
fabric. `REVERIE_NATIVE_AVATAR_FIELD_COUNT_V1=15` and the snapshot
`schema_version=1`; this snapshot is distinct from the Java preferences
string format `v2`. Consumers must first check
`REVERIE_NATIVE_HOST_V1_AVATAR_MIN_SIZE` and use
`ReverieNativeReadAvatarV1`. The helper rejects malformed/unknown fields
and pre-extension hosts; the host falls back to a valid default profile if
JNI data is missing. Call the reader in a guest callback and cache the result
in module state as needed; the snapshot is pinned at module launch. Game saves
never store nor own the global avatar profile.

The JVM reads `ReveriePreferences.getHomeAvatarProfile()` at launch and
passes the 15-value validated profile to `nativeCreate`. The JNI host stores
a copy in the native session, accessible only during that module's callback,
without allowing access to any other game files. This does not constitute an
inter-app Android data bridge for independently installed companion APKs.

`reverie_native_avatar_geometry.h` supplies low-cost first-party **stretched
spheres** for each portion of a reusable seven-piece 3D figure. Torso, hips,
both arms, both legs and head are shaped from the same profile. The default
collision primitive `ReverieNativeAvatarCollisionV1` is an **ellipsoid**,
a stretched sphere with radius 0.27m horizontally, radius 0.85m vertically,
center 0.85m above the feet. It is the standard player collision shape,
independent of visual body, theme, clothing, and gender presentation.
This is mathematically an ellipsoid, not a cylindrical capsule with
hemispherical ends. The collision helper is provided for opted-in physics
implementations; preexisting native game physics is not silently changed.

The Procedural Test Chamber is the first native consumer; it obtains the
snapshot and prepares a visible seven-ellipsoid low-poly character on GL
creation, preserving its existing procedural scene and stereo contract.
Other hosted modules **have access to the avatar immediately**, but must
invoke the shared render helper or attach their own rigged asset to actually
display that body. Availability is not the same as visual adoption.

## Standalone Test Chamber ABI reuse (2026-10-17, source only)

The separate `:companion-test-chamber` Gradle application includes the existing C++ `test_chamber_module.cpp` directly, preserving its unchanged `reverie_native_module_entry_v1` ABI-v1 implementation. `game-apps/test-chamber/src/main/jni/companion_test_chamber_bridge.cpp` validates module identity, native ABI and required GLES2, and calls the real create/update/per-eye render/GL-release/destroy callbacks through a module-local JNI bridge. Game code remains independently packaged, but this cannot be called a verified APK or proof of Galaxy S9 behavior without NDK, Cardboard lifecycle and handset checks. Core hub native host/ABI header remain unchanged.

## Immutable downloaded game-content roots (2026-10-17)

The native host now receives an optional validated **game-local** active content directory when a built-in game is opened. `HostReadAsset` reads module-scoped files from that immutable revision via bounded `open(O_NOFOLLOW)`/read, and falls back to packaged APK assets when the requested entry is absent. The Java `RvPackInstaller` decides which verified revision is active and `NativeModuleRuntime` pins that root for the duration of the game's session. This adds only an internal Java/JNI launch argument, not a field to the frozen game ABI v1. No native C++ executable file is loaded from this directory.

## Scoped Beckenburg room art — 2026-10-17

The host's ABI-v1 read-only `read_asset` callback now admits `native/red-ledger/beckenburg/*.rlprop` for Red Ledger, alongside its existing original icons/models/textures. `reverie_native_asset_scope.h` enforces separate game IDs, extensions, length and bounded byte sizes without opening arbitrary filesystem paths. The game still requires packaged binary assets; this change alone does not prove that the room art appears on the Galaxy S9. Host ABI layout and existing Omega Front PNG access remain unchanged.

## Omega Front native admission — 2026-10-15

The Development-only `omega-front` native module is registered in the built-in host allowlist as `libreverie_module_omega_front.so`, has a corresponding Android.mk shared target and uses unchanged native ABI v1 with a real `reverie_native_module_entry_v1`. It owns one 24×24 battle controller and GLES2 war table, with a single update per simulation tick and read-only stereo eye passes. No C ABI fields were added. Java confines it to Development logging mode. NDK build, ABI exports inside an APK, installation and Galaxy S9 checks remain unverified.

## Gumball Armada hosted module and admission (2026-10-15)

The packaged `gumball-armada` module is an original Development-only GLES2 native game: compile-time host allowlist, unchanged ABI-v1 entry/lifecycle, module-private `campaign.v1` save slot (version-2 binary payload, with validated legacy version-1 migration preserving position/currency/upgrades) and bounded schema/seed/checksum validation, and no additional native ABI methods. `NativeModuleRuntime` rejects the title outside Development logging mode. Built-in module count increases by one; APK library/entry verification and Galaxy S9 Daydream acceptance must be re-run before release claims. The original project design is at `docs/project/GUMBALL_ARMADA.md`.

## Module-reported shader failures in native GL initialization (2026-10-13)

While invoking a guest's `on_gl_context_created` callback, the ABI-v1 host now retains only the latest bounded `REVERIE_NATIVE_LOG_ERROR` message associated with that active module session. If the callback fails, it appends this *guest-provided* detail to the existing module-id and GLES error-code failure. This preserves genuine shader compiler/linker messages for the headset's native launch-failure text and full diagnostics, rather than replacing them with an uninformative `guest_GL=0x0` when the driver reports a shader compile failure without setting a GL error flag. The captured detail is cleared at the next GL creation attempt, never affects callback success, and does not add any ABI field or change successful launch behavior.

## APK-level native library loading contract (2026-10-13)

The signed phone-test verification now executes `scripts/verify-native-module-exports.py` against the final installable APK. It derives the registered module library names directly from the C++ host allowlist, requires exact agreement with `Android.mk`, and verifies every registered module plus the JNI host for both Android ABIs. It reads each shipped ELF shared object, checks ARM versus AArch64 architecture, verifies the globally exported ABI-v1 module entry point (plus native JNI host entry functions), and checks packaged private `DT_NEEDED` dependencies. An absent, mismatched, or hidden entry symbol fails the release instead of producing a home-menu bounce on-device. This is binary static verification only: actual `dlopen`, GL resource creation, stereo rendering, input and Galaxy S9 compatibility remain device acceptance gates.

## Native GL initialization error isolation (2026-10-13)

The host's pre-guest OpenGL error guard now covers **both** `on_gl_context_created` and `render_eye`, not only rendering. The same bounded `glGetError` drain reports the first inherited shell/Cardboard error once per module session and clears it **before** a guest callback. Guest GL errors are never cleared by the guard after an otherwise successful callback; they still fail module-defined initialization or drawing checks. If GL creation fails, the host records the module id and the remaining guest GL error code, not just a generic failure. No native ABI fields, public lifecycle ordering, camera, GL resources, or input calibration were changed. Compilation and Galaxy S9 load/render proof remain separate validation gates.

## Native module render error ownership (2026-10-13)

The packaged Java/JNI native host now drains and reports **inherited** GLES error flags immediately before each allowlisted native module's `render_eye` callback, so a pre-existing Cardboard/shell GL error cannot be misreported by a first-party game's own `glGetError` as a failed render. It does **not** suppress errors generated by the guest: first-party modules still fail their draw check on their own errors, and a failure includes the module id and eye index in the host error. The host links GLESv2 for this boundary check. The Android phone-test APK verification now checks packaged `.so` files for all eleven registered native modules on each of the two ABIs; existence in the APK is not equivalent to a successful dlopen, context creation or real device playthrough.

# ReverieVR Native Module API v1

**Status:** implemented ABI. This document describes the current game-facing C contract and the rules required to use it safely.

**Authoritative declaration:** `app/src/main/jni/native/reverie_native_module.h`

**Synchronization:** this file is governed by `docs/native/CONTRACT_INDEX.md`. Changes to the ABI header, native host services, or `NativeModuleRuntime` require same-change review/update of this reference and should pass both `python3 scripts/verify-native-doc-sync.py` and the host-buildable `python3 scripts/verify-native-abi-contract.py` gate when ABI structure/consumption behavior is affected.

## ABI identity

- ABI version: `REVERIE_NATIVE_MODULE_ABI_VERSION == 1`
- exported module entry symbol: `reverie_native_module_entry_v1`
- module API type: `ReverieNativeModuleApiV1`
- host service type: `ReverieNativeHostV1`
- input type: `ReverieNativeInputV1`
- eye type: `ReverieNativeEyeV1`
- optional capability type: `ReverieNativeModuleCapabilitiesV1`
- maximum single save payload: `65536` bytes

Native modules are trusted libraries packaged inside the APK and admitted through the host's compile-time allowlist. ABI v1 is deliberately *not* a plugin loader for arbitrary code from writable storage.

**2026-10-13 catalogue review:** the host allowlist includes Development-only `ministry-lantern-desk` and `iron-sight-forward-detachment`. Their two GLES2 libraries compile a common finite-state presentation source against separate original C++ kernel sources, reuse the same ABI v1, and are gated both at enumeration and launch. No ABI structure, callback or persistence protocol changed. Their source board interactions are not evidence of a signed APK or Galaxy S9 validation.

## Entry point

Every ABI-v1 module exports:

`const ReverieNativeModuleApiV1 *reverie_native_module_entry_v1(void)`

The returned table is owned by the module and must remain valid for the loaded library lifetime.

The host validates the API table, descriptor, ABI version, required callbacks, OpenGL ES requirement, and module id before creating an instance.

## Structure compatibility

Every public ABI structure begins with `struct_size`. ABI v1 extensions are append-only.

A consumer must validate that a structure is large enough for the **fields it actually intends to access**, using the named minimum-prefix constants in the header rather than comparing against the newest total `sizeof(struct)`. Existing fields must not be reordered or reinterpreted.

Current minimum-prefix constants are:

- `REVERIE_NATIVE_HOST_V1_LOG_MIN_SIZE` — host header plus logging service;
- `REVERIE_NATIVE_HOST_V1_SAVE_MIN_SIZE` — host prefix through read/write save services;
- `REVERIE_NATIVE_HOST_V1_FEEDBACK_MIN_SIZE` — host prefix through shell feedback cue requests;
- `REVERIE_NATIVE_HOST_V1_SOUND_MIN_SIZE` — optional host prefix through ordered named sound cue requests;
- `REVERIE_NATIVE_DESCRIPTOR_V1_SIZE` — exact frozen ABI-v1 descriptor size;
- `REVERIE_NATIVE_DESCRIPTOR_V1_MIN_SIZE` — compatibility alias to that exact frozen size;
- `REVERIE_NATIVE_INPUT_V1_BASE_MIN_SIZE` — base timing/movement/button input;
- `REVERIE_NATIVE_INPUT_V1_POINTER_MIN_SIZE` — input prefix through world-space pointer data;
- `REVERIE_NATIVE_EYE_V1_MIN_SIZE` — mandatory eye matrices;
- `REVERIE_NATIVE_MODULE_API_V1_MIN_SIZE` — mandatory module callback table through `render_eye`;
- `REVERIE_NATIVE_MODULE_API_V1_CAPABILITIES_MIN_SIZE` — optional API tail through the `capabilities` pointer;
- `REVERIE_NATIVE_CAPABILITIES_V1_LOCOMOTION_MIN_SIZE` — capability block prefix through shell-locomotion bounds.

The native host and both reference modules use these prefixes. This makes the append-only rule operational for structures whose tails can actually grow without moving existing fields.

### Descriptor layout exception

`ReverieNativeModuleDescriptorV1` is **layout-frozen** for ABI v1. It is embedded by value inside `ReverieNativeModuleApiV1` before the callback pointers, so appending a descriptor field would shift every callback offset. The v1 helper therefore requires `descriptor.struct_size == REVERIE_NATIVE_DESCRIPTOR_V1_SIZE`.

Do not place future capability declarations by appending to the v1 descriptor. The implemented `ReverieNativeModuleCapabilitiesV1` pointer demonstrates the compatible pattern: add optional metadata at the **tail of `ReverieNativeModuleApiV1`** and prove its presence from `struct_size`, or introduce ABI v2 when the change cannot remain optional.

Adding optional tail fields can remain ABI v1 only when all existing field offsets stay unchanged and old producers/consumers can safely ignore the added tail. Adding a new mandatory callback, changing existing semantics incompatibly, extending the embedded v1 descriptor, or requiring a different mandatory layout requires a new ABI version.

## Module descriptor

`ReverieNativeModuleDescriptorV1` contains:

- structure size;
- ABI version;
- stable module id;
- display name;
- required OpenGL ES major/minor version.

The current host requires the descriptor id to match the allowlisted package entry.

The native module host currently provides **OpenGL ES 2.0**. It compares both descriptor version components through `ReverieNativeGlesRequirementSupportedV1`: required and host major versions must be nonzero; GLES 0.x is rejected as invalid, requirements from valid lower majors are admitted, 2.0 is admitted, and requirements above 2.0 (including 2.1 or 3.0) are rejected before module creation.

Module ids are treated as storage/security identifiers, not display strings. Current runtime policy accepts simple ids made from letters, digits, `-`, `_`, and `.`, with a maximum length of 80 characters.

## Host services

`ReverieNativeHostV1` exposes seven callbacks with ordered-sound, module-scoped asset access, and avatar identity appended to the original ABI-v1 host prefix.

### Logging

`log(level, tag, message)`

Use this for bounded module diagnostics. Do not emit high-volume per-frame logs during normal operation.

Supported levels are debug, info, warn, and error.

### Read save

`read_save(slot, buffer, capacity, out_size)`

### Write save

`write_save(slot, data, size)`

Save slots are scoped by the host to the active packaged module. A module does not receive an arbitrary filesystem path.

### Ordered sound cues (optional latest ABI-v1 tail)

`request_sound(cue_id)` requests one shell-owned, validated sound per
successful action. It is **not** a bitmask and cannot accept PCM, paths,
loop sources or arbitrary cue IDs. IDs `REVERIE_NATIVE_SOUND_WEAPON_MACHINE` through `REVERIE_NATIVE_SOUND_WEAPON_CANNON` (1–5) retain the five Breakwater weapon reports. IDs 6–15 provide hull impact, water splash, boat destruction, gunboat salvo, infantry volley, bomber, strafer, SAM launch, wave start, and shoreline collision. IDs 16–19 are successful shop purchase, completed wave, campaign victory and battery defeat; IDs 20–21 are confirmed deployed-armored-car cannon fire and armored-car destruction. All use the same eight-entry ordered byte queue and preloaded phone PCM16 bank; no additional ABI struct fields, arbitrary files, paths or per-eye sound callbacks. Breakwater's passive song remains the separate owner-original MP3 `Mars des Souvenirs`, looped by `ShellMenuMusic`. Device mix verification is pending. Host-side `ReverieNativeSoundQueueV1` bounds each
packet to eight ordered one-byte IDs; the shell drains once per native
update using `NativeModuleRuntime.nativeDrainSoundCues()`, forwarding
each ID to the same reusable `NativeCombatSound` mixer. Pause clears
outstanding events. The callback is appended **after**
`request_feedback`, capability gated by
`REVERIE_NATIVE_HOST_V1_SOUND_MIN_SIZE`; older hosts continue
receiving legacy `WEAPON_FIRE` fallback from modules that call
`ReverieNativeRequestSoundV1`. It does not change the mandatory
module API descriptor, callback positions or the ABI version.
These five first-party synthesized fallback reports are **not**
downloaded CC0 samples; the P0 recorded sound intake is still open.

### Shell feedback cues

`request_feedback(flags)`

This append-only host tail accepts a bitmask of implemented shell cue requests:

- `REVERIE_NATIVE_FEEDBACK_FOCUS`;
- `REVERIE_NATIVE_FEEDBACK_ACTIVATION`;
- `REVERIE_NATIVE_FEEDBACK_FAILURE`;
- `REVERIE_NATIVE_FEEDBACK_WEAPON_FIRE`;
- `REVERIE_NATIVE_FEEDBACK_RELOAD_START`;
- `REVERIE_NATIVE_FEEDBACK_RELOAD_READY`;
- `REVERIE_NATIVE_FEEDBACK_ARTILLERY`;
- `REVERIE_NATIVE_FEEDBACK_AIRSTRIKE`.

Use `ReverieNativeHostSupportsFeedbackV1` / `ReverieNativeRequestFeedbackV1` so an older v1 host that ends at the save-service prefix remains safe.

The host coalesces flags during each native update and returns them through the existing JNI path. Android maps the original three flags to the same `UiFeedback` cues, and the weapon/reload and artillery/airstrike flags to distinct procedural static-AudioTrack PCM16 one-shots (22,050 Hz mono). The shell generates samples only at initialization, stops playback on Activity pause, and releases tracks on close. Device latency, audible mix, and playback remain hardware validation gates.

This is **not** an arbitrary audio submission/mixing API and does **not** provide haptics. Modules must not describe either capability as implemented through this service.

Slot names are simple filenames. The host owns directory isolation and path validation.

A single write is bounded by `REVERIE_NATIVE_SAVE_MAX_BYTES`.

Save data remains the module's responsibility: use an explicit schema/version, validate length/content before accepting it, and fail safely to a known state when data is missing or corrupt.

## Input contract

`ReverieNativeInputV1` currently supplies:

- `delta_seconds`;
- `move_x`;
- `move_y`;
- `primary_down`;
- `secondary_down`;
- pointer kind;
- pointer origin;
- pointer direction.

The JNI host applies the shared `ReverieNativeSanitizeBaseInputV1` helper before invoking a module: non-finite (NaN/infinite) frame delta and movement axes become neutral **zero**, finite frame delta is clamped to **0–0.1 s**, movement axes to **−1..1**, and button flags to **0/1**. Malformed floating-point input must never become unintended full-strength movement. A game may sanitize a writable input copy again when it needs to defend against another ABI-compatible host. This does not remap controls or grant shell locomotion.

### Pointer kinds

- `REVERIE_NATIVE_POINTER_NONE`
- `REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER`
- `REVERIE_NATIVE_POINTER_VIRTUAL_CONTROLLER`

The pointer is a host-calibrated world-space ray. Modules should use it directly for interaction and must not reconstruct controller orientation from raw Android or transport-specific data.

The JNI host calls the shared `ReverieNativeSanitizePointerV1` helper: finite rays are normalized with an overflow-safe scaled calculation; unknown kinds, non-finite values, near-zero directions, or origins beyond ±1000 m per axis become `REVERIE_NATIVE_POINTER_NONE` with cleared ray data. Native games may reapply this helper to a writable input copy before hit testing. The ABI layout is unchanged.

## Eye contract

`ReverieNativeEyeV1` supplies:

- structure size;
- eye index;
- 4x4 view matrix;
- 4x4 projection matrix.

The shell/Cardboard path owns head tracking, optical projection, eye presentation, viewport/scissor isolation, and current comfort calibration.

A module renders the supplied eye. It does not derive its own Daydream/Cardboard headset model.

Before dispatching `render_eye`, the JNI host uses `ReverieNativeEyeRenderableV1` to require eye index **0 or 1**, the minimum eye prefix, and finite values in all 16 view plus 16 projection floats. Invalid eye data is rejected before guest GLES calls. Both reference games apply the same guard for alternate compatible hosts. This is a safety/compatibility check, **not** an optical-calibration or matrix-invertibility guarantee; the shell does not alter the supplied matrices.

## Lifecycle

### create

`create(const ReverieNativeHostV1 *host)`

Allocate non-GL module state, validate host ABI/size, keep only services you are allowed to call, and return an opaque instance.

Do not assume a GL context exists here.

### destroy

`destroy(instance)`

Release non-GL state. Destruction must remain safe even after a partial start or failed GL setup.

### on_gl_context_created

Create shaders, textures, buffers, and other GL resources. Procedural materials and immutable generated geometry should normally be built here.

Return zero on failure so the host can fail closed.

### release_gl_context

Delete GL resources while the context is valid. Do not make ordinary object destruction depend on GL calls occurring on an arbitrary teardown thread.

### resume

Resume module-level activity after host lifecycle resume.

### pause

Stop or quiesce module-level activity before lifecycle pause, shell transition, or stop.

### update

Advance simulation from normalized input.

This is the correct place for timers, deterministic animation state, input edge detection, AI, economic simulation, and game-state mutation.

### render_eye

Render one eye using the supplied matrices.

Rendering must not advance persistent simulation. The left and right eye must observe the same logical frame.

## Stereo invariants

The host may call `render_eye` twice per logical frame. Therefore modules must not:

- increment timers during `render_eye`;
- consume one-shot input during `render_eye`;
- advance RNG during `render_eye`;
- mutate AI/economy state during `render_eye`;
- generate different procedural content per eye.

Any shared flicker, animation phase, target state, or material choice should be decided during `update` or from stable state before either eye renders.

## GL-state contract

A module is a guest inside the shell's GL context.

It may bind and configure the resources it needs, but it must restore host-visible state that it changes. At minimum inspect/restore the state actually touched by the module, such as program, buffers, textures, enabled attributes, depth/blend/cull state, and related bindings.

Do not assume that the module owns the viewport, scissor rectangle, framebuffer, or the entire render pass.

## Locomotion contract

ReverieVR now has shell-owned bounded view-relative movement for native 3D scenes. That camera translation is applied consistently to both eyes and the controller/pointer frame.

ABI v1 movement axes remain available for game logic and compatibility, but a module must not apply a second copy of shell locomotion to its own eye matrices.

Game-specific movement mechanics that are not the shared player translation—vehicles, mounted tools, levers, cursors, menu navigation—may still consume appropriate normalized actions.

## Trust and packaging

The host intentionally loads only known packaged native libraries.

A valid module must:

- be part of the APK build;
- have an allowlisted module id/library pairing;
- export the v1 entry symbol;
- pass descriptor/API validation;
- satisfy the host graphics requirement;
- fail closed when required callbacks/resources are unavailable.

Do not design game distribution around downloading executable native modules into app-writable storage under ABI v1.

## Optional module capabilities

ABI v1 now uses its first optional API-tail extension for module-declared platform capabilities.

A module may append a pointer to `ReverieNativeModuleCapabilitiesV1` at the tail of `ReverieNativeModuleApiV1`. An older v1 binary whose `struct_size` ends at `render_eye` remains valid; the host must not read `api.capabilities` until `struct_size >= REVERIE_NATIVE_MODULE_API_V1_CAPABILITIES_MIN_SIZE`.

The current capability flag is:

- `REVERIE_NATIVE_CAPABILITY_SHELL_LOCOMOTION` — requests shell-owned bounded horizontal touchpad locomotion.

When that flag is present, the capability block must include positive finite `locomotion_limit_x` and `locomotion_limit_z` values. The host validates an upper safety bound before exposing the envelope to the Java runtime.

Capabilities describe reusable platform behavior. They are not a route for embedding arbitrary game rules in the ABI.

A module with no capability tail or a null capability pointer continues to run; it simply receives none of the optional shell behaviors represented there.

## API evolution policy

The default rule is **append before replace**.

Small optional services should be added as tail extensions when old v1 modules remain safe. Cross-cutting capability that cannot be added compatibly should become ABI v2.

Before adding a host service, prove that at least one real module needs it and that it belongs to the platform rather than one game's private logic.

Likely future candidates include shell-owned audio submission, haptic requests where hardware supports them, comfort/profile queries, richer action state, and shared generated-asset helpers. These are candidates only; they are not part of ABI v1 until implemented in the authoritative header and host.

## Classic Breakout game module (2026-10-15)

The `classic-breakout` native module uses the *unchanged* ABI v1 API and
the same allowlisted, packaged-library mechanism as the six original
classics. Its C++ gameplay state is isolated in `classic_breakout.h`; the
existing `classic_games_module.cpp` implements the colored-mesh rendering
and action handling under `CLASSIC_GAME_KIND=6`. The existing native games
shelf discovers it from the allowlist, rather than a separately installed
archive. Native rules tests, signed APK asset/exports check and Daydream
headset testing remain independent acceptance steps.

## Full-sized Go rendering and gameplay scope (2026-10-15)

The allowlisted `classic-go` module now presents a 19×19 board and uses
361-point state, capture/liberty calculation, exact positional-superko
history and 7.5-komi area scoring. No ABI-v1 structures, callbacks or
shell/game authority boundaries changed. Standard chess/checkers retain
8×8 geometry. Two passes now expose a game-private manual
dead-group review using the existing primary/secondary input: select a
group to toggle its dead status, SCORE to accept the adjusted area count
or RESUME to continue playing. The shell-host ABI is unchanged. Autonomous
CPU adjudication of disputed groups, seki recognition and strong Go
search remain outstanding; see `docs/project/CLASSIC_NATIVE_GAMES.md`.

## English draughts / Checkers draw claims (2026-10-15)

The native `classic-checkers` engine now tracks side-aware exact board
repetition and 40 moves per player without a capture or uncrowned-man
advance. Valid human-turn claims expose the same temporary on-board
CLAIM DRAW pointer target as Chess, without changing any shared ABI
callback, secondary action, shell input routing or Home. WCDF draw
agreements/arbitration and higher-depth search remain future work.

## Chess native game rules and controller extension (2026-10-15)

The classic Chess engine now supports alternate promotion pieces, dead
position detection, exact repetition history, 50-move and threefold
claimability, plus automatic 75-move/fivefold termination using FIDE's
2023 Laws of Chess Article 9. Source-level claims are scoped to these
rules; the one-ply AI is not presented as competitive. The existing
secondary native-game input cycles promotion after selecting a
seventh-rank pawn; the existing primary pointer action on the
conditional on-board CLAIM DRAW target adjudicates a legitimate
claim. No ABI, host input binding or shell quick-menu control changed.

## Classic board games (2026-10-11)

The allowlisted ABI-v1 host now recognizes `classic-checkers`,
`classic-go` and `classic-chess`. All use unchanged descriptor, host
callbacks and normalized input, not a new ABI. The rules/rendering
implementation is game-private; see `docs/project/CLASSIC_NATIVE_GAMES.md`.
