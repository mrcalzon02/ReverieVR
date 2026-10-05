# ReverieVR Execution State

**Role:** durable operational state  
**Updated:** 2026-10-04 (America/Sitka)

## Authority

- Repository: `mrcalzon02/ReverieVR`
- Branch: `main`
- Current verified remote implementation HEAD: `a4b4db3679fc768c4b32e40c2efb04183bf73056`
- Stage A implementation commits:
  - `64d00d48d49ec75fcd95f4c29d76d21970fdf070` — native Android Stage A setup menu
  - `a4b4db3679fc768c4b32e40c2efb04183bf73056` — controller Pair / Sync and Test actions exposed in the setup UI
- Remote readback confirmed the Android project, manifest, Java source, XML layout/resources, build instructions, and ADR-0003 are present on `main`.
- Current verified remote bookkeeping HEAD before this shell-intent capture: `41dfe89bd2675c2e7171edb9558bc7bf5c7479fc`.

## Last completed target

**RV-0001 — Governance/bootstrap adoption**

Acceptance: **static accepted**.

## Current implementation state

### RV-0003 — Android APK skeleton

State: **draft**

Implemented:

- Gradle Android application structure;
- application ID `io.github.mrcalzon02.reverievr`;
- minimum SDK 26;
- compile/target SDK 36;
- Java 17 source;
- no AndroidX/Compose/game-engine dependency;
- no Internet permission;
- documented local build path.

Remaining gate:

- generate/verify standard Gradle wrapper;
- run `:app:assembleDebug`;
- inspect resulting APK;
- install and launch on the Galaxy S9.

### RV-0090 — 2D touchscreen boot/setup surface

State: **draft**

Implemented source:

- conventional portrait Android launch screen;
- device/model/API reporting;
- real phone battery percentage/bar;
- honest unknown controller battery state;
- visible Pair / Sync and Test Controller actions, disabled until the controller stack exists;
- live shortcut to Android Bluetooth settings;
- persisted QoL toggles for VR battery HUD, look-up reveal, numeric percentages, and retro performance mode;
- local settings reset/recovery control;
- Enter VR visibly disabled until Stage B exists.

Remaining gate:

- successful APK build;
- install/launch and touchscreen interaction test on reference hardware.

## Active architecture target

**RV-0002 — Framework/runtime/controller-stack selection**

Stage A is now deliberately independent of this decision. RV-0002 remains responsible for:

- Daydream controller discovery/pairing/input/battery;
- stereoscopic rendering;
- head tracking;
- 2D-to-VR transition;
- recovery from Stage B back to Stage A.

## Captured Stage B shell contract

The previously implemented Stage A preferences for **VR battery HUD**, **look-up reveal**, and **numeric battery percentages** are now tied to an explicit Stage B architecture requirement rather than remaining orphaned UI toggles.

Stage B is a persistent platform shell around the home, video player, and hosted games/modules. The shell owns global navigation/recovery, global settings access, input policy, diagnostics/status access, and the power/status HUD.

The reference power HUD uses two compact upper-right percentage/progress indicators: handset battery and the currently bound controller battery. If controller battery telemetry is unavailable, the UI reports an unavailable/unknown state rather than manufacturing a value.

The stored look-up-reveal preference controls an optional adaptive presentation: normal forward viewing may retract the status HUD, while a deliberate upward look reveals/drops it into comfortable view. Turning this feature off must retain a predictable persistent/manual presentation. Automatic quality-of-life behaviors follow the same opt-out principle.

## Captured user optical calibration contract

ReverieVR must support software user-IPD/alignment calibration because the Daydream View has fixed physical lenses.

The project now explicitly separates:

- **physical viewer geometry** — fixed lens separation/distortion/viewer parameters;
- **user eye geometry** — wearer IPD and user-specific rendered-eye alignment.

Cardboard's current source exposes a physical `inter_lens_distance` and uses it to construct eye-from-head matrices. ReverieVR will retain that value as viewer hardware data and apply user calibration as a separate transform layer rather than corrupting the viewer profile.

RV-0206 now owns live in-headset calibration, persistent user IPD, bounded per-eye correction, binocular test targets, and safe reset behavior. Exact adjustment limits remain a reference-device validation decision rather than an invented desktop assumption.

## Captured first-VR-run onboarding contract

On the first successful Stage A -> Stage B transition, ReverieVR enters a **versioned in-headset setup wizard** before the normal home space unless the current onboarding version was completed or explicitly deferred.

The wizard combines teaching with real configuration:

- neutral seated posture and recenter;
- controller orientation/input familiarization;
- physical viewer-profile confirmation;
- user IPD and live per-eye optical alignment;
- UI/text readability scale;
- battery/status HUD presentation and look-up behavior;
- comfort defaults;
- audio baseline;
- performance/thermal preference;
- summary, recovery, and re-entry instructions.

Each page must have a safe default and must not trap the user. Progress is resumable. Settings exposes both **Run VR Setup Again** and direct individual calibration tools. The setup flow is versioned so a future new critical calibration can be introduced without replaying unrelated completed steps.

## Stage A update system

RV-0093 now has a draft implementation.

The Stage A menu:

- optionally checks for updates at launch;
- can be checked manually;
- queries only `https://api.github.com/repos/mrcalzon02/ReverieVR/releases/latest`;
- treats the absence of any published release as a normal state;
- requires a newer release and a trusted GitHub-hosted APK asset before enabling Update;
- shows release notes and gives the user **Update / Not now**;
- never silently installs;
- uses Android Download Manager and Android's package installer;
- verifies GitHub's published SHA-256 asset digest when one is present;
- leaves core VR operation offline-capable when update checks are disabled or unavailable.

The repository currently has **no published GitHub Releases**, so the expected live result today is "No published ReverieVR release exists yet."

## VR quality-of-life research outcome

The platform should treat user preferences as **system-level defaults** rather than forcing repetitive setup per module.

High-priority requirements now include:

- remembered comfort defaults;
- universal recenter/seated-height recovery;
- readable scalable/high-contrast UI;
- captions and visual alternatives to critical audio;
- one-controller/remappable input and optional gaze/dwell shell fallback;
- reduced-motion behavior;
- controller drift/deadzone/sensitivity calibration and reconnect recovery;
- a universal quick-access panel;
- nonblocking notifications that avoid routine center-screen interruption;
- optional session/break reminders;
- research-only real-world camera peek, clearly distinguished from true passthrough.

Automatic convenience behaviors remain user-toggleable unless disabling them would destroy recoverability.

## Environment limitation observed

The current execution environment has Java but does not have Gradle, Android SDK/build tools, or ADB. Therefore no APK build or device/runtime claim was made.

## Next exact action

From a verified JDK 17 + Android SDK 36 + Gradle 9.6.1 environment:

1. generate the Gradle 9.6.1 wrapper;
2. run `./gradlew :app:assembleDebug`;
3. inspect the APK;
4. install on the Galaxy S9;
5. validate Stage A launch, persistence, battery display, Bluetooth shortcut, reset behavior, and disabled readiness gates.

In parallel, RV-0002 controller/runtime research may proceed without changing the Stage A UI boundary.
