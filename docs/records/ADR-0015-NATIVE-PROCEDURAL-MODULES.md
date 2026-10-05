# ADR-0015 — Native Procedural Game Modules

**Status:** accepted for implementation  
**Date:** 2026-10-05  
**Scope:** RV-0400, RV-0401, RV-0402, RV-0600

## Decision

ReverieVR will support first-party native game modules through a small, versioned C ABI owned by ReverieVR.

The native-module path is distinct from the hosted DOS/libretro path:

- DOS modules remain content packages executed by DOSBox Pure.
- Native modules are trusted ARM binaries packaged with the ReverieVR APK and loaded only from a compile-time allowlist.
- Downloaded/user-imported native machine code is out of scope. Future user-installable native-game content should prefer trusted runtime code plus data packages.

The first native module will be a deliberately small **Procedural Test Chamber** used to prove the host contract before larger games are attempted.

## Werkkzeug3 / .kkrieger source policy

The repository at `jaromil/kkrieger-werkkzeug3` is a source donor and authoring reference, not a wholesale Android port target.

Pinned reference revision:

`72f7697c8b5be6fadae41f9ca6312cd5f88fdc4c`

Use:

- `ktg/` OpenKTG first, because it is isolated, portable C++ and explicitly public domain;
- reusable math/procedural/mesh/scene/game concepts from `werkkzeug3_kkrieger/` only when they are separately audited for platform dependencies and BSD-2-Clause provenance;
- `v2/` only in a later audio slice, with its public-domain notice preserved.

Do not import:

- Win32 application/bootstrap code;
- Direct3D 9 or DirectSound backends;
- Windows message/input code;
- x86/MMX/SSE inline assembly without a portable replacement;
- kkrunchy/YASM as a runtime requirement;
- original .kkrieger branding or game data as ReverieVR content unless separately authorized.

## Native module ABI v1

A native module receives services from ReverieVR rather than owning the platform.

Required lifecycle:

1. enumerate/describe;
2. create;
3. GL-context creation/recreation;
4. resume;
5. update;
6. render one eye using host-supplied view/projection matrices;
7. pause;
8. destroy.

The host owns:

- Android Activity/lifecycle;
- Cardboard head tracking, stereo projection and lens distortion;
- EGL/OpenGL ES context;
- shell-global HUD and recovery;
- input collection and host-reserved controls;
- logging;
- storage roots;
- eventual shared audio/settings/performance services.

A module must not create its own Android window, Activity, EGL context or global recovery route.

ABI v1 uses plain fixed-width C types and explicit structure-size/version fields. No C++ classes cross the module boundary.

## Rendering rule

The proof module renders directly into ReverieVR's active per-eye OpenGL ES context on the Cardboard render thread.

The host supplies:

- eye index;
- monotonic frame/update timing;
- view matrix;
- projection matrix;
- normalized module input snapshot.

The module must restore any GL state that would otherwise corrupt shell-global HUD rendering.

## Input rule

The v1 snapshot exposes a narrow platform-neutral set sufficient for the proof module:

- movement X/Y;
- primary action;
- secondary action when available.

Head orientation is already represented by the host-supplied view matrix. Host Back/Home/recovery remain reserved to ReverieVR and are not consumable by the module.

## First imported component: OpenKTG

Vendor only the audited OpenKTG implementation needed by the proof module:

- `ktg/gentexture.cpp`
- `ktg/gentexture.hpp`
- `ktg/types.hpp`
- `ktg/LICENSE`

Preserve the upstream public-domain notice and record the pinned source revision.

The first module uses OpenKTG at module creation to procedurally generate at least one texture from recipe/parameters rather than shipping that texture as a bitmap.

## Concentrated execution plan

### NM-1 — Architecture and provenance

Deliver:

- this ADR;
- backlog and acceptance-ledger targets;
- pinned OpenKTG provenance.

Gate:

- scope clearly rejects a wholesale Windows engine port and arbitrary downloaded native code.

### NM-2 — Build independence

Deliver:

- NDK build runs for native modules even when DOSBox Pure has not been fetched;
- DOSBox Pure inclusion becomes optional inside the NDK build rather than the condition for having any native code;
- both `armeabi-v7a` and `arm64-v8a` remain target ABIs.

Gate:

- static build configuration contains no hard dependency from native-module compilation to DOSBox Pure presence.

### NM-3 — ABI and loader

Deliver:

- `reverie_native_module.h` ABI v1;
- native host JNI wrapper;
- compile-time allowlist;
- `dlopen`/symbol validation for packaged native modules;
- capability/version/structure-size rejection with explicit errors.

Gate:

- an unrecognized module id or invalid ABI fails closed and returns a diagnostic rather than crashing or claiming success.

### NM-4 — Procedural Test Chamber

Deliver:

- separate packaged shared library for the test module;
- OpenKTG-generated texture at initialization;
- simple low-complexity chamber geometry;
- stereo per-eye rendering from host matrices;
- movement input;
- deterministic lifecycle logging;
- no Android/Java/Window ownership inside the module.

Gate:

- source path is independent of DOSBox Pure and module rendering cannot consume the shell Back recovery action.

### NM-5 — Stage B integration

Deliver:

- Native Modules entry on VR Home;
- launcher for the built-in test module;
- pause/resume with Activity lifecycle;
- clean return to Home;
- shell-global power/status HUD still renders over the module.

Gate:

- module mode is a shell-owned state, not a new Activity.

### NM-6 — Validation stop

Run every deterministic source/build check available in the current environment.

Do not claim device acceptance until the Galaxy S9 proves:

- both eyes render correctly;
- head tracking/projection are correct;
- movement/input are usable;
- Back exits safely;
- pause/resume survives;
- HUD survives;
- sustained frame pacing/thermal behavior are acceptable.

## Explicitly deferred

Until the proof module passes reference-device validation:

- porting Kkrieger gameplay;
- porting Werkkzeug3 mesh/operator systems;
- V2 synthesis;
- downloadable native-code modules;
- a new Android editor;
- arbitrary external plugin ABI compatibility;
- module networking/store/distribution.

## Removal path

If OpenKTG proves unsuitable, the native ABI remains valid. OpenKTG is a module implementation dependency, not a platform dependency, and can be removed/replaced without changing shell lifecycle or module loading semantics.
