# ReverieVR — sustained continuous development contract

Owner directive, 2026-11-24. Canonical repository: `mrcalzon02/ReverieVR`, branch `main`. Source baseline inspected: `2a62f02b9a46b08375972dea1be6fa741c88f8f6`. Existing project instructions, APK build order, native contracts, acceptance ledger, and individual design specifications remain authoritative. This is an executable work-selection contract, not evidence that any project is finished.

## Operating cycle
1. Recover the latest successful equivalent actual run (source, commands, labels, logs, artifacts); reuse it. Reconcile remote main, local changes, current implementation and newer records. The November 8 audit is historical; November 16 campaign evidence supersedes old save/registry findings. Never reimplement already-working functionality.
2. Select one unfinished gate from a track, highest dependency-ready blocker first; keep a stable project and gate ID. For a small model, read one gate, its source and only relevant canon. If a source file exceeds LCI limits, use bounded source excerpts or an explicitly approved smaller file; do not bypass limits.
3. Ask the literal acceptance question: does the real installed/compiled experience perform the specified action? If yes, record independently reproducible evidence for this revision; if no, reproduce with a real input path, inspect implementation, repair the cause, then rerun the same scenario. If unobservable, record blocked/unverified and exact recovery action, not a pass.
4. Work in bounded resumable attempts, prioritizing functional gameplay/apps over ornamental tests, documentation churn, or redesign. Keep the current gate open until acceptance is witnessed. Continue other ready tracks when an external dependency blocks one; retry transient failures with bounded backoff. Preserve every failed attempt and do not disable existing schedules.
5. Stage narrowly; no unrelated repositories, speculative rewrites, stand-in games, separate required APKs, new parallel architecture or artificial maximum registry count. Preserve prior user work. Single writer, main only, no force push. Test actual production behavior, not an alternate implementation or mocked success; do not weaken assertions or fake evidence.
6. Record source SHA, gate ID, input fixture/seed, execution layer, runner identity, exact command, exit and observed assertions, artifact paths/hashes, state before/after, and next action. Visual changes require real game framebuffer/model proof in existing gallery; simulated unit results cannot stand for device evidence. Model self-report and successful dispatch are not independent receipts.
7. Commit scoped verified changes; refresh remote main and read back the commit. Do not call proposal, syntax parse, workflow acknowledgment, skipped build or host-only test a completed release. Published APK must satisfy current release policy: build/launch/run and required updater/multiplayer failures block publication; visuals/style/balance are diagnostics. Development-completion gates are separate from release blockers.
8. Reevaluate completed gates when affected source changes; carry forward older evidence only with explicit unchanged-dependency justification. Use the existing acceptance ledger for final claims. Unverified is not failed; failed is not finished. Continue work on subsequent cycles until every relevant intended function and common completion condition passes.

## Common end-to-end gate (every applicable title/application)
- Fresh actual launch, understandable UI, every exposed control works with normal/denied feedback, correct input binding and release edges, no debug-only route.
- Complete intended game or app workflow, meaningful success/failure, replay/restart, legitimate progress and no softlocks.
- Pause/Quick Menu, return Home, controller disconnect/reconnect, Android recreation/GL loss and saved-state integrity; failed-write retry, no false acknowledgement, no duplicated transactions.
- Both-eye real render and legibility, licensed final assets/audio, explicit unsupported state, physical S9/Daydream controller/optics/audio and sustained thermal/frame evidence for Android. Desktop claim needs actual install/run on each claimed OS; separate XR qualification.
- APK remains one offline phone-resident application, existing updater/signer and registry architecture; no additional game install. Existing independent companion experiments are not current distribution.
- Capture actual per-gate evidence, source pin and readback; record final gate only after all applicable title gates and common criteria. No arbitrary cosmetic requirement is added to release CI.

## Evidence baseline and precedence
- [November 16 production replay and remaining milestones](../gallery/native-games/2026-11-16/campaigns-and-endings/README.md).
- [November 8 detailed intended completion tracks](../records/2026-11-08-native-games-release-audit.md); older implementation status is historical.
- [Integrated diagnostics at baseline](https://github.com/mrcalzon02/ReverieVR/actions/runs/45089933119) and [signed phone test](https://github.com/mrcalzon02/ReverieVR/actions/runs/45089933105) reported success; inspect jobs and artifacts before claiming publication or device acceptance.
- [Native identity matrix](../../modules/game-identity-matrix.json) defines registered games, not a fixed count. Plan coverage is diagnostic and must grow for new modules.
- [Machine-readable tracks](scd/tracks.json). Each guide has the baseline, precise implementation task, acceptance question, evidence and recovery rule. The first open gate must be reconciled with live source before changing it. No gate is certified merely by creation of this plan.

## Project guides
- [Ministry of Intelligence: Lantern Desk](scd/ministry-lantern-desk.md) — 5 ordered gates
- [Sitka](scd/sitka.md) — 5 ordered gates
- [Breakwater Battery](scd/breakwater-battery.md) — 4 ordered gates
- [Omega Front](scd/omega-front.md) — 4 ordered gates
- [Between Deliveries: The Red Ledger](scd/between-deliveries-red-ledger.md) — 4 ordered gates
- [Gumball Armada](scd/gumball-armada.md) — 5 ordered gates
- [Iron Sight: Forward Detachment](scd/iron-sight-forward-detachment.md) — 4 ordered gates
- [Guess the Door](scd/guess-the-door.md) — 3 ordered gates
- [Chess](scd/classic-chess.md) — 3 ordered gates
- [Go](scd/classic-go.md) — 3 ordered gates
- [Checkers](scd/classic-checkers.md) — 3 ordered gates
- [Pong](scd/classic-pong.md) — 3 ordered gates
- [Snake](scd/classic-snake.md) — 3 ordered gates
- [Reversi](scd/classic-reversi.md) — 3 ordered gates
- [Breakout](scd/classic-breakout.md) — 3 ordered gates
- [Chutes & Ladders](scd/classic-chutes-ladders.md) — 3 ordered gates
- [Test Chamber / VR Workshop](scd/procedural-test-chamber.md) — 3 ordered gates
- [Stage A setup / pairing / recovery](scd/stage-a-setup.md) — 3 ordered gates
- [Stage B Home, global menus, settings and comfort](scd/vr-home-shell.md) — 3 ordered gates
- [Native module host and bundled game library](scd/native-host-library.md) — 3 ordered gates
- [DOS and bundled DOS games](scd/dos-runtime.md) — 3 ordered gates
- [Local VR media and music](scd/media-player.md) — 3 ordered gates
- [Immersive OBJ 3D Model Viewer](scd/model-viewer.md) — 3 ordered gates
- [Reverie Photo Studio](scd/photo-studio.md) — 3 ordered gates
- [Reverie Video Studio](scd/video-studio.md) — 3 ordered gates
- [Reverie Model Workshop](scd/model-workshop.md) — 3 ordered gates
- [Reverie Game Workshop](scd/game-workshop.md) — 3 ordered gates
- [Desktop games workbench / internal editor](scd/desktop-workbench.md) — 3 ordered gates
- [Complete native desktop hub / OpenXR](scd/desktop-hub.md) — 3 ordered gates
- [Optional emulation and OpenRA](scd/emulator-catalog.md) — 4 ordered gates
- [Optional phone controller and Steam controller](scd/controller-companion.md) — 3 ordered gates
- [Laptop Link, local SMB and optional screen panel](scd/laptop-link.md) — 3 ordered gates
- [Updater, APK packaging and diagnostic intake](scd/updater-diagnostics.md) — 3 ordered gates
- [Shared native SDK, assets, audio and performance](scd/shared-asset-audio-performance.md) — 3 ordered gates
