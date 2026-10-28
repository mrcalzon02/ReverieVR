# ReverieVR — complete desktop platform expansion

**Decision date:** 2026-10-26. **Status:** product-wide target accepted; Java17 desktop scene-authoring workbench source integrated October 28, 2026; playable desktop hub and OpenXR still unimplemented/unvalidated.  
**Authority:** `docs/project/INSTRUCTIONS.md`; subject to `docs/project/EXECUTION_CONTRACT.md`.  
**Scope:** **the entire ReverieVR application and its hosted library**. This is not a Sitka-specific port, a desktop-only game, an Android emulator, a remote-control utility, or a replacement for the Daydream APK.

## Product requirement

ReverieVR shall be one **cross-platform offline-first VR-and-desktop hub** offering the same first-party Home rooms, avatar identity and wardrobe, library discovery/launch, native games, classic games, DOS/emulator surfaces where legally and technically available, local media, interaction/Quick Menu, settings, saving, audio, diagnostics, and coherent **whole-application release/update** flows. All supported desktop systems must boot, navigate, launch, play and recover **without a headset, Android phone, laptop companion, Steam installation, internet connection or SteamVR**. Network access is needed only for user-initiated update/download features.

| OS / architecture | Required primary distribution | Primary interaction | XR posture |
| --- | --- | --- | --- |
| Android (existing supported ABIs) | ReverieVR APK; one complete phone-resident app | Daydream/3DoF headset and controller with shell gaze recovery; touchscreen Stage A | Existing reference path, independently operational |
| Windows x86_64 | Installable native desktop application | Keyboard, mouse, gamepad, optional VR controls | Provisional OpenXR, SteamVR as supported runtime |
| Linux x86_64 | Native desktop application/package | Keyboard, mouse, gamepad, optional VR controls | Provisional OpenXR, SteamVR or compatible supported runtime |
| macOS Apple Silicon | Native .app and distribution artifact; Intel support evaluated and labeled separately | Keyboard, mouse, gamepad | Complete non-VR desktop required; no promised SteamVR, which ended macOS support |

Do not claim a platform is released until actual installation, hub operation and representative modules are verified on that OS. A single build must never be advertised as supporting platforms it does not ship for.

## Full-hub parity contract

| Subsystem | Cross-platform obligation | OS-specific boundary |
| --- | --- | --- |
| ReverieVR Home | Same selectable rooms, atmosphere, mirrors, wardrobe/avatar profiles, accessible hub navigation and shell Quick Menu | Desktop free look/orbit and optional first-/third-person view; VR uses tracked view |
| Native Games | Launch every **currently supported/installed** module from the same library with real gameplay, lifecycle and exits; apply per-module portability admission rather than pretend a missing port works | Native binaries recompiled for each CPU/OS; no Android .so on desktop |
| Classics and DOS | Real input/game rendering, content rights, state and orderly exit on desktop; full catalog parity is an explicit acceptance item | Port/rebuild emulator integrations and OS-specific filesystem/graphics, not APK reuse |
| Media/music | Offline local files, audio playback, virtual cinema/360 displays with a desktop viewing mode; no phone-dependent file picker | Desktop native file chooser and video/audio backend; licensed codecs/content only |
| Controllers/UI | Common semantic actions, rebinding, reticle, accessible menus, virtual keyboard for VR and physical keyboard on PC | Daydream/BLE and Android side buttons remain Android-only; desktop controllers use desktop HID/gamepad |
| Bundled game catalog / updater | Keep the same module IDs, save/ABI contracts, bundled game library and whole-application update integrity | One cohesive platform-specific installed application; no separate native-game installer or independently updated game archives |
| Avatar identity | Same selectable profiles/body classes and collision conventions across hub and compatible games | Platform-specific rendering/performance profiles |
| Saves/settings | Versioned portable state schema and compatible module saves; explicit import/export/migration | OS app-data paths, separately scoped graphics/UI settings; **no assumed cloud sync** |
| Status, diagnostics, recovery | Honest capability and device state, performance monitor, repair/reset, shell-owned navigation | Phone/controller battery only when present; desktop power/HMD telemetry only when real; no fake HUD values |
| Phone-only features | Do not block the rest of the hub or expose inert buttons | Explicit capability alternatives or suppression: Android Phone Screen and MediaProjection are not silently available on PC |

A game or capability can be labeled pending on a new OS during rollout, but the **finished product** cannot be called full desktop ReverieVR until the full supported module catalog has real launch and interaction proof. Temporary partial milestones must be named as such, not marketed as complete ports. Where native proprietary/third-party emulator licensing blocks redistribution, document that exception and provide lawful user import where permitted.

## Portable architecture: shell-first, shared core

Current authoritative source is Android-first: Android Gradle shell, Java lifecycle/input/services, GLES2-centric native game ABI `app/src/main/jni/native/reverie_native_module.h`, Android-specific loaders/assets, and APK-based module delivery. **No native Windows/Linux/macOS ReverieVR launcher is yet verified.** Merely cross-compiling a game or creating a desktop window does not achieve this requirement.

1. **Shared host core:** factor shell scene/state/navigation, module registry/launch lifecycle, app/service interfaces, game-facing input actions, save/version contracts, shared avatar identities, media and diagnostics policy into portable interfaces. Preserve Android's complete independent phone runtime.
2. **Platform shell hosts:** keep Android Java/JNI/Cardboard adapter; add actual native Windows, Linux and macOS entrypoints owning windowing, event pump, file pickers, storage, audio devices, HID/gamepad, desktop monitor modes, OS lifecycle and install/update paths. Choose/pin small reviewed platform dependencies only after real prototype and license review.
3. **Rendering abstraction:** extract platform-neutral scene/mesh/material and frame data from GLES2-specific calls; use a tested desktop graphics backend suitable for all three OSs (with macOS Metal path or a demonstrated compatible translation). Preserve the original Android low-poly GPU path and 30 FPS thermal budget. Desktop 2D must render one camera, optional OpenXR stereo must render two views from **one** simulation update.
4. **Host-native modules:** retain versioned game ABI semantics but inspect actual GL assumptions, dynamic library names, symbol calling conventions, ownership, threads, context lifetime and available host services before changing headers. Only append compatible ABI features with synchronized SDK docs/tests, or introduce a documented v2 adapter; never break Android v1 modules by accident.
5. **Classic/emulator/media plugins:** audit content and dependencies one by one; provide actual desktop backends instead of mirroring an Android screen or omitting the feature silently.
6. **Portable content pipeline:** use authored common assets, offline builds and shared editor tools, with per-platform shader/texture/audio packaging and source/license attribution. The deliverable is **one unified app and bundled games library per supported OS**, with a versioned platform/architecture-aware **whole-application** update manifest; do not resurrect independently downloadable game packages.
7. **Input/UX:** desktop keyboard/mouse default, gamepad alternative, physical screen menus and usable VR-room flat navigation. XR headset detection never hijacks desktop input or activates gaze while a live controller is bound. Preserve reserved Quick Menu, Home and safe exit.
8. **Privacy/recovery:** no new screen OCR/analysis or unconsented pixel export; explicit consent for capture/network devices; local-first data. A broken VR runtime, missing headset, controller disconnect, absent module, or update failure must leave the desktop hub usable.

## SteamVR and the existing phone-as-headset project

**Desktop ReverieVR VR mode:** optional OpenXR API adapter inside the native Windows/Linux application. Discover the system's **active** OpenXR runtime, obtain tracked headset/controller input and submit stereo swapchains to it. SteamVR can be the active runtime, but it is not required to launch the desktop app, and Steamworks/store integration is not required. Validate Windows first; Linux headset/driver compatibility is hardware-dependent. Do not promise SteamVR on macOS. VR frame timing must follow the **headset's actual refresh rate**, not the phone's 30 FPS target.

**RV-0246 SteamVR Laptop Link:** separate existing project to allow a Galaxy S9 to function as a streamed PC SteamVR headset for third-party PC VR software. Its older document says **not** to port the ReverieVR hub to PC because it scoped only that peripheral project. That exclusion is superseded **only as a global product constraint** by this newly authorized whole-platform expansion; the RV-0246 peripheral project, its security restrictions and its unverified status remain intact. Keep the two integrations separate; neither must depend on the other.

## Current desktop development increment — 2026-10-28

`tools/desktop/` is a **source-integrated Java 17 Swing workbench** that compiles against the *actual canonical* Java game registry, Home environments and bounded OBJ parser. It offers a practical, headset-free 3D wireframe authoring viewport (orbit, zoom, mouse selection), primitive scene placement, repository-local OBJ import, per-object transforms, undo/redo and atomic saves of versioned `REVSCENE 1` documents. The Windows `run.cmd` and Linux/macOS `run.sh` build/run it from a checkout with a JDK 17+; `test.sh` runs the scene codec tests.

**Validation boundary:** source compilation and persistence tests passed locally with signature-compatible stand-ins for those three existing Java types; an Xvfb desktop smoke kept the GUI process running. A full source-tree build on actual Windows/Linux/macOS machines, native-game execution, actual Android GLES Home visual parity, consuming authored scenes inside native games, installers, and OpenXR/SteamVR are **not** yet proved. The workbench does **not** expose inert game launch controls.

**Source-integrated 2026-10-28 bridge:** the independent `REVSCENE 1` Java editor and portable C++ scene decoder now agree on a tested bounded exchange format. `reverie_scene_document.h` produces GLES2-ready interleaved primitive vertices, and Test Chamber loads the exact allowlisted `native/test-chamber/scene.rvscene` bundled with the full APK, appending editor-authored geometry to its existing GL VBO. The editor's real **Stage Test Chamber for APK** button writes the resource after confirmation, refusing unsupported OBJ meshes. `scripts/verify-desktop-scene-bridge.sh` tests Java serialization against native parsing and geometry plus host resource boundaries. A staging action requires rebuilding/reinstalling the APK; it is not an on-device live-edit link.

The bridge has passed local source-level C++17 compile/tests and Java17 tests with compatible Java catalog/OBJ-parser stand-ins. Actual full-repository Java compile, Android NDK/GL integration, headset visual testing, Windows/Linux/macOS desktop runtime play and SteamVR are **not yet accepted**. See [native contract](../native/SCENE_BRIDGE.md). The next integration is live editor-to-desktop-native gameplay rendering and generalizing the asset consumer safely across hosted games.

## Delivery roadmap and non-negotiable gates

**D0 — Inventory/architecture:** enumerate the Android shell components, every registered native/classic module, media services, static assets, Android-only dependencies and external licenses. Capture actual parity matrix, ABI and render/resource ownership. No claims of desktop support.

**D1 — Portable desktop foundation:** reproducible native Windows/Linux/macOS build and install; common shell scene shows real Home content, correct readable text, input, music/ambience and Quick Menu. Desktop mode has no HMD/runtime dependency; desktop application does not run an Android VM/emulator.

**D2 — Hub feature parity:** rooms, avatars/wardrobe/mirror, library discovery, true module installation/update, physical keyboard/gamepad mapping, settings/diagnostics and saved profiles. Test every control as a functional action; no placeholder buttons.

**D3 — Game-module parity:** port/verify native module ABI, registered native games and classics, including authentic input/gameplay and return to Home; game-specific working save/load and corruption isolation. Port DOS runtime only on proven license/architecture basis. Maintain an explicit matrix, not a count of compiled objects.

**D4 — Media and services:** audio, cinema, 360° media in flat mode, legal codec/content playback, files/import/export, persistence/version migration, error recovery.

**D5 — Optional PC XR:** Windows OpenXR runtime entry/exit/stereo/action bindings/comfort/Quick Menu; real SteamVR headset validation; Linux runtime/HMD test matrix; graceful return to window on XR failure.

**D6 — Unified platform packaging and updates:** one whole-app release artifact per platform/arch with bundled supported games; checksums/signing/provenance, distribution and install/uninstall/update/rollback/restart scenarios on every target OS. Retain the unified Android APK updater.

**D7 — Acceptance:** cold start, Home, avatar selection, native games, classics, DOS, media, game exit, controller disconnect, module acquisition/update, offline save recovery and clean OS exit on each OS; sustained Android S9 30 FPS/thermals, measured desktop flat frame pacing and PC VR compositor timing at HMD-native cadence. Release labels distinguish partial prototype, desktop beta and full library parity.

Only run functional diagnostics/build/launch/update tests as required by project policy. Cosmetic screenshot/style comparison tests are not blockers without explicit request. Never equate CI compilation with validated actual OS runtime, and do not run GitHub Actions automatically without user authorization.

## Open decisions to validate before choosing implementation libraries

- Desktop window/input/audio stack and renderer API, with a real macOS metal-capable path and OpenXR graphics binding feasibility.
- How existing Android-hosted media and classic cores become portable (binary rights/SDK limitations must be recorded).
- Exact native-module ABI evolution strategy and desktop dynamic-loading trust/versioning.
- Desktop packaging/signing and release distribution, especially macOS notarization where applicable.

The chosen libraries, versions, licenses and SDKs are implementation decisions **not yet approved by this specification**.