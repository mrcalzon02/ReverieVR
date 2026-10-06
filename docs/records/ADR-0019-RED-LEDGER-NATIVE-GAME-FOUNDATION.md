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

## Temporary development-input rule

ABI v1 currently exposes only movement plus primary/secondary button state. The hidden module may use those two actions as an internal development harness to advance serving/washing while validating simulation/render plumbing, but those bindings are **not** the product interaction design and are not grounds for exposing the module in VR Home.

The playable Red Ledger interaction contract must instead provide controller-oriented work-surface targeting/reach assistance suitable for a Daydream-class 3DoF controller. It must support real selection/use of the tap, cups, money/ledger and other bar objects without pretending a generic "press button to serve" path is hand interaction.

## Verification boundary

This foundation may claim only what has actually been observed:

- strict host-C++ compilation/tests for the simulation core;
- strict C++ syntax validation of the module against the ABI/GLES surface;
- source-level NDK target wiring.

It may **not** claim successful Android/NDK packaging, runtime loading, stereo correctness, controller usability, persistence, thermal safety or Galaxy S9 acceptance until those checks actually run.

## Next gate

Before the module can be allowlisted and shown to the user:

1. close the relevant RV-0402 native-module device/runtime gap on the Galaxy S9;
2. define the smallest backward-compatible native interaction/service extension needed for controller pose/reach and save state;
3. wire real bar-object selection/use through that contract;
4. add deterministic save/load coverage for economic state;
5. build/package both target ABIs and then perform the first headset interaction/thermal pass.

Only after those gates should Red Ledger become a visible Native Apps entry.
