# ADR-0019 — Red Ledger Native Game Foundation

**Status:** accepted for bounded foundation implementation  
**Date:** 2026-10-05  
**Scope:** RV-0610, with RV-0402 / RV-0600 / RV-0601 device gates still authoritative

## Decision

Begin the first full ReverieVR-native game track with **Between Deliveries: The Red Ledger VR**, following the implementation order already recorded in `NATIVE_GAME_PROJECT_POINTERS.md`.

This decision does **not** waive the native-host reference-device stop in ADR-0015. The first Red Ledger slice is deliberately split into two layers:

1. a platform-independent deterministic one-room bar/economy simulation plus a low-complexity GLES2 module implementation that can compile with the current native-module ABI; and
2. a later launchable interaction slice that is gated on an honest controller-pose/reach contract, native save services, and the still-pending Galaxy S9 native-module runtime/performance evidence.

The Red Ledger library is therefore built as a packaged native target but is **not added to the host's compile-time built-in allowlist yet**. It must not appear as a runnable VR Home item until the required interaction path exists. This preserves the project's rule that visible controls and launch targets must reach real behavior rather than placeholder or button-only approximations.

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
- Red Ledger uses ray/AABB targets for its tap, wash station, ledger,
  beer-order card, cup-order card and protection envelope;
- Select continues through the normalized hosted-native binding profile and
  activates only the currently reached target;
- Quick Menu input suppression and shell-owned Back/Home/recovery are unchanged.

Red Ledger state is serialized using an explicit versioned 152-byte
little-endian schema rather than native C++ object memory. The decoder validates
magic/version, bounded values and deterministic day/event consistency; corrupt
data is rejected without replacing the fresh opening state.

The module is still **not** in the host's compile-time built-in allowlist.
Having a compiled interaction path is not evidence that its reach volumes,
controller alignment or comfort are correct on the physical headset.

## Verification boundary

Observed static/package evidence now includes:

- phone-test #50's Red Ledger C++17 `-Werror` simulation test, including
  save/restore and corrupt-save rejection;
- successful full unsigned Android/Java/JNI/NDK assembly;
- successful persistently signed phone-test package assembly;
- APK inspection finding `libreverie_module_red_ledger.so` in both
  `armeabi-v7a` and `arm64-v8a`;
- staged phone-test artifact and published prerelease `phone-test-50-1` from
  commit `2e4d14e9e31dde1e1c674049b15dffb181dbcbaa`.

Run #50 was subsequently marked cancelled when newer phone-test #51 started
under the repository's latest-build-wins concurrency policy. Its relevant
Red Ledger/build/package/verify/release steps had already completed
successfully and the release was published, so those completed outputs remain
static/package evidence. They are not device evidence.

No claim is made yet for runtime loading of Red Ledger, physical ray alignment,
stereo correctness, real save durability, comfort, sustained frame pacing,
battery/thermal safety or Galaxy S9 acceptance.

## Next gate

Before the module can be allowlisted and shown to the user:

1. close the relevant RV-0402 Test Chamber/native-host runtime and lifecycle
   gap on the Galaxy S9;
2. exercise the Red Ledger controller ray on the real Daydream controller and
   tune its work-surface targets/reach from headset evidence;
3. verify Quick Menu, Home, Back and both recenter paths while Red Ledger is
   active;
4. verify Red Ledger state survives real exit/re-entry and an in-place APK
   update;
5. complete the 15-minute Galaxy S9 frame-pacing/battery/thermal pass.

Only after those gates should Red Ledger enter the trusted native allowlist and
become a visible Native Apps entry.
