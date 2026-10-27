# ReverieVR Project Instructions

**Role:** INSTRUCTIONS  
**Authority:** stable project doctrine

## 1. Purpose

ReverieVR is an independent **cross-platform VR and desktop platform**. Its existing Android Daydream application makes Daydream-era phone/headset hardware useful again for local applications, games and media without Google's discontinued services. Its expanded product requirement is the **complete ReverieVR desktop environment**, natively runnable on Windows, Linux and macOS with all hosted first-party capabilities, and provisional PC VR via OpenXR/SteamVR where available.

Reference hardware:

- Samsung Galaxy S9 handset;
- Daydream View headset;
- Daydream controller.

Additional compatible Android phones and simple 3DoF viewers are desirable when support does not compromise the reference path.

## 2. Product goals

ReverieVR should:

1. boot and operate without discontinued Google Daydream servers;
2. provide a conventional touchscreen boot/setup surface before VR entry;
3. pair/sync, verify, and recover the reference controller from that touchscreen surface;
4. transition into a local in-headset VR shell/launcher whenever the VR display/runtime is ready, with or without a physical controller;
5. expose complete local settings for the virtual environment and device behavior;
6. provide seated head-look navigation with configurable physical input;
7. support local VR video playback;
8. host small self-contained VR applications/games as subprojects/modules;
9. expose a documented integration surface rather than binding every experience directly to one device;
10. keep shell-level status, settings, recovery, and navigation available across the home, player, and hosted games/modules;
11. provide a shell-global power/status HUD for the handset and currently bound controller when battery telemetry is available;
12. make automatic quality-of-life behavior optional and locally configurable;
13. remain buildable, understandable, testable, and recoverable.
14. provide full native desktop ReverieVR applications for Windows, Linux and macOS, not an APK emulator, single-game port or phone companion;
15. operate the complete Home, rooms, avatars/wardrobe, native games, classics/DOS, media, shell UI, module download/update, saving and recovery using keyboard/mouse or gamepad **without any headset**;
16. provide **optional** OpenXR-based PC VR, provisionally including SteamVR on supported Windows/Linux systems, without making Steam, Steamworks, an XR runtime or Android a dependency of desktop mode.

## 3. Reference interaction model

ReverieVR has two intentional interaction stages.

### 3.1 Stage A — pre-VR touchscreen setup

Before the phone is placed in the headset, ReverieVR launches into a conventional 2D Android touchscreen interface.

This stage is responsible for:

- first-run setup;
- controller pairing/sync;
- controller connection/state verification;
- controller test/calibration where required;
- device/runtime diagnostics needed before entering VR;
- recovery when the VR input path or settings become unusable;
- an explicit **Enter VR** transition when the VR display/runtime is ready; a missing controller must not block headset-only use.

The exact controller transport/pairing implementation is an engineering decision to be verified during framework/runtime and controller-stack research. The user-facing requirement is stable even if the underlying transport changes.

### 3.2 Stage B — in-headset seated VR

After the user enters VR, the baseline seated interaction model is:

- **head orientation/head-look** for view direction and reticle pointing;
- **controller trackpad/touch area** for directional navigation, scrolling, and context-sensitive analog input;
- **primary click/select** for activation;
- **secondary/app/back control** for backing out or context actions;
- **Home** for returning/recentering according to configurable behavior where accessible;
- **two handset side volume buttons** for volume by default, with remapping only where Android/device behavior safely permits it.

Touchscreen interaction is expected and supported in Stage A. It must not be required for ordinary Stage B operation once VR has been entered successfully.

**Permanent minimum input:** If no controller can deliver usable input, the handset's headset head-tracking must still operate Stage B Home, menus and recovery. The center-of-view gaze ray targets shell buttons. Holding a live, eligible button under gaze for five continuous seconds triggers a single Select, with a visible countdown ring and cancellation on target loss/change; the same target cannot fire repeatedly without refocusing. This fallback overrides a stale controller-only pointer choice, and yields immediately when actual controller input becomes available. A hosted game is not promised analog gameplay through gaze alone: losing controller input while hosted must surface a reachable shell recovery menu; a five-second upward look can reopen the menu after dismissal. Stage A entry must remain accessible with the phone and headset alone whenever the Cardboard runtime works.

The Stage B shell owns platform-global UI and behavior that must survive transitions between the home, media player, and hosted modules. Experiences may supply their own content UI, but they do not replace shell recovery, global settings access, or the global status layer.

The reference global status presentation includes two compact percentage/progress indicators in the upper-right region of the VR view: handset battery and the currently bound controller battery. If controller battery telemetry is unavailable, ReverieVR reports an honest unavailable/unknown state rather than inventing a percentage.

A user-selectable adaptive HUD mode may retract the status layer during ordinary forward viewing and bring it down into comfortable view when a deliberate look-up gesture is detected. Head pitch is the baseline signal; controller orientation may participate only if the selected controller stack exposes it reliably. Thresholds, hysteresis, and animation require comfort testing. Disabling adaptive mode must preserve a simple persistent/manual presentation.

Bindings in Stage B must be represented by a configurable input-action layer rather than permanently hard-coded into application logic.

### 3.2.1 Bluetooth controller privacy and entry

ReverieVR must not check system Location-services state, request Android Location permissions, or require Location services as an application-level prerequisite for controller pairing, controller reconnection, standard Bluetooth HID input or VR entry. For controllers already paired through Android (including the original Steam Controller), use `InputDevice` input routing without a new BLE scan. Reuse known Daydream bonds before attempting BLE discovery. Where Android itself filters or refuses an app-initiated BLE scan, expose the actual scan/permission failure without claiming Location must be enabled, silently bypassing that error, or weakening recovery. Android 12+ Nearby Devices (`BLUETOOTH_SCAN`/`BLUETOOTH_CONNECT`) permissions may still apply to direct BLE access. Do not conflate the operating system's constraints on new discovery with a requirement for standard controller operation.

### 3.3 Shell continuity invariant

The ReverieVR shell is not merely the first 3D scene. It is the persistent platform layer around hosted experiences.

At minimum the shell owns:

- return-to-home and recovery routing;
- access to global settings;
- global input-action policy;
- handset/controller power status;
- platform diagnostics/status access;
- module pause/exit boundaries;
- safe transition back to Stage A.

A game or player module must not silently remove those capabilities. Deliberate suppression of noncritical HUD elements must be explicit, reversible, and user-configurable; recovery controls remain reachable.

### 3.4 Recovery invariant

The user must never be permanently trapped by a bad controller state, broken binding, or unusable VR setting.

A reliable route back to Stage A must exist so the user can:

- reconnect/re-pair the controller;
- reset bindings;
- reset VR/display settings;
- inspect diagnostics;
- retry entry into VR.

### 3.5 Single-APK / phone-first authority (Android deliverable; does not exclude native desktop builds)

**The Android ReverieVR APK is a complete phone-resident VR system, never a client requiring a PC.** This Android independence remains mandatory even as the separately compiled complete desktop host is developed.
The canonical Android deliverable is **one ReverieVR APK**. Stage A, Stage B,
stereo/head tracking, Home, input routing, controller-free gaze-and-dwell
recovery, native app/game hosting, media viewing, DOS hosting, the emulator
host infrastructure once implemented, configuration, imported local game
media, and shell recovery must execute on the handset. Core functionality
must remain operable without Wi-Fi, internet access, ADB, scrcpy, a laptop,
a separately installed Android companion, or a desktop helper. Any optional
phone-controller companion must remain optional.

A laptop connected over USB or a user-approved local network is an
**optional peripheral and file source**, not an execution prerequisite. It
may supply a forwarded controller, keyboard, user-approved read-only files,
optical-disc image access, or storage. **The user has explicitly permitted a small Windows Laptop Link utility
when it is bundled within the one ReverieVR APK and exported on demand
from Stage A.** This is not permission for a second required software
distribution, silent PC installation/execution, a privileged service,
an always-running Windows agent, or PC-hosted VR emulation. The APK
currently exports a three-file, human-readable portable PowerShell/cmd
Windows bundle via Android's Create Document picker **or** an explicitly
opened, temporary, single-use download URL over trusted private Wi-Fi
or already authorized USB/ADB port forwarding. Windows must deliberately
download/copy/extract/run it; the APK may not remotely execute programs,
advertise a permanent HTTP service or bypass host authorization.
Its current features launch an
existing scrcpy installation for selected input/mirroring modes and
inspect optical-drive availability without raw-disc access. The
computer's own SMB2/SMB3 sharing remains an alternative source for
user-approved read-only files, with the client inside this APK.
A future enhanced host utility may add narrowly authorized device
protocols, but only after explicit pairing, privacy, compatibility,
resource and licensing gates. The source
computer must explicitly enable sharing/forwarding as needed; no Android
APK can unilaterally read a PC's USB peripherals, raw optical sectors,
or screen pixels. Disconnecting the PC must preserve the handset's core
shell, local media, local input, and locally installed games.
Remote media that has not been imported may become unavailable with a
clear non-fatal message, but unrelated local features cannot fail.

A hotspot hosted by the laptop, the phone's hotspot, or an ordinary Wi-Fi
router are interchangeable **local IP transports** when network conditions
allow. Never require an internet router or account. Android Wi-Fi joining
may still use the platform's existing user-approved network UI. Ordinary
TCP/UDP application networking does **not** grant Android 10 access to
Android 11's protected Wireless Debugging feature, does not expose a
laptop as an Android USB hub, and does not substitute for ADB authorization.
scrcpy remains an optional external developer/forwarding tool, not a VR
runtime dependency.

User-requested, license-permitted emulator cores or content can be kept as
**app-private modules/data managed by this same APK**, without installing a
second Android package. The initial APK must remain lean: do not bundle the
full upstream emulator catalog, ROMs, BIOS images, desktop binaries, or
large optional assets. A playable core must still meet Android ABI,
provenance, licensing and runtime validation gates. Downloading a source
archive alone must not be labeled installation or playability.

**PC workload neutrality and interaction scope:** ReverieVR cannot assume
the laptop is idle or available for processing; users may be in calls, playing
music, transcoding large videos, transferring files, or using any other
application. Its purpose is not to know, enumerate, profile, or manage those
activities. External interoperability is limited to explicitly selected,
low-latency input control and consented display/mirroring, plus separately
approved read-only media access. Never make controller events wait for video
frames, file transfers, screen capture, or PC CPU/storage operations. Keep
transports asynchronous, queues bounded, and the local stereo/frame loop
isolated; on disconnection release external held inputs and preserve the
headset-only gaze controller.

Phone-to-PC display mirroring through user-started **scrcpy** is an external
Android/ADB workflow, not an always-on ReverieVR outgoing video stream.
The phone's internal Phone Screen mirror is local display only: never
transmit, record, inspect, OCR or analyze its pixels in ReverieVR. A
future PC-to-phone screen/window feed would need separate active consent,
narrow scope, secure pairing, local decode, a user-visible stop action and
headset performance proof. It is **not** an excuse to inspect PC windows,
applications, audio, or other activity. The governing rationale and
acceptance gates live in
`docs/records/ADR-0023-OPTIONAL-PC-INTEROPERABILITY-AND-RESOURCE-ISOLATION.md`.

All new integrations must pass a **phone-only acceptance check**: launch
offline, enter and exit VR without a controller, use gaze recovery, open
first-party native content and locally available media, and survive a
laptop/peripheral disconnect without terminating the shell. Check the
single APK install/update path separately from any optional component.

### 3.6 Project-wide native desktop and optional PC VR authority (2026-10-26)

**Authorized product expansion:** develop **the entire ReverieVR shell/hub and native/content module library** as independently launchable and usable desktop applications for Windows, Linux and macOS. Desktop users must be able to enter the familiar Home environment without a headset, use keyboard/mouse or gamepads, interact with functional global menus, customize avatars, launch actual installed native and classic games, operate DOS/media systems where permitted, persist progress, and acquire/update modules through appropriately packaged platform-native delivery. Desktop hardware capacity may enable enhanced render quality, but does not permit degradation of the original Galaxy S9 / Daydream path.

Desktop products run **locally**, without running the Android APK, requiring an Android phone, shipping an inert recreation of the menus, requiring Steam/SteamVR, or depending on network connectivity for installed core features. A desktop project containing only one game or a PC-to-phone control/video bridge does **not** fulfill this directive. The existing optional Windows Laptop Link utility and RV-0246 SteamVR phone-as-headset work remain separate, independently scoped features; their prior exclusion of desktop hosting applies only inside that peripheral project and is no longer a global prohibition.

**Required platforms:** Windows x86_64, Linux x86_64 and macOS Apple Silicon initially; Mac Intel support is separately evaluated. No platform may be called accepted without actual executable launch, hub interaction and representative module lifecycle validation on that OS. PC VR is a **provisional optional** OpenXR mode on supported Windows/Linux configurations with SteamVR as one possible active runtime, not a mandatory dependency or Steam-store integration. macOS SteamVR is not a delivery commitment.

Host services own OS-windowing, file access, display/HMD, controller transport, audio, install/update mechanics and safe exit; shared modules own game rules/state and use host-normalized interactions. Use genuine portable code, shaders/assets and versioned saves, while keeping platform-specific native binaries and installer packages. Device-specific functionality must be capability-gated, not presented as working by inert menus or invented telemetry. Cross-platform feature parity covers all supported first-party hub features and hosted games, subject to documented legal restrictions on third-party distribution.

The normative subsystem/architecture/acceptance plan is [Desktop Platform Expansion](DESKTOP_PLATFORM_EXPANSION.md). This direction is **planned**; writing this rule is not evidence desktop builds, SteamVR integration or cross-OS validation already exist.

## 4. Performance doctrine

ReverieVR is **performance-first and retro-first**.

The reference phone is finite thermal and battery hardware. VR workloads that chase unnecessary graphical fidelity until the handset overheats, throttles, or drops frames are considered defective.

Default design expectations:

- simple low-complexity geometry;
- deliberately restrained scene complexity;
- small/appropriate texture budgets;
- baked/static lighting where practical;
- minimal transparent overdraw;
- aggressive visibility culling;
- level-of-detail systems where useful;
- object/material batching where it actually reduces work;
- minimal post-processing;
- no expensive effect merely because the framework makes it easy to enable;
- dynamic render scale or quality fallback when required to preserve frame pacing;
- thermal behavior and sustained performance tested on the reference device.

**Art-direction authority (PlayStation-era primitive 3D + procedural-asset discipline):**
Environments, equipment, native games, and digital models should read as early
PlayStation-style primitive 3D: strong simple silhouettes, visibly faceted geometry,
small low-resolution or untextured materials, quantized/limited palettes, and
deliberately economical scene composition. Do not add modern photorealistic
materials, reflections, large texture packs, dynamic post-processing, or dense
meshes as routine detail passes. Preserve accurate controller alignment,
stereoscopy, readable text/UI, and robust input behavior over stylistic tricks.

Borrow the **procedural-content principle** associated with .kkrieger as
inspiration—not its engine, binary content, or any assumed performance claim:
store short deterministic recipes/seed/material tables when cheaper than baked
assets; generate immutable geometry and any truly needed tiny texture atlas once
at startup/GL context creation; upload/reuse those resources for both eyes; keep
the frame loop generation-free. Regenerate only after context loss/recipe changes.
A texture-free flat-color mesh is preferable to making a procedural texture
just for the sake of having one. Startup CPU time, APK size, VR frame pacing,
thermal load, and GPU uploads must be measured independently: generation can
make startup **slower** if the recipes are too complex. Never claim lower load
times or battery use without evidence.

A polished Doom/early-3D-scale visual language is acceptable. Visual complexity is allowed only when measured headroom supports it.

**Stable frame pacing, sensor latency, input latency, thermal sustainability, and legibility inside the headset outrank polygon count and graphical spectacle.**

## 4.1 Mandatory XR equal-access and comfort gate

ReverieVR must be operable by users who cannot distinguish common UI
hues or rely on two-handed/precision controller motion, hearing-only cues,
or unrestricted head/neck movement. Before broad/extended headset release,
perform the evidence-based acceptance matrix in
`docs/project/ACCESSIBILITY_AND_COMFORT.md` on Stage A, Stage B shell,
home environments, media, DOS host and every admitted first-party native
game. Color is never the only representation of a critical state;
protan/deutan/tritan, grayscale, low-vision, reduced-motion, hearing and
motor alternatives are design and device-test requirements. Document
limitations and failed passes, rather than asserting accessibility from
palette names or code inspection alone. Recovery must not require
uncomfortable prolonged head movement or a particular controller brand.

All controller layouts and user remapping follow
`docs/project/CONTROLLER_LAYOUTS_AND_BINDINGS.md`; the selected
physical controller remains a user choice, and a native game must never
take exclusive control of shell emergency recovery.

## 4.2 Android forward compatibility and open-source maintenance contract

ReverieVR is intended to remain a **performance-friendly, operationally forward-compatible, independently maintainable open-source Android application**, not merely an Android 10 / Galaxy S9 restoration artifact. The canonical deliverable remains one offline-capable phone-resident APK with capability-driven defaults; do not create per-device code/product forks or introduce required discontinued Google services, proprietary online activation or cloud dependencies.

Protect the legacy Galaxy S9/Android 10 VR reference while validating newer Android versions. The available Samsung Galaxy A14 5G (SM-S146VL), owner-confirmed Android 15, is the secondary *compatibility* reference, not proof of flagship-class VR performance; its gyroscope, viewer fit, APK install, native-game loading and stereo operation require physical validation. Do not mistake `minSdk`/`targetSdk` declarations for real-device acceptance or claim universal future-device support.

Treat OS/SDK upgrades, native ABI/page-size compatibility, sensors, graphics/display modes, controller/permission APIs, update/data persistence, lifecycle behavior and sustained frame pacing as maintainability obligations. Detect actual capabilities and provide honest, recoverable fallbacks. Frame-time consistency, thermal stability, legibility and long-term battery practicality outrank peak FPS and visual spectacle. Dependency provenance, build reproducibility, coherent modules and test evidence must make the open-source project maintainable by future contributors.

The detailed governing decision, test matrix and acceptance gates are in `docs/records/ADR-0024-ANDROID-FORWARD-COMPATIBILITY-AND-MAINTAINABILITY.md`. It extends rather than supersedes the existing performance-first doctrine in ADR-0001.

## 5. Non-goals unless explicitly promoted

The project does not initially promise:

- binary compatibility with every historical Daydream application;
- reimplementation of every Google cloud/backend service;
- redistribution of proprietary Google software/assets;
- 6DoF tracking from hardware that cannot provide it;
- support for every Android device;
- a public app store/cloud ecosystem replacement;
- modern high-end VR graphical fidelity.

Research does not silently expand product scope.

## 6. Authority hierarchy

Resolve truth in this order:

1. current explicit user instruction and granted scope;
2. platform safety, authorization, and repository permission boundaries;
3. this document;
4. `docs/project/EXECUTION_CONTRACT.md`;
5. verified repository state on `main`;
6. `docs/project/EXECUTION_STATE.md` and `docs/project/ACCEPTANCE_LEDGER.md`;
7. durable design/decision records;
8. `docs/project/APK_BUILD_ORDER.md`, `docs/project/BACKLOG.md`, and `CHANGELOG.md`;
9. conversational context, memory, prompts, or informal notes.

Lower layers cannot silently override higher verified authority.

## 7. Repository policy

- Canonical repository: `https://github.com/mrcalzon02/ReverieVR`.
- Canonical branch: `main`.
- Main-only development by default.
- No side branches, alternate implementations, replacement repositories, or PR workflow unless explicitly authorized.
- No force-push or published-history rewriting.
- No GitHub Actions unless explicitly authorized.
- Preserve unrelated user work.
- Reconcile concurrent changes before mutation.
- Prefer one coherent target or tightly coupled change set per commit.

## 8. Engineering rules

### Offline-first
Core headset/runtime operation must remain useful without an external service.

### Capability honesty
No hidden stubs, swallowed failures, no-op compatibility shims, dummy success responses, or simulated service availability. Clearly marked scaffolding is allowed only when it cannot be mistaken for working behavior and is tracked as incomplete.

### No inert user controls
Every user-facing button, switch, menu item, setting, or action must invoke implemented, observable behavior at the time it is exposed. Do not ship placebo controls that only persist state, display a label, or suggest a capability without changing actual application behavior. Planned controls remain in design/backlog documentation until their behavior is wired. If an inert control is discovered, remove the exposed control and its orphaned preference/state wiring unless the underlying behavior is implemented in the same scoped change.

### Responsive interaction feedback
Every actionable button/control must provide an immediate visible responsive state and audible activation feedback appropriate to its interaction surface. Touch buttons require clear pressed and disabled presentation; in-headset controls require clear focus/hover presentation before activation. A rejected or failed requested action must produce a clearly different failure sound from the normal activation sound, and failure feedback must not hide or replace the real error/status information. Feedback code is platform behavior and should be centralized rather than duplicated ad hoc per screen.

### Root-cause repair
Repair authoritative source and architecture rather than generated output or layered compensating hacks.

### Reference-first abstraction
Abstract hardware interfaces where useful, but do not over-generalize before the Galaxy S9 + Daydream View path works.

### Graceful degradation
Missing optional hardware/capabilities should disable only the dependent feature, explain the limitation, and preserve unrelated functionality.

### Inspectable local configuration
Headset parameters, render settings, controls, comfort settings, and compatibility decisions must be represented by local inspectable configuration/code with provenance.

### Recoverable mode transition
The 2D setup surface and 3D VR shell are distinct application modes with an explicit, testable transition. Failure to enter VR must return cleanly to a usable 2D state instead of leaving a black screen, crash loop, or unusable stereoscopic UI.

### Performance is correctness
Performance, thermal behavior, frame pacing, decoder behavior, tracking latency, and input latency are acceptance criteria.

### Living documentation is correctness
Normative developer documentation must remain true of the current implementation. When a documented function, service, utility, API contract, lifecycle rule, generated-content mechanism, or reusable platform behavior changes, update the corresponding documentation in the same scoped change. Known-stale documentation is a defect, not deferred cleanup. Native game work follows `docs/native/CONTRACT_INDEX.md` and must run the native documentation drift guard when applicable.

### Shell owns cross-module platform behavior
Global status, recovery, settings access, and common navigation live in the shell/platform layer. Hosted modules consume platform services and input actions instead of cloning or bypassing them.

### Quality-of-life features remain optional
Automatic HUD reveal/retract, gaze-triggered behavior, comfort helpers, and similar convenience systems require local toggles when they materially change presentation or interaction. Turning an enhancement off must leave a predictable usable baseline.

## 9. Provenance and reverse engineering

- Keep independently authored source separate from third-party artifacts.
- Do not commit proprietary Google binaries, applications, SDK payloads, firmware, keys, credentials, copyrighted assets, or other material without redistribution rights.
- User-supplied local artifacts for compatibility research stay outside distributable source unless licensing permits inclusion.
- Record provenance for imported open-source code, specifications, calibration data, or assets.
- Prefer public documentation, observable behavior, independently written compatibility code, and reproducible fixtures.
- Preserve required attribution/licenses.

## 10. Documentation roles

- `docs/project/INSTRUCTIONS.md` — stable doctrine.
- `docs/project/EXECUTION_CONTRACT.md` — execution/Git/validation rules.
- `docs/project/APK_BUILD_ORDER.md` — ordered product construction.
- `docs/project/BACKLOG.md` — eligible and deferred work.
- `docs/project/EXECUTION_STATE.md` — current verified operational position.
- `docs/project/ACCEPTANCE_LEDGER.md` — admission state.
- `docs/records/` — architecture, investigations, decisions.
- `docs/native/` — native game developer handbook, implemented API reference, and reusable generated-content standards.
- `CHANGELOG.md` — verified completed project changes.

Do not create competing trackers for the same authority role.

## 11. Decision discipline

Recorded rulings prevent drift but are not sacred. A better approach must identify the existing ruling, present the new evidence/tradeoff, and update the authoritative records if adopted.

## 12. Development-unit rule

Every implementation unit states:

- bounded target;
- authoritative files;
- dependencies;
- expected behavior;
- deterministic checks available now;
- device/runtime validation still required;
- acceptance state reached.

Planning alone is not completion.

## 13. Project identity

The project name is **ReverieVR**. It is not an official Google, Daydream, Samsung, or Meta product. Historical names may be used factually for compatibility targets/hardware.
