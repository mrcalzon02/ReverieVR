# ADR-0019 — Red Ledger Native Game Foundation

**Status:** accepted for bounded foundation and Development-only device admission  
**Date:** 2026-10-05  
**Scope:** RV-0610, with RV-0402 / RV-0600 / RV-0601 device gates still authoritative

## Decision

Begin the first full ReverieVR-native game track with **Between Deliveries: The Red Ledger VR**, following the implementation order already recorded in `NATIVE_GAME_PROJECT_POINTERS.md`.

This decision does **not** waive the native-host reference-device stop in ADR-0015. The first Red Ledger slice is deliberately split into two layers:

1. a platform-independent deterministic one-room bar/economy simulation plus a low-complexity GLES2 module implementation that can compile with the current native-module ABI; and
2. a later launchable interaction slice that is gated on an honest controller-pose/reach contract, native save services, and the still-pending Galaxy S9 native-module runtime/performance evidence.

The packaged Red Ledger library may now enter the trusted native-host allowlist solely to create an honest physical-test path. Normal visibility remains gated: Standard logging filters Red Ledger out of Native Apps, Development logging exposes a clearly marked `[DEV]` entry, and the Java runtime separately rejects a Red Ledger start unless the VR session itself was created in Development mode. This is test admission, not normal release admission.

## Opening-state simulation

The deterministic foundation models only the ground-floor opening pressure described by RV-GAME-01:

- day-based cash and debt state;
- beer stock, clean cups, dirty cups, serving and washing;
- four recurring patron archetypes with deliberately small drink-price/reputation differences;
- a tiny supplier catalogue containing beer and mismatched cups;
- protection-payment pressure;
- an inspection/fine condition tied to a visibly neglected bar state;
- a supply-interruption day that blocks purchases;
- a day-close ledger recording revenue, spending, fines, debt service, patrons, stock, reputation and pressure deltas.

The event sequence is deterministic for now so tests and early headset observation are reproducible. Content breadth, random event weighting, faction systems and campaign-scale progression remain out of scope.

## Native visual foundation

`libreverie_module_red_ledger.so` is a separate GLES2 shared-library target. Its first room deliberately stays below the pre-PSX production ceiling:

- one concrete room;
- simple bar/counter and working-tap silhouette;
- one stool;
- visible clean/dirty cup proxies;
- mattress/ledger props in the back area;
- one patron proxy at a time;
- a small diegetic event marker;
- a visibly flickering ceiling light;
- no PBR, shadows, large scene graph or modern-engine dependency.

The module restores host GL state after each eye render and owns no Android Activity, window, EGL context, Cardboard state or shell recovery action.

## Native interaction and persistence contract

ABI v1 remains the compatibility identity. The interaction/persistence slice
uses its existing structure-size convention to append fields and services
rather than introducing a parallel ABI:

- `ReverieNativeInputV1` appends a normalized world-space pointer ray plus a
  pointer-kind discriminator for tracked controller versus virtual gamepad;
- controller quaternion interpretation, yaw calibration, stale-pose rejection,
  controller anchor/emitter placement and virtual-gamepad aim remain owned by
  the ReverieVR shell;
- raw Android controller pose is not exposed to modules;
- `ReverieNativeHostV1` appends module-scoped save read/write callbacks;
- the host restricts save slots to safe filenames inside the active module's
  private app-data directory, enforces a 64 KiB payload ceiling and writes by
  temporary file plus atomic replacement;
- Red Ledger uses ray/AABB targets for the cup rack, tap, patron, payment,
  wash station, ledger, beer-order card, cup-order card, protection envelope
  and patron exit;
- the seated service transaction is explicit: take/return cup -> fill cup ->
  hand the drink to the patron -> collect payment. Cash and patron progression
  occur only when payment is collected;
- Select continues through the normalized hosted-native input bus as virtual
  joystick button 0. Red Ledger has a dedicated Select-only hosted profile, so
  Test Chamber touchpad locomotion is not exposed as an inert game control;
- Quick Menu input suppression and shell-owned Back/Home/recovery are unchanged.

Red Ledger state is serialized using an explicit version-2 160-byte
little-endian schema rather than native C++ object memory. It includes held
drink state and pending payment as well as the economic/day ledger. The decoder
validates magic/version, bounded values and service/day consistency; corrupt
data is rejected without replacing the fresh opening state. The v2 slot name is
distinct from the incompatible foundation draft.

The host now recognizes Red Ledger as trusted packaged code for Development-only
device testing. Standard-mode enumeration and runtime launch remain blocked.
Having a compiled Development entry is not evidence that its reach volumes,
controller alignment, held-cup placement or comfort are correct on the physical
headset. The Development build therefore renders a small contact cursor exactly
at the selected ray/AABB hit point. The module also normalizes the supplied
pointer direction defensively before hit testing; ABI v1 and shell ownership of
controller calibration remain unchanged.

## Verification boundary

Phone-test #52 completed successfully from commit
`48d4f2f1d22c4370bba5caea5a612d06ba2018b2`.

Its Red Ledger C++17 `-Wall -Wextra -Wpedantic -Werror` test covered clean-cup
pickup/return, fill, serve, explicit pending-payment collection, washing,
protection, day close, mid-transaction save/restore, corrupt-save rejection,
inspection, supply interruption and patron turn-away. The same run completed
the full unsigned Android/Java/JNI/NDK tree, persistently signed packages, APK
verification, artifact preservation and prerelease publication.

Prerelease `phone-test-52-1` contains
`ReverieVR-phone-test-52-48d4f2f.apk` with SHA-256
`e425419a612752611860674d55f728d6edb11d289c5980a22506527024140fbf`.
The APK gate continued to require `libreverie_module_red_ledger.so` in both
`armeabi-v7a` and `arm64-v8a`.

Phone-test #60 completed successfully from combined commit
`416e866a7c87cf7bb802920afa9f786d73bb1874`. It proves the Development-only
host admission, Standard-mode filtering, fail-closed Red Ledger runtime launch
gate and the dedicated Select-only hosted profile/JVM regression test through
the full unsigned and signed Android/Java/JNI/NDK pipeline. APK verification,
artifact preservation and prerelease publication also completed, alongside the
concurrent stereo-isolation repair.

Prerelease `phone-test-60-1` published headset APK
`ReverieVR-phone-test-60-416e866.apk` with SHA-256
`7a8aa42b47ff684a1393a244582198c9ed0ce3cd7578aa7da7291d72c3ed22ee`.

No claim is made yet for Red Ledger runtime loading on the physical S9,
controller-ray alignment, held-cup usability, stereo correctness, real save
durability, comfort, sustained frame pacing, battery/thermal safety or Galaxy
S9 acceptance.

## Next gate

Before the Development-only label/filter can be removed for normal visibility:

1. install the current Development-admission build on the Galaxy S9, set
   launcher logging to Development and launch the `[DEV]` Red Ledger entry;
2. exercise the Red Ledger controller ray on the real Daydream controller and
   tune its work-surface targets/reach from headset evidence;
3. verify Quick Menu, Home, Back and both recenter paths while Red Ledger is
   active;
4. verify Red Ledger state survives real exit/re-entry and an in-place APK
   update;
5. complete the 15-minute Galaxy S9 frame-pacing/battery/thermal pass.

Only after those gates should Red Ledger lose its Development-only filter and
`[DEV]` label and become a normal Native Apps entry.
