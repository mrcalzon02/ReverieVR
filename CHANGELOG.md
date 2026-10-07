# Changelog

- Adopted ADR-0015 for a focused native procedural game-module path: a versioned ReverieVR C ABI, packaged trusted modules, OpenKTG-first procedural content, a built-in Procedural Test Chamber, and an explicit rejection of wholesale Win32/.kkrieger runtime porting before the proof module passes device gates.

All entries describe verified project/repository changes. Planned work belongs in the backlog, not here.

## Unreleased

- Hardened phone-test release publication against transient GitHub API 5xx errors: bounded retries, recovery of partially created tags, and mandatory read-back verification of commit target and all three nonempty release assets. The prior #75 APKs passed build/signature checks but publication failed with GitHub HTTP 500; this workflow change triggers a fresh run.

### Headset rig foundation

- Added an allocation-free shell-owned center-head player rig with calibrated eye-origin markers, headset wireframe geometry for a future mirror/third-person pass, and an oriented-box collision probe. The tracked/virtual controller proxy now reports headset overlap transitions in Development diagnostics without interfering with live tracking, controller motion, Cardboard optics, or stereo. Unit tests cover head translation/yaw, eye separation, collision, geometry and invalid pose handling. No mirror renderer, scene collision response or physical S9 proof is claimed.

- Corrected the superseded ADR-0022 recenter note to reflect the already shipped ghost/spring/sound implementation rather than leaving known-stale project guidance.

### Performance diagnostics

- Removed per-eye viewport/scissor scratch-array allocations from Cardboard stereo rendering and reused its viewport query for eye diagnostics. Added bounded, allocation-free-on-record Development left/right CPU submission avg/p95/p99/max with mode-isolated windows; log once per minute alongside battery percentage and existing frame/thermal metrics. JVM tests cover percentiles, invalid samples, mode transitions and bounded retention. Actual GPU/compositor and S9 power/thermal gains remain unmeasured.

### Native games

- Hardened native GLES descriptor admission to reject invalid zero-major requirements and hosts; extended host C++ ABI regression coverage and synchronized API/SDK reference guidance. The prepared commit was subsequently integrated into `main`; full repository ABI/doc checks, Android/NDK packaging and Galaxy S9 validation remain pending.

- Reconciled the native SDK acceptance record with shared base-input sanitization and stereo-eye validation, and strengthened the documentation guard against future GLES/acceptance drift. This is source/documentation hardening, not device acceptance.

- Centralized overflow-safe finite pointer-ray validation in `ReverieNativeSanitizePointerV1` and routed JNI host and Red Ledger through it. Invalid pointer kinds, NaN/infinity, out-of-range origins, and degenerate directions now fail closed; huge finite rays normalize without squared-length overflow. Extended the host-buildable ABI regression gate and living native handbook/drift guard. Android/NDK and Galaxy S9 validation remain pending.


- Collapsed duplicate Test Chamber/Red Ledger hosted input profiles into one `native-standard` native-game profile: Select maps to virtual button 0 and the two reclaimed former volume-key inputs map to game buttons 1/2. `VrActivity` no longer chooses a profile by module id; optional movement remains separately module-declared through the locomotion capability.

- Promoted two more repeated correctness utilities: `ReverieNativeLogV1` now centralizes safe host logging/fallbacks, and `ReverieNativeBuildProgram` plus `ReverieNativeGlAttributeBindingV1` centralize the common GLES compile/bind/link/cleanup lifecycle. Test Chamber and Red Ledger keep their own tags, shader text, bindings, uniform policy and diagnostics but no longer duplicate host-log dispatch or program-link boilerplate.

- Added `docs/native/SYSTEMS_CATALOG.md`, a living maturity/discovery index for the native environment: ABI and host services, module capabilities, input/recovery/locomotion, rendering helpers, generated materials/geometry, shell-only generation references, validation methods, game-private techniques, and planned systems. The native drift guard now requires every standardized contract-map source/document to remain discoverable there and pins selected shell references to their actual generated dimensions.

- Added an append-only native host feedback service with `REVERIE_NATIVE_FEEDBACK_FOCUS`, `ACTIVATION`, and `FAILURE` flags. Requests are sanitized/coalesced in the native session, drained through the existing JNI update call, and mapped to the shell's implemented `UiFeedback` audio cues. Test Chamber and Red Ledger now exercise the service. This does not claim arbitrary audio mixing or haptics.

- Added a host-buildable executable ABI contract gate (`native_abi_contract_test.cpp` via `verify-native-abi-contract.py`) covering old-v1 mandatory prefixes, optional capability tails, frozen descriptor layout, host/input/eye capability checks, shell-locomotion gating, and OpenGL ES version admission. The host now compares both required GLES major and minor through the shared `ReverieNativeGlesRequirementSupportedV1` helper instead of checking only the major version.

- Added the first compatible optional ABI-v1 module capability tail: `ReverieNativeModuleCapabilitiesV1` with `REVERIE_NATIVE_CAPABILITY_SHELL_LOCOMOTION`. Test Chamber and Red Ledger now declare their travel envelopes in their module API; JNI validates/caches the capability once at launch and the renderer consumes the cached bounds instead of hard-coded module-id branches. Older v1 modules whose API ends at `render_eye` remain valid and simply expose no optional capability block.

- Tightened ABI-v1 evolution rules around the embedded module descriptor: `ReverieNativeModuleDescriptorV1` is now explicitly layout-frozen and validated at its exact v1 size because growing it would shift the surrounding API callback table. Optional future v1 metadata must live at the tail of `ReverieNativeModuleApiV1`, otherwise the change requires ABI v2.

- Promoted two independently duplicated native-game utilities into focused shared SDK headers: `ReverieNativeMat4Multiply` in `reverie_native_math.h` and `ReverieNativeCompileShader` in `reverie_native_gl_utils.h`. Test Chamber and Red Ledger now consume the common implementations; `CORE_UTILITIES.md` and the drift guard explicitly prevent these helpers from drifting back into game-private copies.

- Added shared `reverie_native_gl_state.h` plus `GL_RENDERING_STANDARD.md` and migrated both native games to it. The guard restores program/buffer/texture/enabled-state plus complete modified vertex-attribute configuration and generic values, closing the prior leak where modules restored only attribute enable flags after overwriting pointer/buffer/current-value state.

- Added the first shared `reverie_native_sdk.h` helper layer and `docs/native/SDK_HELPERS.md`: host logging/save capability checks, descriptor/API mandatory-prefix checks, base/pointer input checks and eye-matrix checks are now centralized. The native host, Procedural Test Chamber and Red Ledger consume the shared helpers instead of duplicating ABI size/version logic, and the drift guard treats the helper header/document pair as a strict synchronized contract.

- Repaired the ABI v1 append-only compatibility contract: the public header now exposes minimum-prefix size constants for host logging/save services, descriptors, base/pointer input, eye data and the mandatory module API table. The host, Procedural Test Chamber and Red Ledger validate only the prefix they consume instead of the newest total struct `sizeof`, and the native documentation drift guard rejects regression to full-structure size checks.

- Made the native developer documentation an active implementation contract: added `docs/native/CONTRACT_INDEX.md`, a same-change source-to-document synchronization rule, and dependency-free `scripts/verify-native-doc-sync.py` checks for ABI version/symbol/save limits, pointer/services/lifecycle names, module-id policy, procedural atlas dimensions/format utilities, and static-geometry counts/footprint. Project doctrine, execution rules, AGENTS, backlog and acceptance evidence now treat known-stale native documentation as a defect rather than deferred cleanup.
- Established `docs/native/` as the native-game SDK documentation surface: a developer handbook, exact ABI v1 reference, and procedural-content standard covering host/module ownership, lifecycle/input/save/stereo invariants, generated materials, static-geometry batching, model-recipe direction, validation, and promotion of proven game techniques into shared platform helpers. Added RV-0403 to track the path from the current living standard to a stable reusable ReverieVR game API.
- Halved Red Ledger's procedural GPU atlas footprint from 64 KiB RGBA8 to 32 KiB RGB565 using direct packed generation (no intermediate RGBA allocation). Moved animated flicker from per-eye rendering to once-per-update for stereo-consistent lighting. Native tests validate RGB565 quantization against the RGBA reference and deterministic bounds/seed behavior. S9 visual/power gains remain unmeasured.
- Baked 11 immutable Red Ledger room/furniture cubes into a 15,840-byte world-space VBO once per GL context. The shared atlas and shader now render the static room in one draw per eye (formerly 11) using per-vertex color and material coordinates; interactive props remain dynamic. Added deterministic native geometry/buffer/UV tests and Android build verification. Real S9 visual and energy benefit remains unverified.
- Introduced a seeded, deterministic four-material procedural atlas for the Development-only Red Ledger module: stone, wood, metal and paper textures are generated into one 128×128 RGBA8 atlas only on GL-context creation, uploaded once, and shared by both eyes. The module has UV-mapped cube faces, one atlas binding per eye, per-material tile selection without rebinding textures, GL resource teardown/recreation, and strict host-C++ reproducibility/bounds tests. This is an asset-size/CPU-generation design; S9 battery and thermal improvements are not claimed without measurements.
- Reworked **Red Ledger** into a manual seated bar loop: take/return a clean cup, fill it at the tap, hand it to the patron, explicitly collect payment, wash returned cups, order supplies, handle protection, turn patrons away at the exit, and close the ledger. Cash and patron progression now happen only at payment collection. Save schema v2 is 160 bytes and preserves held-drink/pending-payment state. Phone-test #52 passed the strict simulation, full Android/NDK assembly, signing, dual-ABI APK verification and published `phone-test-52-1`.
- Added a bounded **Development-only Red Ledger admission path** for physical headset validation. The trusted host recognizes the packaged module, Standard logging filters it out, Development logging exposes a `[DEV]` Native Apps entry, runtime start separately requires Development, and a dedicated Select-only hosted profile removes the seated game's otherwise inert Test Chamber movement mappings. Phone-test #60 passed the full JVM/Android/JNI/NDK/signing/APK-verification path for this admission/profile hardening and published `phone-test-60-1` together with the current stereo-isolation repair.
- Added a Development-only Red Ledger ray-contact cursor at the actual selected work-target hit point and defensive pointer-direction normalization, making the first physical target/reach calibration pass observable without changing the native ABI.
- Began **Between Deliveries: The Red Ledger VR** as ReverieVR's first full native-game track: added a deterministic one-room bar/economy core, first three pressure events (protection, inspection, supply interruption), day-close accounting, recurring patrons/supplies, a deliberately low-poly GLES2 bar-room module, strict host-C++ smoke coverage, and an NDK shared-library target.
- Extended native ABI v1 append-only with shell-calibrated tracked/virtual-controller pointer rays and bounded module-private save callbacks, then replaced Red Ledger's button-only development harness with ray-targeted tap, wash, ledger, supplier-card and protection-envelope interactions. Red Ledger now uses an explicit versioned save schema with round-trip/corruption tests. Phone-test #50 completed the Red Ledger `-Werror` test, full unsigned and signed Android/NDK builds, dual-ABI APK library checks, artifact staging and `phone-test-50-1` publication before newer #51 superseded the overall workflow under latest-build-wins concurrency. The game remains deliberately outside the Native Apps allowlist pending physical Galaxy S9/Daydream interaction, recovery/recenter, persistence and thermal validation.

### VR presence and headset comfort

- Added a translucent neutral-target duplicate of the actual Daydream controller, a procedural 3D spring and synthesized spring sound to gentle handset recenter. The ghost follows headset-relative positioning and live quaternion, the same coil is used for both eyes, transparent rendering restores depth/blend state, and static PCM playback respects Activity lifecycle. JVM tests cover geometry, sound, moving target and cancellation. Galaxy S9 headset validation remains pending.

- Corrected the remaining Quick Menu handset-recenter action to use the same gentle positional return as the shake gesture (fixing a Java compile failure in phone-test #71). Touchpad activity now clears only a pending shake impulse, preserving the three-second recenter cooldown. Native locomotion gate is renderer-thread-owned, including on BLE disconnect. Added cooldown regression coverage.

- Hardened native touchpad travel with release-to-rearm after scene entry, Quick Menu, click and stale controller packets; an immutable touch snapshot prevents mixed-coordinate frames. Reduced shake recenter false triggers by requiring opposing sharp impulses and ignoring touchpad use; handset now eases toward neutral over three seconds while preserving active inertial movement and aim. Wired live Android battery-change broadcasts into the renderer and avoided redundant battery HUD texture refreshes. Added pure-JVM gate, battery conversion and gesture regression tests. Hardware comfort, shake thresholds and battery drain remain to be measured.

- Implemented bounded headset-view-relative Daydream touchpad travel in native 3D scenes. Touch top/bottom to advance/retreat and left/right to strafe, with analog deadzone, speed/area limits, no coasting, and release/click/menu/stale-input interlocks. Shell owns the camera transform for both eyes and controller ray, replacing duplicate Test Chamber world-axis movement. Red Ledger has a narrow behind-bar envelope; Test Chamber has a larger area. Pure-JVM motion and binding tests added; real S9 comfort and reach remain unverified.

- Reworked the Daydream 3DoF controller placement as a yaw-only torso-side anchor rather than inheriting headset pitch/roll. Added a persistent Left/Right hand control and real Reset Hand Position action on the Controller panel, reduced inertial position travel to a small bounded envelope, and retained independent tracked-quaternion aiming. Torso yaw follows sustained turns with a deadband and smoothing; actual S9 controller-ray direction/comfort remains a hardware gate.

- Added the user-supplied **Starry Cereal** track as looping ambient music for the VR shell/environment. The packaged 267.312-second stereo MP3 is APK-optimized to 48 kbps while preserving the source program, plays at a deliberately subdued menu gain, pauses across Activity lifecycle changes and whenever Media/DOS/Native hosted content owns the experience, then resumes from its prior position when the shell returns.
- Added a shell-owned floating Orientation menu available from the Daydream App/Menu button, Cardboard system menu control, gamepad Start/Mode, and keyboard Menu key.
- Added **Center on headset** and **Center on controller** heading choices, plus Back-one-level and Close actions; the controller option is disabled when no fresh tracked pose exists.
- VR startup now explicitly aligns shell forward to the first headset heading instead of inheriting an arbitrary sensor/world yaw.
- Separated shell heading changes from the Daydream provider's hardware recenter command. Software controller-yaw calibration now follows shell heading changes so rotating the workspace does not drag the virtual controller away from its physical direction.
- The Orientation menu remains available as a modal overlay above Media, DOS, Native, setup, and Home surfaces, and suppresses hosted input while open so menu/select actions do not leak into games.
- Expanded that modal into the first universal Quick Menu slice: Headset Forward, Controller Forward, volume down/up, contextual Back, Home, Exit to Phone, and Close all invoke real shell behavior; Home/Exit stop active media, DOS, or native content through the owning lifecycle before changing modes.
- Added live Quick Menu status for phone/controller battery, elapsed VR session time, Android thermal status, battery-sensor temperature, and rolling frame-time p95; the readout refreshes once per second while the modal is open without turning status text into inert controls.
- Added real per-session window brightness down/up controls to the Quick Menu. They start from the current Android brightness when no ReverieVR override exists, adjust in bounded 10% steps, and affect only the VR Activity rather than requiring global system-write permission.
- Added a nested Quick Settings page that stays inside the shell modal over hosted content. It exposes only wired controls: battery HUD, numeric percentages, look-up reveal, pointer mode, and UI scale, with Back-to-Quick and Close navigation.
- Quick Menu placement now captures the headset's current horizontal pointing direction when summoned, so the recovery/orientation panel appears in front of the user even when the existing shell heading is wrong.
- Recenter and startup heading math now derive yaw from the actual forward vector using the shell's rotation convention, with cardinal-direction unit tests guarding sign/polarity.
- Initial shell heading no longer locks to the first sensor frame: it follows the current headset heading briefly and locks only after a stable-frame window or a bounded settle timeout, reducing arbitrary startup orientation.
- While the Quick Menu is open, physical keyboard arrows drive a dedicated shell-owned two-column focus model, Enter/Space activates, and Escape/Back closes or backs out. These keys are consumed by the modal and cannot fall through into media seek or hosted DOS/native input; normal keyboard delivery resumes when the modal closes.

- Added bounded inertial headset translation from the handset linear-acceleration sensor so small real head/body movements produce controlled parallax instead of leaving the VR viewpoint rotationally pinned; the offset is damped, spring-returned, bounded, and reset on recenter rather than pretending to provide drift-free 6DoF tracking.
- Retained the earlier shell-geometry contraction for panel comfort, then added a separate centered per-eye presentation viewport at 82% of Cardboard's raw eye width/height from physical headset calibration. Each raw eye is cleared to black first, the world/UI render inside the inset without changing projection or IPD math, stereo scissor isolation remains bounded to the SDK-owned eye rectangle, and the power HUD stays on the raw viewport as a stable calibration reference.
- Repositioned the three Home planes farther left/right and tilted the side planes so their outer edges come toward the viewer, creating a shallow wrap-around launcher instead of a flat wall.
- Vendored and integrated the selected low-poly Daydream controller model with preserved upstream attribution/license files; its separated controls respond to button state and the procedural model remains only as a load-failure fallback.
- Unified the visible controller anchor and pointer emitter, including the bounded headset/body offset, so the ray begins at the front of the rendered controller instead of its center.
- Added pure-Java tests for inertial deadzone, bounds, reset, and spring return.
- Phone-test #42 passed the full JVM/Android/NDK/signing/package-verification pipeline and published `phone-test-42-1`; CI confirmed the final APK contains both `CardboardView` and the vendored Daydream controller OBJ.

### UI integrity

- Replaced the canonical ReverieVR branding with the latest user-supplied soft-edged logo revision; the same master now feeds repository/internal branding and the Stage A app header, while both Android apps use a square visor/orbit launcher crop derived from it.
- Adopted the new user-supplied ReverieVR logo as the Stage A setup header and repository README branding, using a lightweight app-ready copy of the supplied artwork.
- Derived a square launcher icon from the user-supplied ReverieVR headset/orbit mark and applied it to both the headset APK and the ReverieVR Controller companion so installed builds carry consistent project branding.
- Phone-test #44 correctly rejected a malformed first launcher-icon payload during AAPT2 resource compilation; both APKs now use the byte-verified 256×256 PNG (`SHA-256 edf2e1d16c991e9f424510609aaf3c1f8f922e9dce883600639f70b52a3a36d3`) generated from that mark.
- Removed the inert **Prefer retro performance-first quality** Quality of Life switch and its orphaned stored-preference wiring. ReverieVR remains performance-first and retro-first by project doctrine; this is a product baseline, not a placebo user toggle.
- Added an explicit project-management rule forbidding user-facing controls that do not reach implemented, observable behavior.
- Added centralized activation and rejection/failure sounds for Stage A buttons, Stage B shell selections, and the controller-phone application's buttons.
- Added explicit pressed, disabled, and temporary failure-flash visual states to the touchscreen button styles while preserving Stage B gaze-hover highlighting.
- Made the controller companion's Start/Stop availability reflect actual server running state so the buttons cannot present an enabled no-op.
- Added project doctrine requiring responsive visual states, normal activation sound, and a distinct failure/rejection sound for actionable controls.

### Phone-test cloud build and prerelease

- Phone-test #56 proved the Cardboard viewport repair by passing the complete unsigned validation tree, then correctly stopped before publication when AAPT2 rejected the newly derived indexed-color launcher PNG in the signed controller build. The launcher crop is now stored as an AAPT-safe true-color JPEG resource for both Android packages instead of weakening the resource gate.
- Phone-test #55 failed at Java compilation because a viewport-correction patch called the non-existent `CardboardView.Eye.getViewport()` API. The repair restores the pinned Cardboard SDK as the owner of per-eye `glViewport` setup and keeps the existing `GL_VIEWPORT` diagnostic readback, removing the invalid API dependency instead of bypassing the stereo verification.
- Corrected the in-app updater's false certificate-mismatch rejection on Android/OEM builds whose archive parser does not expose APK signing metadata reliably: ReverieVR now uses both signing metadata paths, falls back where available, and delegates final replacement-signer enforcement to Android Package Installer when archive signer metadata is unavailable after all other trust checks pass.
- Stopped minifying the standalone Cardboard adapter AAR, which could compile successfully and then strip public Java API classes such as `CardboardView` before packaging; the phone-test release gate now inspects the final APK DEX payload and refuses publication unless `com.google.cardboard.sdk.CardboardView` is actually present.

- Added an explicitly authorized GitHub Actions phone-test workflow with manual dispatch, pinned Android/NDK inputs, third-party verification, tests, dual-APK assembly, APK content/signature checks, SHA-256 output, and GitHub prerelease publication.
- Repaired Android resource apostrophe escaping exposed by the first real AAPT build.
- Completed the controller companion priority/coalesced-send refactor exposed by javac.
- Added the Cardboard protobuf-lite runtime explicitly to the headset app compile classpath.
- Corrected the stripped Cardboard linker configuration so Vulkan/Unity can remain disabled while both reference ABIs link successfully.
- Made ReverieLog inert before initialization so pure-JVM binding tests do not call Android's unmocked Log stub.
- GitHub Actions run #6 passed the complete test/build/verify pipeline and published prerelease `phone-test-6-1` with headset APK, controller APK and SHA-256 checksums.
- Phone-test run #8 correctly rejected the first DOS pause-gate integration because two AudioTrack paths still referenced the removed single `paused` flag; those stale references were repaired at the authoritative source.
- Phone-test run #9 then passed the complete test/build/verify pipeline and published prerelease `phone-test-9-1` with the DOS quick overlay and pause interlock present.
- Phone-test run #10 passed the complete test/build/verify pipeline with per-module DOS profile selection/tuning present and published prerelease `phone-test-10-1`.


### Native procedural modules

- Added ABI v1 for trusted APK-packaged native game modules with explicit structure/version validation and shell-owned lifecycle/render boundaries.
- Added a compile-time module allowlist and fail-closed `dlopen`/entry-symbol validation instead of loading arbitrary native code from writable storage.
- Made the headset application's NDK build independent of optional DOSBox Pure while preserving both `armeabi-v7a` and `arm64-v8a`.
- Vendored the minimal public-domain OpenKTG texture generator from pinned upstream revision `72f7697c8b5be6fadae41f9ca6312cd5f88fdc4c` with provenance.
- Added the separate Procedural Test Chamber shared library. It expands an OpenKTG texture from recipe/seed data, renders a low-complexity GLES2 room, consumes normalized movement/primary input and restores host GL state after rendering.
- Added explicit GL-context release to the module lifecycle so destructors do not depend on Android teardown happening on a render thread.
- Added the Java native-module runtime bridge and built-in descriptor enumeration.
- Added a dedicated native-test binding profile: touchpad X/Y feed virtual joystick movement and Select feeds primary action.
- Added a sixth VR Home action that launches the packaged Procedural Test Chamber without leaving the shell Activity.
- Native modules now receive user-IPD-corrected Cardboard per-eye view/projection matrices, Activity pause/resume, normalized movement/action input, host-owned Back escape, render-thread GL release and the shell-global battery HUD.
- Android NDK compile/link and Galaxy S9 stereo/input/lifecycle/performance validation remain.

### Bundled DOOM Shareware sample

- Added a checksum-pinned build path for the original `doom19s.zip` DOOM Shareware v1.9 archive.
- Added POSIX and Windows fetch helpers using the known SHA-256 `cacf0142b31ca1af00796b4a0339e07992ac5f21bc3f81e7532fe1b5e1b486e6`.
- Kept the third-party binary outside ReverieVR git while allowing it to be packaged unchanged as a nested Android asset.
- Made free release packaging expect the shareware archive by default, with an explicit `-PexcludeDoomShareware` escape hatch.
- Added runtime SHA-256 re-verification and automatic registration of **DOOM Shareware v1.9** with the built-in Doom binding profile.
- Recorded the legal boundary: shareware is copyrighted and not GPL/public-domain; paid/commercial distributions require appropriate permission or omission of the payload.
- Added bounded, path-traversal-safe extraction of the verified outer shareware ZIP into an app-private runtime working tree while preserving the original asset byte-for-byte.
- Added generated DOSBox Pure `DOS.YML` metadata and a first-run bootstrap batch that drives DEICE, expands `DOOMS_19.EXE`, installs a Reverie-owned Doom config, and launches `DOOM.EXE`.
- Added direct post-install launch metadata: once `DOOMS/DOOM.EXE` exists, subsequent launches bypass installer input automation entirely.
- Added a first-run Doom configuration with mouse input and Sound Blaster SFX matched to DOSBox's 0x220 / IRQ 7 / DMA 1 baseline; music remains deliberately disabled pending audio/music validation.
- Added pure-Java tests for ZIP extraction bounds/traversal defense and for the install-vs-direct-play bootstrap plans.
- Phone-test run #11 passed the complete JVM/Android/NDK build/verify pipeline with this bootstrap present and published prerelease `phone-test-11-1`.



### Diagnostics and retro framebuffer geometry

- Added Standard and Development logging modes.
- Added bounded rotating app-private log files with Standard incident/milestone history and intentionally high-volume Development diagnostics.
- Added uncaught fatal-exception capture while preserving Android's existing crash handler.
- Added Stage A logging-mode selection, clear-log control, and manual diagnostic ZIP export through Android's document UI.
- Kept all diagnostic submission manual; no automatic GitHub/network reporting is implemented.
- Added a retro display-mode catalog that separates source pixel dimensions from intended physical display aspect.
- Added explicit Mode 13h/VGA/EGA/text/handheld source modes plus integer-scale calculations.
- Kept Cardboard authoritative for VR eye viewport/projection/distortion rather than hard-coding generic per-eye resolutions or a universal lens supersampling multiplier.
- Declared Development-logging runs invalid for formal performance/thermal/frame-pacing acceptance.



### DOS modules, bindings and standard HID

- Added a stateful virtual input binding engine that maps Daydream/controller/gamepad/head signals into virtual keyboard, mouse and joystick outputs.
- Added press/release lifetime, analog/threshold transforms, deadzones, sensitivity scaling and persistent versioned binding profiles.
- Added built-in starter profiles for Doom-style DOS control, generic head-mouse FPS control and touchpad absolute-cursor control.
- Added direct Bluetooth/USB/Android HID keyboard passthrough and mouse buttons, relative/absolute motion and wheel input.
- Added mouse delivery through both generic-motion and mouse-sourced pointer dispatch to cover Android HID behavior.
- Added a generic Stage A DOS module importer for DOSBox Pure-supported content, copied into app-private storage with stable full paths.
- Added persistent DOS module metadata/listing and library clearing without deleting the user's original source files.
- Accepted GPLv2/GPLv2+ for the embedded DOS runtime and pinned DOSBox Pure `1.0-preview6` / `a4a0bab7f8931433588f2fcad9045c85b277373d`.
- Added verified DOSBox Pure fetch scripts and recorded corresponding-source/release obligations.
- Added conditional Android NDK integration for the pinned DOSBox Pure checkout; shell/media-only builds still work when the checkout is absent, while DOS-enabled builds now emit the upstream `libretro.so` core plus ReverieVR's `libreverie_dos_host.so`.
- Added the JNI/libretro DOS frontend with real core init/load/run/unload/deinit lifecycle, app-private system/save/content directories, legacy core-option defaults, shutdown handling and Android log forwarding.
- Added tightly packed XRGB8888 framebuffer capture, bounded stereo PCM buffering, and synchronized virtual keyboard/mouse/joystick polling behind the existing `DosNativeRuntime` API.
- Forced the initial Voodoo path to DOSBox Pure's software multithreaded renderer until ReverieVR implements a libretro hardware-render callback.
- Changed `DOS_RUNTIME_BUILT` from a hard-coded false value to the actual presence of the verified pinned source checkout at Gradle configuration time, and extended bootstrap verification to reject a fetched checkout at the wrong commit.
- Host-side C++ syntax/type checking and JNI descriptor verification passed locally; Android NDK compile/link, Stage B framebuffer/audio consumption, and Galaxy S9 runtime validation remain required.
- Added a Stage B DOS session owner that runs the native core on a dedicated worker thread, drives virtual input polling, and stops cleanly on shell/lifecycle exit.
- Added an Android AudioTrack stereo PCM sink with runtime sample-rate initialization and bounded chunk draining off the Cardboard GL thread.
- Added a nearest-filtered guest framebuffer renderer that uploads the native XRGB8888 frame, shader-swizzles host byte order, flips the libretro image vertically, and applies the RV-0414 intended display aspect.
- Added a fifth VR Home action that opens a paged in-headset DOS library, with honest runtime/module-unavailable labels and direct launch of any imported/bundled module.
- Added temporary hosted binding-profile activation/restoration so a module profile does not permanently overwrite the user's prior active binding profile.
- Replaced the temporary Back = immediate DOS exit behavior with an in-headset DOS quick overlay rendered translucently over the paused guest framebuffer.
- The quick overlay now provides Resume, Recenter, volume down/up, Home, and Exit VR recovery actions and displays the active hosted binding profile.
- Added independent lifecycle and overlay pause reasons so Android background/resume cannot unpause a DOS guest behind the quick menu.
- Opening the overlay releases existing guest input and suppresses controller bindings, head-mouse deltas, keyboard, mouse buttons, pointer motion, and wheel events from accumulating stale guest state while paused.
- Added deterministic pause-gate tests covering overlay/lifecycle interlock behavior.
- Added a second paused DOS binding page reached from the quick overlay.
- Profile Previous/Next now cycles only DOS-capable built-in profiles and persists the selected profile in that module's metadata for future launches.
- Analog sensitivity can be adjusted by 10% steps and deadzone by 0.02 steps; edits are stored as a bounded module-local custom profile without overwriting the user's global binding profile.
- Reset Tuning deletes only that module's custom override and restores its selected built-in profile.
- Hosted-profile replacement preserves the pre-game global profile for restoration when DOS exits.
- Added pure-Java tuning tests for signed analog sensitivity, deadzone clamping and no-op behavior on digital-only profiles.
- Left full arbitrary per-binding remapping, directory-tree import, richer module browsing, installer/autostart completion and reference-device validation as the next DOS-host gates.



### Global VR power HUD

- Added a separate persistent stereo PHONE/CTRL battery HUD with two real progress bars.
- Kept the HUD visible across the VR shell, setup flow and local video playback when enabled.
- Added honest unknown-controller presentation instead of treating unavailable telemetry as 0%.
- Wired the existing Show Percentages preference to numeric HUD values while preserving the bars.
- Implemented the optional player-look-up reveal: compact upper-right during normal viewing, lower in view when looking steeply upward.
- Kept Battery HUD and Look-up Reveal as user-toggleable QoL options.
- Reused HUD texture/geometry storage without per-eye allocation.
- Left controller-pointing-up reveal pending physical controller-axis validation rather than guessing the quaternion orientation.



### Setup and input familiarization

- Bumped the resumable first-run setup schema to v2.
- Added an in-VR controller familiarization page showing the active input source and most recent normalized action.
- Captured Back locally on the familiarization page so it can be tested safely without leaving the page.
- Preserved the Optical / Display Calibration Home shortcut so it still jumps directly to IPD.



### ReverieVR Controller companion

- Added a second lightweight Android application module, `controller-app`, with application ID `io.github.mrcalzon02.reverievr.controller`.
- Added a spare-phone controller UI with a large touchpad, Select, App/Back, Home/Recenter, and physical volume-button forwarding.
- Added rotation-vector orientation, gyroscope, and accelerometer transmission over the historical Daydream controller-emulator RFCOMM framing.
- Added a bonded-client-only RFCOMM server using UUID `ab001ac1-d740-4abb-a8e6-1cb5a49628fa`.
- Added separate prioritized control and coalesced sensor queues so high-rate pose traffic cannot strand button-up events.
- Added a backward-compatible ReverieVR status extension carrying the companion phone battery percentage; historical emulators that omit it continue to report battery as unknown.
- Added pure-Java companion protocol-writer tests and headset-side parser coverage for the battery extension.
- Hardened the headset updater so it cannot accidentally select a controller companion APK from a multi-APK GitHub Release.
- The companion app remains draft pending build and two-phone hardware validation.



### Local media

- Added Android Storage Access Framework local-video selection with persisted per-document read access instead of broad storage permission.
- Added explicit flat-cinema and mono equirectangular-360 projection choices.
- Added a platform MediaPlayer decoder backed by an OpenGL ES external OES SurfaceTexture.
- Added shell-owned video entry, Daydream click play/pause, horizontal touchpad-swipe ±10-second seeking, Menu/back return to VR home, and decoder reattachment across GL-surface recreation.
- Added an inward-viewed equirectangular sphere renderer for mono 360 video.
- Kept stereoscopic layouts, seeking/library UI, subtitle work, and sustained thermal acceptance as later media work.

### VR runtime and controller integration

- Added an independent Android BLE backend for the Daydream controller: discovery, bonding, GATT connection, pose/input packet decoding, battery/voltage telemetry, recenter command, and a live Stage A input test.
- Added a multi-provider controller manager: physical Daydream BLE and historical second-phone Daydream controller-emulator RFCOMM are separate transports.
- Added paired-phone controller selection using the historical RFCOMM UUID and an independently authored minimal protobuf-wire decoder for touch, orientation, sensor, and key events.
- Added live Android gamepad/joystick detection as an alternate Stage-B readiness source.
- Added a normalized `VrInputAction` router so physical Daydream, phone emulator, generic gamepad, and Cardboard trigger/system input share Select/Back/Recenter/navigation/volume semantics rather than leaking raw buttons into the VR activity.
- Added pure-Java phone-controller protocol parser fixtures.
- Added a generic controller-provider/manager boundary so later controller types do not require rewriting the VR shell.
- Pinned Google Cardboard SDK v1.35.0 at commit `5969239e7c87f4cd64c8ec170ce1e7f4eb559e37`.
- Disabled Cardboard Vulkan and Unity-plugin native paths; ReverieVR uses the OpenGL ES path.
- Added an application-scoped controller lifetime across Stage A and Stage B.
- Added the first Cardboard Stage B activity and world-space VR shell with head-gaze selection, Daydream click activation, back/recenter handling, power status, and resumable first-run setup pages.
- Added functional initial setup controls for neutral forward direction, virtual user eye spacing, UI scale, and HUD preferences.
- Added pure-Java unit tests for the Daydream packet decoder and updater version comparison.
- Hardened controller readiness so READY is not emitted until a valid pose packet is actually received.
- Hardened failed BLE setup/permission paths to close dead GATT sessions rather than leave zombie connections.
- Removed avoidable per-frame VR-shell allocations and switched shell UI texture refreshes to persistent bitmap storage plus texture sub-image updates.


### Developer diagnostics

- Added first-class scrcpy/ADB compatibility requirements for Galaxy S9 development.
- Added Windows and POSIX scrcpy launch helpers using stay-awake and Android-10-safe no-audio defaults.
- Defined Stage A remote-control and Stage B mirror/record validation while Daydream BLE remains active.
- Prohibited treating scrcpy-assisted sessions as performance/thermal acceptance evidence.

### Android bootstrap

- Added a native Java/XML Android application skeleton for the Stage A pre-headset setup surface.
- Added a conventional touchscreen setup menu with device identification and real phone-battery reporting.
- Added persistent quality-of-life toggles for the future VR battery HUD, look-up reveal behavior, numeric battery percentages, and retro performance-first mode.
- Added visible physical-controller Pair / Sync, paired-phone controller, and Test actions with honest readiness gating.
- Added an Android Bluetooth-settings shortcut without claiming generic Bluetooth pairing is sufficient for the Daydream controller.
- Added local settings reset/recovery behavior.
- Added an Enter VR control gated on a usable dedicated controller source or Android gamepad/joystick.
- Added build instructions and ADR-0003 establishing a lightweight native Android Stage A boundary.
- Added the verified Gradle 9.6.1 wrapper and pinned both wrapper-JAR and distribution SHA-256 values.
- No APK build or reference-device validation has yet been claimed.

### Project foundation

- Adopted AI Project Manager-compatible project governance and repository execution rules.
- Established the ordered ReverieVR APK construction plan.
- Defined a two-stage boot model: 2D touchscreen setup/controller readiness followed by explicit entry into the 3D VR home.
- Required a recovery path from VR back to the 2D setup surface for pairing, input, display, and startup failures.
- Defined the Galaxy S9 + Daydream View seated head-look/controller interaction baseline.
- Adopted performance-first, retro-first VR rendering doctrine and sustained-device performance as an acceptance concern.
- Established backlog, execution-state, acceptance-ledger, and architecture-decision record structure.
- Formalized the persistent Stage B platform-shell contract so home, media player, and hosted games/modules share one global status/settings/recovery layer.
- Defined the future VR power HUD as two shell-global percentage/progress indicators for handset and bound-controller battery, with honest unavailable state when controller telemetry cannot be read.
- Defined the reference HUD position as the upper-right region and recorded optional look-up-triggered reveal/retract behavior as a user-toggleable quality-of-life feature with persistent/manual fallback.
- Removed the initial placeholder `test` file.
