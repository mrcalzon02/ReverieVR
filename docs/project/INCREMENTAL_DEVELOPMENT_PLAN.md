# ReverieVR Incremental Development Plan

**Role:** executable roadmap derived from `BACKLOG.md`, `APK_BUILD_ORDER.md`, the acceptance ledger, execution state, ADRs, and current Galaxy S9 test feedback.  
**Status:** living plan. It decomposes backlog intent; it does not itself prove implementation or acceptance.  
**Reference target:** Samsung Galaxy S9 + Daydream View, with the physical Daydream controller as the primary reference controller.

## 1. Planning doctrine

ReverieVR has enough implemented surface area that the main risk is no longer "can we build a VR APK?" It is integration drift: individually implemented systems can exist while the installed product has stereo, scale, input, update, lifecycle, or recovery defects. Development therefore proceeds in **small installable slices**. Every slice must produce one observable improvement, preserve recovery, and be suitable for a phone-test release when it changes runtime behavior.

The backlog remains the authoritative catalogue of intent. This document supplies ordering, dependencies, acceptance evidence, and smaller work packages.

A slice is complete only when all applicable stages are satisfied:

1. **Intent** — identify backlog IDs, user-visible behavior, invariants, and explicit non-goals.
2. **Implement** — change the authoritative source; no placebo controls and no bypasses.
3. **Static verify** — compile/test/lint or equivalent checks; inspect generated package where relevant.
4. **Release verify** — for handset-facing work, produce a persistently signed phone-test APK and verify versionCode, package ID, signer, and release asset.
5. **Device verify** — exercise the changed behavior on the Galaxy S9/Daydream View when hardware is required.
6. **Regression verify** — retest shell recovery, quick menu, controller route, and update path affected by the change.
7. **Record** — update execution state/acceptance evidence only to the level actually demonstrated.

No runtime control is exposed merely because a feature is planned. A visible control must reach implemented, observable behavior in the same slice.

## 2. Immediate product objective

The next milestone is **Reference Headset Usability Baseline A**: ReverieVR boots, updates in place, enters VR, renders the same correctly composed world to both eyes, fits comfortably inside the Daydream optics, displays the intended logo and shell audio, accepts a usable controller path, exposes a reliable quick menu/recenter path, and always returns safely to Home or 2D recovery.

This milestone deliberately precedes additional native games, broader settings, visual embellishment, or compatibility expansion. A platform with a broken right/left eye, oversized viewport, unreliable update, or inaccessible recovery menu is not ready to absorb more content.

## 3. Milestone A0 — lock the persistent update channel

**Backlog:** RV-0093, ADR-0018.  
**Purpose:** make every later device slice cheap to deploy and test.

### A0.1 Baseline installation
- Treat phone-test #37 or a later build with the permanent distribution certificate as the baseline lineage.
- Record installed versionCode and signer fingerprint in diagnostic output.
- Confirm the old ephemeral-signed build has been replaced once.

### A0.2 First real in-app upgrade proof
- Publish the next valid runtime slice with a larger versionCode and the same signer.
- From the installed persistent baseline, use ReverieVR's own Check for updates / Update path.
- Verify download digest, candidate package ID, candidate versionCode, installed signer, candidate signer, and package-installer handoff.
- Confirm Android performs an in-place update without uninstall.
- Confirm preferences and app-private state survive.
- Record this as the first device-level acceptance evidence for persistent updates.

### A0.3 Update failure behavior
Incrementally test: no network; no newer release; malformed/non-ReverieVR asset; digest mismatch; same/older version; cancelled installer; unknown-app install permission absent. Each must fail closed with a useful message and leave the installed app usable.

**Exit gate:** two consecutive in-place upgrades on the permanent signing lineage, with retained settings/state and no uninstall.

## 4. Milestone A1 — stereo renderer correctness

**Backlog:** RV-0100, RV-0101, RV-0200, RV-0300.  
**Current device defect:** VR content has been observed in only the right-eye viewport.

This is the highest-priority rendering defect. Do not mask it with a monoscopic duplicate unless that is explicitly used as a diagnostic mode unavailable in normal UI.

### A1.1 Instrument the eye pipeline
- Add development-only frame/eye diagnostics: frame number, eye index, viewport rectangle, framebuffer identity, projection/view matrix summary, render target size, and GL error checkpoints.
- Keep standard logging milestone-oriented; reserve per-eye/per-frame detail for Development logging.
- Add a deterministic stereo diagnostic scene containing a center marker, left/right eye labels in development mode, depth-separated primitives, horizon/grid, and edge-safe reference marks.

### A1.2 Trace Cardboard frame lifecycle
Verify, in order, that both eyes receive:
- a valid eye viewport;
- the correct eye-specific projection;
- the correct eye-from-head transform;
- scene submission;
- distortion/composition;
- swap/present.

Audit for stale framebuffer binding, uncleared scissor state, viewport reuse, early return, right-eye-only draw calls, or incorrect render-target attachment.

### A1.3 Correct stereo composition
- Fix the root cause at the renderer/compositor boundary.
- Both eyes must render the same scene with appropriate binocular offset.
- No eye may depend on a previous eye's framebuffer contents.
- Preserve media/DOS/native host paths by routing them through the same proven composition contract.

### A1.4 Stereo regression fixture
Create a deterministic test/diagnostic contract that catches obvious one-eye regressions before release. Host-side tests cannot prove optical output, so the final gate remains physical-device observation.

**Exit gate:** both eyes display continuously through Home, Quick Menu, and one hosted scene; no one-eye blanking during pause/resume or menu transitions.

## 5. Milestone A2 — optical fit and viewport contract

**Backlog:** RV-0200, RV-0206, RV-0108.  
**Current feedback:** the view remains too large; reduce the usable visual presentation by approximately another 10%.

Separate three concepts that must not be conflated: Cardboard lens/distortion geometry, user IPD/alignment, and the shell's comfortable content-safe region.

### A2.1 Define safe presentation geometry
- Introduce a named shell content-safe scale/overscan parameter rather than arbitrary scattered multipliers.
- Apply the requested approximately 10% additional contraction to the user-visible shell/view content, not by corrupting physical viewer lens parameters.
- Keep distortion output sized correctly for the actual display.
- Ensure HUD, menu, text, and reticle obey a documented safe angular region.

### A2.2 Calibration scene
Add a calibration page with center cross, horizontal/vertical edge markers, readable corner targets, binocular fusion marks, and a "comfort boundary" reference. It must allow reset to known-safe defaults.

### A2.3 Separate user eye alignment
Continue RV-0206 only after A1 is correct. User IPD/per-eye alignment must alter intended eye transforms while preserving the physical viewer profile.

### A2.4 Persistence and bounds
- Persist only validated values.
- Clamp dangerous/extreme transforms.
- Reset must restore a usable display even if prior settings are corrupt.

**Exit gate:** center and important UI are visible without eye strain or exaggerated upward gaze; no clipping introduced by the contraction; both eyes remain fused comfortably.

## 6. Milestone A3 — shell identity and audio integrity

**Backlog:** RV-0100, RV-0109, RV-0203.  
**Current feedback:** new project logo is not appearing at the top of the 2D menu; Starry Cereal is intended as VR environment/menu music.

### A3.1 Logo asset chain
- Trace the supplied logo from repository asset through Android resource/asset packaging to Stage A binding.
- Remove obsolete logo references.
- Verify density/scaling behavior and preserve aspect ratio.
- Add a package-level check that the expected asset is present in the APK.

### A3.2 VR shell identity
Use the same current project identity consistently in 2D setup and VR Home where appropriate; avoid maintaining divergent copies that can drift.

### A3.3 Starry Cereal lifecycle
- Verify the supplied audio asset is packaged.
- Start looping ambience only in shell/environment context.
- Pause/duck/stop when Media, DOS, or Native content owns audio.
- Resume correctly on return to Home.
- Respect Activity pause/resume and audio focus.
- Do not expose mute/volume controls until RV-0203 wiring actually affects playback.

**Exit gate:** logo visible on physical Stage A; music loops in Home, yields to hosted content, resumes cleanly, and does not double-play after lifecycle transitions.

## 7. Milestone A4 — universal quick menu and recenter reliability

**Backlog:** RV-0211, RV-0216, RV-0103.  
**User requirement:** a floating menu must be summonable from controller/menu input and allow recentering toward either the controller-pointed or headset-forward direction, because startup orientation cannot be trusted to face the keyboard/work area.

### A4.1 Invocation contract
- Reserve a shell-global action that modules cannot consume.
- Support the physical Daydream controller, phone controller, generic gamepad, keyboard fallback, and Cardboard/system route where available.
- Debounce invocation and guarantee the menu can be closed.

### A4.2 Floating placement
- Spawn the menu into a comfortable bounded position relative to the current head pose.
- Never place it behind the user or at an unusable pitch.
- Preserve readable distance and scale.
- Re-summoning may reposition it to current forward direction.

### A4.3 Recenter modes
Implement real behavior for:
- **Headset Forward** — current headset yaw becomes forward.
- **Controller Forward** — controller pointing/yaw establishes forward when orientation data is trustworthy.
- If controller orientation is unavailable, do not expose or enable a fake controller-forward action; explain unavailable state or fall back only with explicit labeling.
- Preserve seated height/forward offset separately from yaw where possible.

### A4.4 Recovery invariants
Quick Menu must remain available over Home, Media, DOS, Native modules, onboarding, and recoverable error states. Home and Exit/Recovery must be host-owned.

**Exit gate:** while seated facing an arbitrary direction, the user can summon the menu, recenter toward the desired keyboard/work direction, close it, and continue without restarting the app.

## 8. Milestone A5 — controller representation and normalized input

**Backlog:** RV-0091, RV-0094, RV-0102, RV-0215.  
The 3D controller representation is functional feedback, not decoration.

### A5.1 Capability model
For every active controller provider expose capabilities explicitly: orientation, touch/track surface, primary/secondary buttons, menu/home, analog axes, battery, vibration if any. UI and 3D representation consume capabilities rather than guessing by controller name.

### A5.2 Simplified controller model
Implement the requested low-complexity controller representation: body/cube-like handset, central controls, right-side buttons as applicable, touch/track orb/pad, and interaction emitter/ray. Keep geometry cheap enough for S9.

### A5.3 Pose strategies
- 3DoF device: use quaternion orientation.
- Gamepad/no inertial orientation: use stick-driven cursor/orientation strategy rather than pretending pose exists.
- Phone emulator: use transmitted rotation-vector pose.
- Calibrate neutral orientation and expose drift recovery.

### A5.4 Interaction feedback
Ray, hover, press, drag, and unavailable states must be visibly distinct and driven by actual routed actions.

**Exit gate:** at least physical Daydream and one fallback controller path can operate Home + Quick Menu with understandable visual feedback.

## 9. Milestone A6 — recovery, lifecycle, and shell regression pass

**Backlog:** RV-0092, RV-0103, RV-0104, RV-0216, RV-0402.

Create a reference transition matrix and exercise:
2D Setup -> VR Home -> Quick Menu -> Home -> Media -> Home -> DOS -> Home -> Native -> Home -> Android pause -> resume -> 2D Recovery -> VR Home.

For each transition verify:
- stereo remains valid;
- head/controller pose resumes;
- no duplicate audio;
- global HUD remains shell-owned;
- input focus is correct;
- back/recovery action is reachable;
- settings are not silently reset;
- framebuffer/decoder/native resources are not leaked catastrophically.

**Exit gate:** complete the matrix twice on-device without restart or unrecoverable state.

## 10. Milestone B — measured Galaxy S9 performance baseline

**Backlog:** RV-0005, RV-0300, RV-0301, RV-0302.

Only after Milestone A is usable should performance numbers become acceptance thresholds.

### B1 Calibration fixture
Use the retro-complexity scene with deterministic object count, texture budget, UI overlay, controller ray, and representative shell effects.

### B2 Instrumentation
Capture frame time distribution (not only average FPS), p95/p99 frame time, dropped/late frames where observable, memory, thermal status, battery temperature, sustained clock/throttling symptoms, and render scale.

### B3 Sustained profiles
Run short smoke, 15-minute warm, and longer sustained sessions. scrcpy-assisted runs are diagnostic only and cannot be thermal acceptance evidence.

### B4 Presets and fallback
Define performance-first defaults from measurements. Dynamic scaling must have hysteresis and bounded changes so it does not visibly hunt.

**Exit gate:** measured reference thresholds are written into the acceptance ledger and can be repeated.

## 11. Milestone C — settings completion without placebo UI

**Backlog:** RV-0200 through RV-0218.

Implement settings by vertical slices rather than building a giant settings screen first:

- **C1 Display:** safe content scale, UI scale, viewer profile route, IPD/alignment reset.
- **C2 Input:** bindings, handedness, controller calibration, deadzones, sensitivity.
- **C3 Audio:** master/shell/media/module volume and mute with real mixer behavior.
- **C4 Comfort:** seated profile, turn policy, vignette preference, movement direction.
- **C5 Accessibility:** text scale, contrast, captions, gaze/dwell fallback where implemented.
- **C6 Diagnostics:** standard/development logging selection, export/submission, performance HUD.
- **C7 Notifications/session:** nonblocking notifications and optional break timer.

Every settings slice requires persistence, bounds validation, safe reset, immediate observable behavior where appropriate, and no inert control.

## 12. Milestone D — diagnostics as a development accelerator

**Backlog:** RV-0004, RV-0205, RV-0208.

### D1 Standard vs Development logging
Standard logs record lifecycle milestones, provider changes, recoverable failures, updates, module transitions, and significant thermal/performance events. Development logs may record high-frequency renderer/input detail with bounded storage.

### D2 Session identity
Every diagnostic package should include build SHA, versionCode, signer fingerprint, device/API, viewer/profile, active input provider, relevant settings, and a session identifier.

### D3 User-submitted error path
Finish RV-0208 only with real broker deployment and handset-to-GitHub proof. Until then, manual export remains the truthful fallback.

### D4 Repro bundles
For rendering/input defects, make it possible to attach a bounded recent-event buffer so "right eye blank" or "controller lost" reports include the state immediately before failure.

## 13. Milestone E — native module host stabilization

**Backlog:** RV-0400, RV-0401, RV-0402.

Do not expand native game scope until the host contract is boring and predictable.

Incremental order:
- **E1 ABI identity:** version, module ID, capability declaration, allowlist, fail-closed mismatch.
- **E2 Lifecycle:** create/start/pause/resume/stop/destroy with deterministic logging.
- **E3 Render service:** host-owned stereo targets and module scene callback; modules must not own distortion or bypass eye composition.
- **E4 Input service:** normalized actions and pointer/pose capabilities.
- **E5 Shell services:** Quick Menu, Home, HUD, recenter, audio ownership.
- **E6 Persistence:** module-scoped save/config service with versioning.
- **E7 Budget:** declared memory/render/audio budget and measured S9 behavior.
- **E8 Procedural Test Chamber:** use RV-0401 as the conformance module and regression fixture.

**Exit gate:** the Test Chamber survives the full A6 transition matrix and sustained B-series performance test.

## 14. Milestone F — DOS runtime as first substantial hosted workload

**Backlog:** RV-0410 through RV-0415.

Incremental order:
- F1 deterministic DOSBox Pure bootstrap and provenance verification;
- F2 core lifecycle with no game content;
- F3 framebuffer bridge;
- F4 audio bridge;
- F5 virtual keyboard/mouse/joystick bus;
- F6 real HID passthrough and host-reserved recovery chord/action;
- F7 app-private module filesystem and save/config persistence;
- F8 in-session Quick Menu/binding overlay;
- F9 pixel/display aspect and nearest/integer presentation modes;
- F10 verified DOOM Shareware installation from the original archive;
- F11 sustained S9 play test and recovery test.

Do not make DOOM-specific assumptions part of the generic DOS host.

## 15. Milestone G — media player completion

**Backlog:** RV-0500 through RV-0504.

Use the same host contracts proven above:
- G1 flat local video;
- G2 playback controls and seek;
- G3 180/360 mono;
- G4 SBS/OU stereo layouts;
- G5 subtitle path;
- G6 library/recent-file usability;
- G7 audio-focus coexistence with shell music;
- G8 sustained hardware-decode thermal validation.

Projection correctness should have dedicated geometric fixtures; media projection must not become a second independent stereo compositor.

## 16. Milestone H — first native games, one playable loop at a time

**Backlog:** RV-0600, RV-0601, RV-0610, RV-0620, RV-0630, RV-0640.  
**Order remains:** Between Deliveries -> Ministry of Intelligence -> Iron Sight.

### H1 Between Deliveries: The Red Ledger
Current foundation is useful, but admission to the Native Apps allowlist requires:
1. one work surface and reach/pointer interaction;
2. one complete buy/hold/sell or equivalent economy loop;
3. save/load through host persistence;
4. Quick Menu/Home/recenter compliance;
5. packaged NDK verification;
6. 15-minute S9 thermal/runtime pass;
7. only then expand goods, suppliers, factions, rooms, or narrative systems.

### H2 Ministry of Intelligence: Lantern Desk
Build only one honest intelligence cycle first:
1. one sector selection;
2. one reconnaissance mission;
3. one generated/captured photo;
4. desk inspection/pan/zoom;
5. one annotation/identification decision;
6. one submitted report;
7. hidden truth comparison and consequence;
8. save/load with strict World Truth / Ministry Knowledge separation.

Expand staff, intercepts, aircraft, commandos, broader war state, and campaign systems only after this loop is compelling.

### H3 Iron Sight: Forward Detachment
Build one patrol:
1. briefing;
2. one vehicle/route choice;
3. movement through one compact sector;
4. one uncertain contact;
5. identify/report/avoid/engage decision;
6. resource or damage consequence;
7. return and debrief;
8. persistence of sector outcome.

Vehicles, squad complexity, logistics, and larger combat expand only after this loop works within the S9 budget.

### H4 Breakwater Battery — prioritized playable device slice

RV-0640 defines a stationary coastal gun with development-gated module admission. Verify boat interception prevents one-person rowboat and later 3–6-person landing craft landings; shore troops damage the position; inter-wave shop purchases have observable consequences; day/night multi-day campaigns and late aircraft/anti-air gating remain within the Galaxy S9 budget. Preserve Quick Menu/Home/recenter, ship a signed test APK, exercise the physical controller and thermal regression before normal Native Apps visibility. MIDI-era music is a required future shared service, not yet audible.

## 17. Milestone I — compatibility expansion

Only after the S9/Daydream reference path is accepted:
- additional Android devices;
- additional viewers;
- broader controller mappings;
- per-device optical/performance profiles;
- historical Daydream compatibility research.

Every new target must be additive. It may not weaken or silently change the reference-device behavior.

## 18. Cross-cutting workstreams

### Rendering contract
One shell-owned stereo/composition path for Home, Media, DOS, and Native content. No module-specific eye hacks.

### Input contract
All providers terminate in named actions/capabilities. Modules never bind raw Android keys as their only recovery route.

### Audio contract
Explicit owner/mix policy prevents shell music, media, DOS, and native audio from fighting or duplicating across lifecycle transitions.

### Persistence contract
Settings and module state are versioned, bounded, recoverable, and survive ordinary in-place updates.

### Update contract
Permanent signer, monotonically increasing versionCode, trusted release origin, digest verification, and no silent install.

### Performance contract
Galaxy S9 is the reference budget. Features earn complexity through measurement rather than assumption.

### UX contract
No inert controls. Critical recovery is always reachable. Routine notifications are nonblocking. Unavailable telemetry is shown honestly rather than fabricated.

## 19. Release cadence

Prefer one coherent handset-visible behavior per phone-test release. A useful cadence is:

- **R38:** first post-bootstrap in-place update proof plus any minimal updater corrections discovered.
- **R39:** stereo diagnostic instrumentation and right-eye-only root-cause fix.
- **R40:** approximately 10% additional safe-view contraction plus optical calibration fixture.
- **R41:** current logo asset chain and shell identity verification.
- **R42:** Starry Cereal lifecycle/audio ownership validation.
- **R43:** Quick Menu summon/recenter hardening.
- **R44:** controller representation/capability strategy.
- **R45:** lifecycle/recovery transition-matrix hardening.
- **R46+:** performance baseline and settings vertical slices.

Numbers are planning labels, not promises: failed or diagnostic builds may consume GitHub run numbers. The functional order matters more than the literal release number.

## 20. Evidence required for backlog status

Use four meaningful states:

- **Planned:** intent exists; implementation has not begun.
- **Draft:** authoritative implementation exists but required verification is incomplete.
- **Static accepted:** source/build/tests prove everything possible without reference hardware.
- **Device accepted:** required Galaxy S9/Daydream behavior has been observed and evidence recorded.

A task must not become device accepted from CI success alone. Conversely, device feedback should be converted into a specific defect/slice rather than leaving a broad backlog item permanently vague.

## 21. Backlog maintenance improvements

The existing backlog is strong as a catalogue but too coarse to drive day-to-day execution by itself. Maintain it by:
- keeping stable RV IDs and product intent in `BACKLOG.md`;
- putting decomposed work packages and dependency order here;
- keeping current factual implementation state in `EXECUTION_STATE.md`;
- keeping proof and acceptance in `ACCEPTANCE_LEDGER.md`;
- using ADRs for decisions whose rationale must survive implementation changes;
- recording newly observed device defects against the smallest relevant RV item;
- closing stale "remaining gates" when evidence exists rather than duplicating the same gate across documents;
- never treating a plan, commit, CI build, release, and physical-device acceptance as interchangeable evidence.

## 22. Immediate execution queue

Until Reference Headset Usability Baseline A is closed, the default queue is:

1. prove #37 -> next release in-place update on the permanent signing lineage;
2. fix and device-prove two-eye stereo rendering;
3. contract the comfortable view region by the requested additional ~10% and device-tune it;
4. repair and package the current 2D menu logo;
5. validate Starry Cereal shell-audio lifecycle;
6. harden floating Quick Menu placement and headset/controller-forward recenter;
7. complete controller capability representation and interaction feedback;
8. run the complete shell lifecycle/recovery matrix;
9. establish measured S9 performance/thermal thresholds;
10. then resume settings, module-host, DOS/media, and native-game expansion in the milestone order above.

This queue is intentionally biased toward making the installed headset build dependable. New feature breadth resumes after the core loop is comfortable, updateable, recoverable, and measurable.
