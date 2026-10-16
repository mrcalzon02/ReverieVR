# ADR-0024 — Performance-First Android Forward Compatibility and Open-Source Maintainability

**Status:** Accepted architectural direction; cross-device runtime validation pending  
**Date:** 2026-10-16  
**Authority:** `docs/project/INSTRUCTIONS.md`, §4.2; extends ADR-0001 and ADR-0007

## Context

ReverieVR began as an offline-first revival of Daydream-era phone VR, but its long-term value is as an efficient, inspectable, maintainable open-source Android VR application. Locking the implementation to a single aging phone or discontinued vendor service would undermine that goal. Requiring current flagship hardware would undermine the reuse goal. New Android releases change permissions, packaging, lifecycle rules and device capabilities even when an APK nominally installs.

The existing project uses an independent Cardboard-based stereo/head-tracking path, has no required Google Daydream Services dependency, and declares Android API 26 minimum with API 36 compile/target settings at this decision's adoption. These declarations are **not** evidence that all intervening or future Android releases/devices work.

## Decision

**One phone-resident, offline-first, performance-friendly ReverieVR APK shall adapt to supported Android versions and actual device capabilities, rather than splitting into per-phone product forks or requiring obsolete vendor services.** Keep the independently authored platform source readable, reproducible, documented and practical for outside contributors to maintain after original hardware and developers are unavailable.

1. **Preserve legacy support deliberately.** The Galaxy S9 with Android 10 and Daydream View/controller remains the primary VR behavior, comfort and performance reference. Do not casually increase `minSdk`, remove compatible ARM support, or upgrade a dependency in a way that breaks the reference path. A future minimum-version increase requires an explicit recorded compatibility decision, migration/fallback analysis and evidence, not silent drift.
2. **Support newer Android through normal platform APIs.** Maintain current supported build tooling and SDKs; audit behavior changes at each target-SDK upgrade. Use version-gated OS APIs and permission flows for Bluetooth, storage/document access, network access, package installation, audio, system UI, foreground/background lifecycle, and rendering. Do not require proprietary Google Daydream/VR Services, a companion phone, a computer, Play Services or cloud authentication for core functionality.
3. **Capability detection precedes device assumptions.** Detect OS API, installed ABI/native libraries, available sensors (particularly a real gyroscope), supported GL features, screen/display mode, usable refresh rates, memory/thermal signals, controllers, and viewer calibration before choosing modes. Distinguish features truly absent from initialization errors. Device-model profiles may provide tested defaults, never replace capability checks. Preserve a functional 2D Stage A and honest diagnostic explanation if stereo VR is unavailable; gaze/recovery continues wherever tracking is functional.
4. **Prefer stable delivery over peak throughput.** Optimize frame *interval consistency*, low tracking/input latency, comfort, legibility, power and sustained thermal behavior ahead of maximum FPS or fidelity. Select a display-supported pacing mode; never promise a fixed frame rate without device evidence. Favor conservative render scale, simple geometry, reused per-eye resources, bounded memory/allocation, low overdraw, and cost-controlled native modules. Optional enhanced effects and higher refresh modes must be conditional on measured headroom, not mandatory.
5. **Make performance adaptation explainable.** Measure and, where exposed by the OS, record actual refresh/display mode, callback frame intervals and P95/P99/jitter/long gaps, per-eye CPU submission, memory, thermal/battery signals, and render scale/quality decisions. Use hysteresis to avoid visible quality oscillation; do not infer GPU completion, display-present timing or missed vsync from CPU callback timing alone. The existing optional PERF overlay and diagnostic ZIP are the first instrumentation surfaces; keep them opt-in for display and locally controlled for export.
6. **Keep native code portable and verifiable.** Continue to ship the native libraries necessary for each supported ABI, particularly ARM64, and check binary loading, compatible page-size alignment/packaging (including third-party .so files), graphics state isolation, and the built-in DOS and native-game hosts. No dead compatibility shims, stub modules, fake success or per-model code duplication. Version shared native/module interfaces and preserve working callers with explicit migration steps.
7. **Maintain an open, replaceable dependency stack.** Prefer standards, small audited components, clear module boundaries, reproducible builds, exact dependency versions, recorded provenance/licenses, minimal external services and public documentation. Keep the third-party rights boundaries of the assembled APK distinct from the MIT license on independently authored code. Document maintenance and replacement plans for abandoned dependencies; no mandatory account, telemetry or online activation.
8. **Treat forward compatibility as an ongoing regression obligation.** Test installs/upgrades and data preservation, Stage A, sensor/VR preflight, stereo/cardboard geometry, controller/gaze recovery, native module and DOS loading, media/audio, pause/resume, and diagnostic/error paths on every available reference OS. New functionality must not break the old device simply because it works on a newer device; newer Android failures must not be hidden by successful builds.

## Available device matrix (not a certification)

| Device | OS | Role | Evidence status |
| --- | --- | --- | --- |
| Samsung Galaxy S9 | Android 10 | Legacy reference: Daydream View, controller, sustained VR/optics, low-end performance budget | Existing project test target; current-version acceptance still requires physical testing |
| Samsung Galaxy A14 5G (SM-S146VL) | Android 15, confirmed by device owner | Newer Android behavior: installation, permissions, BLE/HID, native loading, lifecycle, display adaptation, sustained efficiency | Hardware available; ReverieVR APK acceptance and gyroscope/VR compatibility **not yet verified** |
| Other Android 11–14, Android 16+ and future devices | Varies | Broader ecosystem portability goals | Untested unless individually entered in evidence ledger; no universal compatibility claim |

The A14 is newer but budget-class; it is **not** a modern flagship performance proxy. Its physical compatibility with a viewer and sensor/VR performance must be checked before claiming headset support. Testing on two phones cannot establish all Android devices or future OS releases.

## Acceptance and maintenance gates

- **Static/build:** preserve the declared legacy API minimum until a recorded decision; compile against the chosen supported toolchain; assemble/sign/inspect the APK; verify ARM native libraries and any required 16 KiB page-size compatibility; check permissions and API-level guarded paths. Keep checks runnable locally; do not introduce GitHub Actions by default.
- **Device functional:** install, update-over-update, retained local preferences/game saves/imports, offline Stage A, sensor/GL VR entry (when supported), calibrated stereo viewer behavior, controllers/gaze shell access, real DOS and native-game launch, media/audio, suspend/resume, clean recovery and failure diagnostics.
- **Device sustained:** log measured frame intervals and irregular gaps, thermal/battery trend, UI latency and memory during representative home/media/game workloads at tested refresh/pacing settings. Compare multiple sustained runs before accepting a visual-cost increase or claiming a performance win. Budgets and device-specific thresholds are determined from measurement, not invented here.
- **Version roll-forward:** repeat key gates on new OS versions/toolchains, including runtime permission, install/update, lifecycle and renderer changes, and record actual tested build/OS/ABI in `docs/project/ACCEPTANCE_LEDGER.md` and `docs/project/EXECUTION_STATE.md`. An unsupported capability must degrade precisely, without a black-screen trap.

## Alternatives rejected and consequences

- **S9-only Android build:** too fragile and dependent on legacy hardware.
- **High-end flagship minimum:** excludes the retro-reuse purpose and conceals performance regressions.
- **Separate Android-10 and modern-phone forks:** duplicates maintenance and risks divergent game saves, fixes and controls.
- **Trusting manifest/Gradle SDK declarations as proof:** cannot establish sensor behavior, native module loadability, VR comfort, permissions or thermal stability.

Consequently, compatibility work has continuing engineering cost, but a single adaptable, transparent source tree is more valuable than chasing maximum rendering quality on a particular generation. This ADR documents the **design and validation obligations**, not completed device support or a completed implementation.
