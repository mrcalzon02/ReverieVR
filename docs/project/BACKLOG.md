# ReverieVR Backlog

**Role:** intent and prioritization. This file does not prove execution.

**Execution decomposition:** `docs/project/INCREMENTAL_DEVELOPMENT_PLAN.md` converts these stable backlog intents into ordered, handset-testable slices, dependencies, release cadence, and evidence gates. Keep detailed work-package decomposition there rather than inflating each RV item here.

## P0 — Foundation

- RV-0001 — Adopt AI Project Manager-compatible project governance and repository execution rules.
- RV-0002 — Research and select the minimum Android VR/rendering framework and controller integration stack for Galaxy S9 + Daydream View, including license/provenance, offline behavior, 2D-to-VR transition support, and pairing implications.
- RV-0003 — Create Android/Gradle APK skeleton with reproducible local build instructions.
- RV-0004 — Establish device capability/diagnostic reporting.
- RV-0005 — Establish performance instrumentation and sustained-test procedure.
- RV-0006 — Establish scrcpy/ADB developer-mirroring compatibility: Stage A remote operation, Stage B observability/recording, USB-debug coexistence with Daydream BLE, Android-version-aware audio expectations, and a strict rule that scrcpy-assisted sessions are not performance/thermal acceptance evidence.

## P1 — Pre-VR boot and controller readiness

- RV-0090 — Implement conventional 2D touchscreen boot/setup surface.
- RV-0091 — Implement controller pairing/sync, readiness state, battery telemetry where available, pose/button/touch decoding, recenter behavior, and controller test workflow. Reference sources are physical Daydream BLE, a paired Android phone using the historical controller-emulator RFCOMM protocol, and ordinary Android gamepads/joysticks through the normalized action layer.
- RV-0094 — **Draft implementation present.** Finish and validate the optional independently authored **ReverieVR Controller** companion APK for a spare Android phone: orientation/gyro/accelerometer transport, touchpad surface, primary/App/Home/volume controls, bonded RFCOMM compatibility transport, real controller-phone battery telemetry, reconnect behavior, clear connection state, and no dependency on Google VR Services.
- RV-0092 — Implement explicit 2D-to-VR entry plus safe fallback/recovery to 2D setup.
- RV-0093 — Implement optional authoritative GitHub Release update checking, user-notified Update/Not now choice, APK integrity validation when a digest is published, and Android package-installer handoff.

## P2 — VR shell/menu

- RV-0100 — Implement stereoscopic ReverieVR home shell.
- RV-0101 — Implement seated head-look orientation and reticle.
- RV-0102 — Implement controller/input action abstraction across Daydream BLE, phone-controller emulator input, Cardboard trigger/system controls, and Android gamepad/joystick events.
- RV-0103 — Implement VR-operable menu.
- RV-0104 — Implement persistent settings model and safe-reset path.
- RV-0105 — Implement in-headset diagnostics/performance display.
- RV-0106 — Implement shell-global handset/controller power HUD with percentage/progress presentation and honest unavailable states.
- RV-0107 — Implement optional gaze-adaptive status reveal/retract plus persistent/manual fallback and local QoL toggles.
- RV-0108 — Implement versioned first-VR-run onboarding/calibration wizard with resumable progress, safe defaults, skip/defer behavior, completion summary, and **Run VR Setup Again** entry point.
- RV-0109 — **Draft implementation present.** Package the user-supplied **Starry Cereal** track as looping VR shell/environment ambience with lifecycle-safe pause/resume and automatic suppression while Media, DOS, or Native hosted content is active. Remaining gate: Galaxy S9 + Daydream View mix/comfort validation; user-facing music volume/mute belongs under RV-0203 audio settings rather than an inert placeholder control.

## P3 — Settings and controls

- RV-0200 — VR/display settings.
- RV-0201 — performance/quality settings and dynamic fallback.
- RV-0202 — Implement persistent input mapping settings and in-VR binding editor over the virtual keyboard/mouse/joystick output bus.
- RV-0207 — Implement standard Bluetooth/USB/scrcpy keyboard and mouse passthrough into the virtual guest keyboard/mouse, including relative pointer capture for hosted DOS sessions and a host-reserved overlay/recovery control.
- RV-0203 — audio settings.
- RV-0204 — player defaults.
- RV-0205 — developer/diagnostic settings.
- RV-0208 — **Draft implementation present.** Finish and device-validate secure one-tap diagnostic submission: explicit preview/coarse redaction, credential-free handset upload to the private Cloudflare/R2 intake broker, automatic GitHub issue creation with stable diagnostic ID/hash/reference, Durable Object rate limiting, retention controls, and manual-export fallback. ADR-0017 governs the security boundary. Do not mark complete until the broker is deployed and a real handset submission reaches a GitHub issue with matching server/client hash evidence.
- RV-0206 — Implement user optical/IPD calibration separately from the physical viewer profile: persistent user IPD, live per-eye alignment correction, binocular fusion test pattern, safe reset/default, and device-validated limits.
- RV-0210 — Implement a persistent global comfort-preference profile that hosted modules can consume: seated mode, snap/smooth turning, turn speed, vignette/tunneling preference, movement direction, and related safe defaults.
- RV-0211 — Implement universal recenter and seated-height/forward-offset recovery accessible from the shell and controller without digging through module menus.
- RV-0212 — Implement UI accessibility controls: text/UI scale, contrast, brightness/gamma guidance, dark interface, and no color-only critical states.
- RV-0213 — Implement audio accessibility defaults: subtitles/captions, caption size/background/position, visual alternatives for critical audio, mono/balance options where practical.
- RV-0214 — Implement input accessibility and fallback: configurable actions, one-controller navigation, left/right-hand presentation, larger targeting tolerance, optional gaze/dwell selection, and no precision-timing requirement for essential shell actions.
- RV-0215 — **Draft implementation present.** Implement controller-quality controls: gyro recalibration, drift/deadzone settings, touch sensitivity/deadzone, disconnect pause/reconnect overlay, and recovery without losing module state. The handheld representation now has headset-reference-relative placement, independent bounded inertial translation from controller acceleration, and a deliberate double-sharp-shake gesture that recenters controller position without rotating the room or overwriting tracked orientation. Remaining gate: Galaxy S9 + physical Daydream controller tuning for drift, shake threshold, and translation scale.
- RV-0216 — **Draft implementation present; functional scope wired.** Universal Quick Menu now provides headset/controller forward calibration, summon-time headset anchoring, per-session brightness down/up, contextual Back, Home, Exit/Recovery, phone/controller battery, elapsed session time, Android thermal status, battery-sensor temperature, rolling frame-time p95, and nested shell settings for volume down/up, HUD/percentages/look-up reveal/pointer mode/UI scale. The physical controller rocker is no longer shell-owned Android volume input; its two directions remain normalized hosted-game inputs and are assigned as native game buttons 1/2 in the built-in native profiles. Modal keyboard navigation remains available with arrows + Enter/Space + Escape/Back. Remaining gate is Galaxy S9 + Daydream View comfort/readability, heading-polarity, shake sensitivity, and interaction validation.
- RV-0217 — Implement nonblocking notification policy with user-selectable categories, durations, placement, animation/reduced-motion behavior, and no mandatory center-screen modal for routine status.
- RV-0218 — Implement optional session timer and break reminders with complete opt-out.
- RV-0219 — Research an optional S9 rear-camera 'real-world peek' mode. Treat it as a convenience view, not room-scale passthrough or a safety boundary, and reject it if latency/distortion is uncomfortable.
- RV-0222 — **Implemented; S9 proof pending:** Native locomotion gesture re-arm: entering a module, closing a menu, touchpad click, BLE dropout or mode change requires a fresh touchpad release before travel resumes; controller touch coordinates are sampled as one immutable snapshot per frame. No unintended movement when a thumb is already held across a transition. JVM gate tests.
- RV-0224 — **Implemented; S9 proof pending:** Translucent headset-relative neutral-target Daydream controller, procedural spring from live hand to neutral, and short generated spring cue. Tracked orientation and inertial input remain live; transparency restores GL state. Confirm stereo, audio, head turns, cancellation and comfort on Galaxy S9.
- RV-0223 — **Implemented; S9 proof pending:** Controller shake now requires two opposing sharp acceleration impulses, 65–350 ms apart with 3 s cooldown, ignoring touchpad activity. Recenter begins a three-second slow positional return rather than instantly resetting handset/hand anchor; live motion and quaternion orientation remain enabled. JVM detector/translation tests; validate gesture thresholds and return on hardware.
- RV-0305 — **Implemented; discharge proof pending:** Battery ACTION_BATTERY_CHANGED receiver now updates phone percentage as well as temperature using validated level/scale conversion; only changes dirty the HUD. Minute-by-minute logs no longer rely solely on startup/resume readings. Test broadcast accuracy and sustained S9 discharge before inferring power use.
- RV-0221 — **Implemented; S9 proof pending:** Daydream touchpad gives bounded continuous view-relative native 3D movement: 0.28 radial deadzone, 0.70 m/s cap, normalized diagonals, no inertia, release/click/Quick Menu/stale-input gating, no artificial head rotation. Native modules now opt in through the ABI-v1 `REVERIE_NATIVE_CAPABILITY_SHELL_LOCOMOTION` tail and declare their own horizontal envelope: Test Chamber ±1.55 m X/Z; Red Ledger ±0.70 m lateral and ±0.32 m depth. JNI validates/caches the declaration once at launch, so the renderer no longer contains module-id locomotion branches. Shell applies one camera translation to both eyes and controller/pointer; duplicate Test Chamber module translation and touchpad-to-joystick bindings remain removed. JVM motion/gate tests cover direction, speed, bounds, stop and input ownership. Validate on Galaxy S9 for comfort, stereo and geometry/pointer reach.
- RV-0220 — **Planned interaction primitive:** support an explicit world-anchored controller/tool attachment mode for mounted guns, turrets, yokes, fixed levers, vehicle controls, and similar fixtures. This is an opt-in module mechanic only; the normal handheld controller remains headset-reference-relative with independent tracked rotation and bounded inertial translation.

## P4 — Performance baseline

- RV-0300 — Build retro-complexity calibration scene.
- RV-0301 — Define measured reference-device performance/thermal acceptance thresholds from actual Galaxy S9 tests.
- RV-0302 — Establish performance regression procedure.
- RV-0304 — **Implemented; S9 gate pending:** Reuse stereo GL viewport/scissor query buffers (six fewer short-lived arrays per frame), reuse viewport readback in eye diagnostics, and report separate Development-only left/right CPU submission avg/p95/p99/max once per minute (600 samples/eye). Log battery percentage with existing frame/thermal data. CPU submission is not GPU/compositor time; confirm real stereo visuals and sustained S9 frame/thermal behavior.
- RV-0303 — Procedural-first native asset/power budget: compact seeded recipes, one-time generation and cached GPU uploads, low-cost shading, no per-frame texture synthesis, and measured S9 CPU/GPU/battery/thermal acceptance. Track per-eye cost and stereo correctness; never trade VR frame pacing for a marketing battery target.

## P5 — Module host

- RV-0400 — Define and implement versioned native module ABI/lifecycle with a compile-time allowlist, packaged native-library loading, shell-owned GL/lifecycle/input/recovery services, and fail-closed ABI validation. ADR-0015 governs the first implementation.
- RV-0401 — Implement the built-in Procedural Test Chamber as a separate packaged native module using pinned OpenKTG-generated content, simple GLES rendering, movement input, lifecycle logging, and clean shell return.
- RV-0402 — Enforce platform settings/input/performance contracts for native modules: host-reserved recovery controls, shell-global HUD continuity, Activity pause/resume, bounded capability surface, and reference-device performance/thermal validation.
- RV-0403 — Maintain the ReverieVR Native Game Developer Handbook and stabilize the game-facing SDK/API: document ABI v1, host/module ownership, lifecycle/input/persistence/stereo contracts, procedural material/geometry/model-generation standards, performance/thermal gates, and the promotion path from game-specific proof to shared SDK. Maintain `reverie_native_sdk.h` as the small shared helper layer for repeated cross-game correctness rules, with a canonical module-start pattern and no game-private behavior. Maintain focused shared components such as `reverie_native_gl_state.h`, `reverie_native_math.h`, and `reverie_native_gl_utils.h` when multiple modules repeat platform correctness/utility logic. Shared utilities must remain the smallest proven common behavior; do not promote game-private mechanics speculatively. Enforce active truth through a source-to-document contract index, same-change synchronization rule, and machine-verifiable drift guard. The first optional API-tail capability is shell locomotion; use this extension pattern rather than putting new metadata into the frozen v1 descriptor. The first append-only host-service tail beyond persistence is shell feedback cues: focus/activation/failure flags are coalesced during update and mapped to existing shell audio feedback, while arbitrary audio mixing and haptics remain unimplemented. Maintain the host-buildable `native_abi_contract_test.cpp` + `verify-native-abi-contract.py` gate for prefix compatibility, frozen-layout rules, optional tails, host-service tails, capability checks, and GLES admission whenever the ABI/helper layer changes. Treat higher-level conventions as provisional until repeated module use and reference-device validation justify stable compatibility guarantees.
- RV-0410 — Integrate pinned DOSBox Pure as a ReverieVR-hosted libretro DOS runtime under the accepted GPLv2+ distribution policy.
- RV-0411 — Implement generic DOS game-module import/library for DOSBox Pure content, with app-private full-path copies, per-module launch state, saves/configuration, and binding-profile association.
- RV-0412 — Implement hosted DOS video/audio/input bridge and lifecycle: core load/run/pause/resume/unload, framebuffer/audio delivery, virtual keyboard/mouse/joystick polling, save directory, and safe return to ReverieVR Home.
- RV-0413 — Implement in-game quick overlay for DOS modules with binding editor, profile selection, pause/resume, recenter, audio, Home, and Exit/Recovery.
- RV-0414 — Implement retro framebuffer presentation: recognized source modes, intended display/pixel aspect correction, optional integer/nearest scaling, independent high-resolution VR UI, and Cardboard-owned per-eye composition.
- RV-0415 — Bundle the original verified DOOM Shareware v1.9 archive as the default free-release DOS sample, preserve redistribution/license provenance, register it as a built-in module, and implement first-run installer/autostart behavior without repacking the distributed archive.

## P6 — Media player

- RV-0500 — Local flat virtual-screen playback.
- RV-0501 — 180/360 projection.
- RV-0502 — stereoscopic media layouts.
- RV-0503 — playback controls and file/library UI.
- RV-0504 — sustained decode/thermal validation.

## P7 — First games/experiences

- RV-0600 — Define first tiny retro VR gameplay prototype.
- RV-0601 — Implement and device-test first playable module.
- RV-0602 — Add further experiences only after regression/thermal gates remain healthy.
- RV-0610 — **Draft manual-service/dev-test foundation present:** **Between Deliveries: The Red Ledger VR** now has the deterministic economy, seated GLES2 room, cup→fill→serve→payment→wash interaction loop, supplier/protection/ledger/turn-away targets, v2 mid-transaction saves, and verified dual-ABI packaging from phone-test #52. The packaged module is admitted only for physical testing: Standard logging hides it, Development logging exposes `[DEV]`, runtime launch is independently Development-gated, and its hosted profile maps Select only. Phone-test #60 verifies that admission/profile path and packages it with the current stereo-isolation repair. The next build adds a Development-only ray-contact cursor at the actual selected target and defensive pointer-direction normalization. Remaining work is real Galaxy S9/Daydream target/reach/held-cup tuning, recovery/recenter, save/update durability and the 15-minute runtime/thermal gate before normal visibility.
- RV-0611 — **Implemented foundation; current packed path is RV-0613:** Red Ledger's four deterministic stone/wood/metal/paper recipes generate one 128×128 atlas at GL-context creation, shared across all cubes and both eyes with no bitmap files or per-frame generation. The original RGBA8 proof path remains as a tested reference generator, while the live Red Ledger upload path is direct RGB565 as recorded under RV-0613. Deterministic seed/diversity/bounds checks remain; real Galaxy S9 visual/thermal/power measurements are still required before treating procedural generation as a net energy win.
- RV-0612 — **Implementation pending S9 proof:** Red Ledger bakes 11 immutable cubes (396 vertices, 15,840-byte VBO) at GL-context creation and renders them in one draw per eye, down from 11. Hoverable objects remain dynamic. CI covers deterministic geometry and buffer/UV safety; handset visual, frame-pacing, and thermal gates remain.
- RV-0613 — **Implementation pending S9 proof:** Red Ledger generates its seeded atlas directly as RGB565 (32 KiB vs 64 KiB RGBA8) and calculates flicker once per simulation update, sharing illumination between stereo eyes. Native CI compares packed pixels to RGBA reference and checks bounds/determinism. Gate: S9 material appearance, frame pacing and battery/thermal measurements.
- RV-0620 — **Planned:** prototype **Ministry of Intelligence: Lantern Desk VR** as a seated intelligence-analysis native game; preserve strict World Truth vs Ministry Knowledge separation and prove one complete reconnaissance-photo-to-report cycle before expanding the war simulation.
- RV-0630 — **Planned:** prototype **Iron Sight: Forward Detachment VR** as a compact reconnaissance/patrol native game; begin with one sector, one small detachment, route/identification/reporting pressure, and deliberately constrained combat rather than the full campaign.

## Deferred

- broad historical Daydream app binary compatibility;
- public store/cloud services;
- additional handset/controller compatibility;
- network/social services;
- graphically intensive experiences.
