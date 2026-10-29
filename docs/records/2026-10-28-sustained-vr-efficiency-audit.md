# ReverieVR — preventive power, thermal and comfort efficiency audit

**Date:** 2026-10-28 Alaska local. **Status:** research inventory plus tested host-side sampler; no Android optimization, new APK, or S9 temperature result is claimed. The user's phone was **mildly warm after roughly one hour**, not hot, throttling or shutting down. This is preventive engineering. Source baseline: main at cdd3b85583020bffaa332f9b1c1e04c6a1a25f30.

## Principles and existing code

Apply the repository's existing [performance-first VR doctrine](ADR-0001-PERFORMANCE-FIRST-RETRO-VR.md), compact procedural-asset guidance in [project instructions](../project/INSTRUCTIONS.md), [pacing telemetry](PERFORMANCE-PACING-TELEMETRY.md), and [optical composition contract](ADR-0013-RETRO-FRAMEBUFFER-AND-VR-COMPOSITION.md). The .kkrieger-inspired goal is to store compact recipes where appropriate, generate *once* and upload/reuse immutable meshes/materials across both eyes. Binary size, first-start generation, draw calls, CPU demand, GPU bandwidth, and battery expenditure are **different** measurements.

Source-reviewed implemented paths: Cardboard GLES2 stereo via VrActivity; onNewFrame gets head view and runs locomotion/IK once per callback; onDrawEye renders two independent eyes; Home uses a centered 73.8%-of-eye *presentation viewport*, while native games/model/video modes use full eye and fullscreen DOS has a separate 70% comfort inset. **Viewport inset alone is not a lower-resolution framebuffer.** Existing performance HUD records callback FPS, frame P95/P99/jitter, per-eye GL thread CPU-submission timing and Android battery temperature/thermal status. No reliable GPU completion or display present metrics; battery temperature is not CPU/GPU junction temperature. Device warnings at battery 42/45°C are informational, not an automatic thermal governor. Activity onPause closes/pause-releases relevant Cardboard, DOS, video, native and audio paths, pending physical verification.

**Do not** halve headset presentation to 30 FPS to save power. In the absence of verified timewarp/reprojection, preserve fresh low-latency head-pose-aligned 60Hz stereo wherever physically sustainable. Game *simulation* may run at a fixed 30 Hz and interpolate object state into 60Hz headset views; native games must be tested individually. Avoid interpolating head orientation through slow filters, synthetic motion blur, ghost frames, or claiming Cardboard already performs Daydream-style asynchronous timewarp.

## Ranked opportunity inventory

| ID | Priority | Candidate; not implemented unless stated | Proof before acceptance |
| --- | --- | --- | --- |
| EFF-01 | P0 | **Implemented sampler:** scripts/perf/s9-session-sampler.py samples Android battery %, temperature, voltage, charging, thermal status and whether main app process exists. | Offline unit tests, then **real** 10/30/60 min S9 runs; charging-vs-unplugged clearly separated. |
| EFF-02 | P0 | GPU/display presentation telemetry capability probe (existing callback FPS is not physical present). | Do not fabricate compositor metrics or sleep in the head renderer; show N/A if unsupported. |
| EFF-03 | P1 | Actual offscreen stereo scene render scaling at baseline 100%, trial 90%, optionally 85/80% **linear dimensions**. 90% means 81% scene pixels, *not* guaranteed 19% GPU/battery saving. | Preserve full-size distortion, HUD, optics, IPD and the separate existing comfort inset. A/B test S9 text and motion. |
| EFF-04 | P1 | Stable 30Hz game simulation/interpolated world transforms when viable, distinct from 60Hz head view. | No head tracking latency, double interpolation, input stutter or unsynchronized physics. |
| EFF-05 | P1 | Reuse immutable VBO/IBO and procedural geometry; create once and share per eye. | Count allocations and uploads; evaluate startup time vs runtime gain and compressed APK size independently. |
| EFF-06 | P1 | Batch matching flat-color materials/static geometry; reduce state changes and draw calls. | Profile actual CPU/GPU savings and verify culling, depth, mirror/wardrobe and controller visibility. |
| EFF-07 | P1 | Size-aware texture atlas/overdraw audit for Home, HUD, each native game, alpha vegetation and video. | Preserve UI readability; compare alpha overdraw and upload frequency without changing active scenes' semantics. |
| EFF-08 | P1 | Distance/frustum/occlusion LOD for costly new Sitka, Breakwater and Omega environments. | Keep player-relevant entities, critical routes and interaction targets visible. |
| EFF-09 | P1 | Dirty-only UI/canvas/texture upload; avoid unnecessary every-frame update/polling. | Controller hover and Quick Menu remain instantly responsive; neither eye lags. |
| EFF-10 | P1 | Pause unused module simulations, screen capture, offline media and expensive environment animation. | Verify each activity transitions/resumes with no audio/input/save loss; never disable connected BLE controller. |
| EFF-11 | P1 | Per-mode individual CPU/GPU/audio budgets. | Home White Room/Forest/Beach, DOS, Native, Video, Model, Wardrobe, mirror and sample games measured separately. |
| EFF-12 | P2 | Thermally aware *scene quality* hysteresis after real device baseline. | Small, stable, reversible quality steps, no repeated up/down oscillation or emergency-only optimization. |
| EFF-13 | P2 | Capability-probe Android sustained performance mode. | Supported hardware only; compare sustained 60-minute frame pacing; may worsen peaks and is not guaranteed power saving. |
| EFF-14 | P2 | Optional user-approved Do Not Disturb VR session mode. | Obtain Android authorization, respect priority interruptions and manually changed settings; restore only own changes. |
| EFF-15 | P2 | Brightness and optional radio/background activity guidance. | Never turn Bluetooth/location off for functioning controller, silently disrupt connectivity or require airplane mode. |
| EFF-16 | P2 | Asset/audio compression and caching, voice/ambience playback lifetime. | Runtime resource usage distinguished from APK byte count; avoid new decompression burden per frame. |
| EFF-17 | P2 | Review per-eye GL state reads and preferences lookups; use cached source state if safe. | No GL state pollution, incorrect scissor, hidden stale setting or duplicate-eye regressions. |
| EFF-18 | P3 research | Late pose sample / limited rotational reprojection experiments. | Two-eye head-motion-to-photon latency and artifacts measured. Not enabled by default merely to claim smoothing. |

## Measurement matrix and actual evidence

Do not require a hot phone to test. Stop if Android severe/critical status occurs or user discomfort develops. Ensure consistent room temperature, brightness, charge starting band, and comparable active game/action. First establish **baseline** before touching rendering quality.

**Runs to perform on actual Galaxy S9** (not yet executed):

1. R0: stationary VR Home White Room / no controller interaction, 10 minutes.
2. R1: White Room normal head turns and controller UI, 30 minutes.
3. R2: Forest and Beach environments separately, each 30 minutes.
4. R3: DOS fullscreen and DOS windowed separately, each 30 minutes.
5. R4: Native Breakwater, Omega, Gumball and Sitka (where actually runnable), each 30 minutes.
6. R5: Locally decoded VR video, 30 minutes.
7. R6: Repeat worst sustainable workload for 60 minutes *unplugged*, under the same conditions.
8. R7: After real render scaling implementation, A/B each at 100% vs 90%, only extend to 85% if visually comfortable.
9. R8: After world-update interpolation implementation, compare p95/p99/frame misses/latency symptom reports against baseline.
10. R9: Controller disconnect/reconnect, pause/resume, emergency gaze menu, stereo symmetry, optical fit and in-place updater after every renderer change.

### How to run the sampler

Requires Python 3, installed Android platform-tools (\`adb\`), USB debugging authorized, exactly one connected handset, ReverieVR already launched. It does not launch or alter the application:

\`\`\`sh
python3 scripts/perf/test_s9_session_sampler.py
python3 scripts/perf/s9-session-sampler.py --label home-baseline --minutes 60 --interval 60
\`\`\`

Output: local CSV time series and JSON summary under \`validation/vr-perf/\`. Do not commit captured logs or device identifiers. Sampling over USB normally charges the phone; when charging is detected, the script warns. That run is **not** comparable to unplugged battery endurance. For unplugged capture, use an already authorized safe Wi-Fi ADB connection if available, with the same conditions, or use the in-app periodic PERF report/diagnostic export and hand-record battery. No Wi-Fi connection is a project requirement.

**Sampler limitations:** shows Android battery sensor temperature, not chipset/GPU die temperature; does not measure watts, true display present, GPU completion, motion-to-photon latency, or user comfort. Device builds and physical tests remain gates in [APK build order](../project/APK_BUILD_ORDER.md).

### QA evidence read during this audit

- Existing latest-source integrated-game GitHub CI run 40795941586: **8/8 jobs passed** at the baseline commit. This is C++ source functional coverage, not VR temperature evidence.
- Existing signed phone-test workflow run 40795941309 completed successfully; it does **not** establish new efficiency work or physical headset acceptance.
- Existing Android emulator hub runs 40795250325 and 40795941148 each had **3/12 tests fail**, recurring Test Chamber module creation/menu capture and Omega Front battle-start sound-cue assertion. These require separate root-cause repair before advertising full hub integration acceptance. Re-running unchanged emulator workflow without a fix is not useful thermal evidence.
- A locally executed host-side sampler suite passed **12/12 offline tests**: battery values/units, missing/invalid/negative temperature, non-interference with phone settings, thermal status/unavailability, one-sample CSV/JSON smoke, and missing/multiple-device rejection. A real Galaxy S9 run, production APK build and thermal savings measurement were **not** performed in this audit.

## Preferred implementation order

First real handset baselines; then low-risk reused geometry, dirty-driven textures, redundant work reduction and bounded module lifecycle improvements; then actual framebuffer resolution experiments; then optional fixed-step simulation; only afterward thermal governors or reprojection research. Every change must include rollback path, before/after evidence, device comfort, two-eye parity, and existing shell/controller recovery checks. **Less unnecessary work is the objective; lower FPS is not the objective.**
