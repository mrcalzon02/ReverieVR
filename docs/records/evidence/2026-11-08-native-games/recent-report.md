# ReverieVR recent-games release audit

Audit date: 2026-11-08. Source baseline: `12d4018c90e56a3767b8ffc58a2e5394440caec1`. Targeted source was unchanged when HEAD advanced to `8a5196aa97a73cdae1393e27529bd51914fc1b0f` during the parent audit; that advance is the parent's checkpoint work. All probe files and logs here are scratch-only. No production code was edited by this audit agent.

## Decision

None of these four projects can conservatively be called release ready. Breakwater and Guess the Door have independently reproduced persistence/interaction defects, not simply missing paperwork. Sitka contains a substantial and tested collection of systems but is not an admitted, launchable native game. Test Chamber is a useful integrated development and authoring utility, with more careful save handling than the two games, but needs recovery and presentation acceptance before a stable utility release.

| Project | Actual admission | Current usable scope | Conservative status |
|---|---|---|---|
| Breakwater Battery | Native registry, development-only | Campaign/shops/weapons/transport/infantry/armor/aircraft with a new coastal renderer | Integrated alpha; incomplete requested rebuild; save safety blocks release |
| Guess the Door | Native registry, development-only | Twelve-room deterministic feature-clue maze with slow transitions and retry | Playable prototype; restart defect, no continuity, incomplete art/voice |
| Sitka | Gallery project inspector only; absent from native registry and Android.mk | Source-only terrain/walking/fishing/economy/boat systems; synthetic integration fixtures | Integration-stage project, not a playable APK title |
| Test Chamber | Native registry, development-only | Material/light/audio showcase, avatar, 30-piece scene and three-slot VR primitive workshop | Development utility alpha; do not assess it as a campaign game |

Registry evidence: `app/src/main/java/io/github/mrcalzon02/reverievr/GameModuleRegistry.java:27-28,37-38,55-56`; Sitka admission statement `docs/project/SITKA.md:3-5`; Guess build entry `app/src/main/jni/Android.mk:149-150`. Parent independently audits complete host/package identity.

## Tests and evidence actually obtained

The immutable command/status ledger is `results.json`, with one raw log per invocation. Commands were executed from the repository root. No Galaxy S9 or Daydream controller was available to this agent.

| Test | Result | What it establishes / what it does not |
|---|---|---|
| `bash scripts/verify-guess-the-door.sh` | exit 0 | Actual rule header, 80 seeds × 128 room indexes; determinism, exactly one physical-feature match, progression, fall and victory. Does not call module Update or persistence. |
| `python3 scripts/sitka/verify_gameplay.py` | exit 0; 34 CPU programs | Source-system rules, safety, navigation/boat math, fishing/progression and fixture checks. Does not create a packaged world. |
| Three extra Sitka module/GLES executables using the script's default strict compiler flags | all exit 0 | Actual Update walk-to-desk/sale/token/upgrade/save/retry; actual two-eye licensed text and terrain render; GL-resource recreation. Synthetic map/water/desk fixtures bypass packaged Create. |
| `bash scripts/verify-breakwater-visuals.sh` | exit 0 | 25 game/content checks plus asset scope. Clang unavailable and skipped. Despite its name, this suite is primarily geometry/rules/asset checks, not a framebuffer observation. |
| Full `breakwater_campaign_test.cpp` executable | exit 0 | Broad actual campaign regression: transport/landing/upgrades/ballistics/armor/saves, etc. This is not a human full-campaign playthrough. |
| `bash scripts/verify-breakwater-world-gl.sh` | exit 0 | Real Mesa GLES: 4/4 horizons, 16/16 materials, 5/5 models, both eyes at all three stations, guide-on/off pixel differences, immutable static coast, render/pick parity and destroyed/recreated EGL context. Mixed fleet is a controlled fixture. |
| Audit copy of that Breakwater EGL fixture preserving both eyes | exit 0 | Six actual station/eye framebuffers plus console/rack/shore views. Only the include path and save-both-eyes instrumentation differ. |
| `reverie_vr_workshop_test`, `reverie_scene_document_test`, `reverie_scene_asset_scope_test` | all exit 0 | Primitive editing/undo/save slots/corrupt rejection; strict scene grammar/safety/asset scope. |
| `reverie_vr_project_motion_test`, `reverie_vr_project_lighting_test`, `reverie_vr_project_sound_test` | all exit 0 | Spin/bob/pulse, format upgrades, scene-motion-light-sound bundle integrity, failed-save retention, undo, cue/rate/distance rules. Not audible handset acceptance. |
| Audit `guess_module_probe.cpp` | exit 1: two release expectations violated | Includes production module, plays actual Update through twelve rooms, then demonstrates double-action new-seed click. Also measures zero save calls and lost seed/room after module recreation. |
| Audit `breakwater_save_probe.cpp` | exit 1: release expectations violated | Drives production Interact menu paths with host read/write failures; proves success feedback after failed write, no pause retry, and replacement of an unreadable slot. |

### Graphics provenance and compile finding

Real driver string: `OpenGL ES 3.2 Mesa 25.0.7-2+deb13u1`, using surfaceless software rendering. Root provisioned Debian EGL/GLES headers/libraries in scratch. `CPATH`, `LIBRARY_PATH`, `LD_LIBRARY_PATH`, `EGL_PLATFORM=surfaceless`, and `LIBGL_ALWAYS_SOFTWARE=1` are recorded in the ledger. No fake GL implementation was used.

An additional optimized GCC experiment (`-O1 -Werror`) failed to compile the three Sitka integration executables because `reverie_native_gl_utils.h:27-29` produces `-Wformat-truncation` for bounded shader diagnostics. Repeating with the **existing script's actual flags**, without `-O1`, compiled and passed all three. Keep this distinction: optimized strict desktop warning failure, not a failing gameplay assertion and not evidence that Android Clang compilation fails. Logs for both attempts are retained.

The parent recovered an authentic Android artifact from run `42417911507`, source `c10ddd72cfd5150646c855f25e44955490d233c7`; it is baseline's parent and parent reports thirteen Android integration tests passing. I visually inspected its Guess and Test Chamber boot PNGs. Those frames supplement the local tests but do not certify the baseline's Sitka code or handset playability.

## 1. Breakwater Battery

### What is implemented

The simulation has four locations—Shingle Bay, Iron Quay, Blackcap Island, Stormwatch Point—four difficulties, day/night schedules, five selected weapons, upgrades, SAM/artillery/airstrike/support systems and differentiated transport, infantry, armor and aircraft threats. Four locations are explicit in `breakwater_campaign.h:183-184` and `breakwater_campaign.cpp:15-18`; do not confuse the three player stations with three campaign theaters. Wave/campaign transitions and success/defeat cues are simulation-owned (`breakwater_module.cpp:1924-1943`). Menu geometry and hit regions share `breakwater_menu_layout.h`, and reset requires a second deliberate press (`breakwater_module.cpp:442-462`).

The active coastal rebuild is materially beyond the older miniature-beach descriptions: roughly 114m approach, 19m battery elevation, broad ocean and bounded distant rendering. Source separates simulation coordinates from visible placement and inverse hit transforms. The November 6 record explicitly leaves weapon receivers/mounts/pivots/reload/recoil/UV work, invasion models, physically continuous unloading, speed/range tuning and campaign shoreline detail unfinished (`docs/records/2026-11-06-breakwater-coastal-rebuild-foundation.md:11-19,33`). Current starts are around 600m; planned 1–3km starts have not been implemented. These are intended-function gaps, not optional polish to silently discard.

### Reproduced save faults

**BW-01 — unreadable slot can be overwritten as a new campaign.** `ReadSlotState` maps every result except a valid exact-size decoded save to `false` (`breakwater_module.cpp:322-333`). Slot selection treats that result as an empty slot and replaces the current campaign with a fresh default (`380-399`); selecting difficulty then writes it (`420-437`). The audit injected `REVERIE_NATIVE_SAVE_ERROR`, selected the displayed slot and a difficulty, and observed a fresh 80-byte replacement write. Existing data may still be recoverable when a read fails; this path destroys that distinction. Release blocker.

**BW-02 — failed writes report success and are not retried on pause.** `SaveState` returns a meaningful boolean (`335-342`), but summary/difficulty/purchase paths discard it (`411`, `436`, `475-480`). The real menu probe observed phase Shop, ACTIVATION feedback, one failed write, zero persisted bytes. Calling production Pause did not make another write. Pause only clears interaction effects (`1788-1805`); Destroy deletes state (`1666`). Release blocker.

**BW-03 — wave-clear checkpoint is delayed until summary action.** On Combat→Shop, Update displays the result but does not save (`1927-1943`). The save happens later when a summary action is selected (`402-419`). A process exit while the earned result is visible can therefore reopen the previous checkpoint. This specific interruption was established by source tracing, not a killed-process device test.

The 80-byte format intentionally saves **Shop checkpoints**, not mid-combat enemies/projectiles/clocks (`breakwater_campaign.cpp:204-275`). Conservative completion must state whether interruption restarts the wave or resumes it exactly. Save-at-wave-boundary behavior can be a valid contract, but it must be automatic, durable and visible. The current missing/invalid/I/O error conflation is not a valid contract.

### Readability and interaction

The real local framebuffer `captures/breakwater-console.png` shows `HULL HIT -8` crossing `DOUBLE CLICK A SHORE POST` and `WEAPONS: LEFT RACK / PAD`. Source places the always-visible text at `breakwater_module.cpp:1404-1407` and the transient damage banner around `1491-1499`. This is a concrete overlapping-HUD defect, reproduced through the existing renderer fixture. It is stronger evidence than text-length arithmetic.

The forward frame shows very small distant unit silhouettes despite a clear cyan trajectory and target diamond. This is a necessary target-recognition/timing test at the new physical scale, not yet a proven device defect. The physical left rack is readable in its fixture view. Six local eye frames visibly contain the trajectory; numerical guide-pixel counts range from 2255 to 2313. This establishes renderer output, not headset optical clarity, comfortable head turns or a player's ability to finish a wave.

### Completion track

1. **Persistence hardening first:** distinguish absent/invalid/I/O-failed saves; preserve unreadable bytes; add recoverable error state and explicit replacement/reset. Add dirty state, failure indication and retry; save immediately at completed waves and successful transactions. Checkpoint campaign victory. Keep mid-wave restart semantics explicit.
2. **Finish the requested battlefield rebuild:** complete weapon mounts/pivots/recoil/reload/UVs; unit model identity and unloading paths; full range/speed/targeting tuning; shore details for all four theaters. Keep render/hit/collision transforms shared.
3. **Run the QoL pass:** reserve layout areas for persistent instructions vs alarms; avoid overlay collisions; explain reload/locked weapons/AA immunity; make threatened battery/shot impact/next-wave state readable; verify all three save-slot, summary, shop/armory, reset, rack, support and station-switch paths with held and stale input.
4. **Release evidence:** actual APK full campaign paths for all difficulties/theaters, meaningful pause/kill/reopen tests at shop, purchase, wave-clear, defeat and victory; measured target visibility and two-eye alignment; real S9/Daydream controller, audio, long-session frame/thermal checks. Pass clean install and update-over-existing-save. Only then remove development marking.

## 2. Guess the Door

### What is implemented

There is a coherent bounded game: twelve seeded rooms, exactly one matching physical door, five two-valued feature families, a 2.3-second clue, inspection popup, lever/open/walk/fall phases, same-seed retry and victory. Rule implementation is `guess_the_door_game.h:9-87,105-169`; the deterministic rule gate passed. The module supports pointer lever targeting and rocker fallback when no usable pointer exists. The slow transitions are 0.7/1.5/1.6/1.8 seconds, with no forced yaw.

The canonical brief still requires the existing Mechanist closed/open artwork and spoken Easter line. They are explicitly unaccepted (`docs/project/GUESS_THE_DOOR.md:28-46,48-53`). Current render is procedural doors, the Easter event is caption-only and cues are borrowed host effects (`guess_the_door_module.cpp:224-259`). Do not call this complete by assuming those requirements are waived.

### Reproduced faults

**GD-01 — new-seed click also pulls the first lever.** Victory handler resets the game, then falls through to the Ready handler using the same primary rising edge (`guess_the_door_module.cpp:307-311,339-344`). The audit drives actual Update to a legitimate twelve-room Victory, releases input and presses once. Observed result: phase Pulling, side 0, rather than Ready awaiting the new clue. With no pointer this deterministically selects left; with a usable lever pointer it can select that target. Add event consumption or a release gate after restart. The pure-rule suite cannot catch this module-level defect.

**GD-02 — zero continuity across recreation.** Production Create always seeds from the current steady clock (`262-269`), Destroy simply deletes state (`272`), and Pause/Resume are empty (`388-392`). After reaching room 3 through actual Update, the audit paused/resumed/destroyed/recreated the module: room became 0, seed changed, read-save count=0, write-save count=0. A player has no return to the remembered route after ordinary module exit or process recreation. Save root seed, room/falls/progression and a clearly defined safe phase; handle corrupt data and failed writes. Current deterministic retry within one instance is not durable save preservation.

**GD-03 — essential clue clipped in authentic Android boot frame.** Parent's `native-menu-guess-the-door.png` shows only the trailing `...ETAL RIVETS` clue, cropped room heading and clipped seed/footer. The clue is at a fixed HUD depth and wide x span (`guess_the_door_module.cpp:185-201`); the controls use a low baseline (`222-223`). This proves clipping at the instrumentation view/projection, not necessarily every headset pose. Because the clue expires after 2.3 seconds, default-view readability is gameplay-critical. Establish a real per-eye safe region and repeat captures across all ten clue strings, inspection text, fall and victory states.

Secondary QoL observations: the gaze timer is not reset when switching directly between doors, only when looking away (`337-338`); rocker choice can be irrelevant when a forward pointer exists but is not actually aimed at a lever (`317-340`). Both should be tested against the promised control guidance rather than accepted from the rule suite. Gaze-dwell behavior may be intentional; no failing device action was claimed.

### Completion track

1. Fix restart event consumption and add module-level transition tests; prevent held input selecting a new puzzle automatically.
. Add versioned durable save and explicit Continue/New Maze behavior; define whether an interrupted animation restarts its room or resumes at a safe checkpoint. Test exact seed/room preservation, failed writes, corrupt read, process recreation and explicit reset.
3. Fit clue/status/control/inspection/victory text within the actual visible region, preserve the challenge while offering readable timing/contrast and distinguishable feature geometry. Verify cue/animation order and recovery menu access in every phase.
4. Integrate approved Mechanist pairs without breaking five-feature clue truth, and supply the requested licensed/self-recorded speech plus appropriate opening/walking/pit audio. Any approved postponement should be an explicit scope decision.
5. Validate actual Android two-eye traversal, every failure/retry/victory/new-seed path and real controller/headset performance. A complete twelve-room playthrough and process recovery should be captured from the same candidate APK.

## 3. Sitka

### Honest current state

Sitka has substantial source systems, but the application cannot launch it as a native game today. Gallery visibility is an inspector, not admission. Module Create requires a bounded approved `sitka/coast/terrain.sitkam` and other validated world inputs (`sitka_terrain_module.cpp:136-153,320-337`); it does not create a fake geographic fallback. No Sitka assets directory or Android.mk native module was found. The latest project document says explicitly that it is source-only and absent from the playable launcher (`docs/project/SITKA.md:3-5`).

The latest genuine increment is meaningful: Tier 0 supplies, infinite basic bait/no starter wear, retained fish and shellfish sales, pending finds/token registration, 48 gear levels and four functional training tracks, existing hull/fitting purchase paths, movement locked while fishing/shop browsing, fresh-edge transaction controls and visible save failures (`docs/records/2026-11-07-sitka-starter-harbor-loop.md:9-17,35-44`). Actual-module Update tests walk a synthetic route to a desk, sell, register and purchase, then check stored state and forced-failure retry. Actual module GLES tests render two eyes and confirm read-only render does not award money twice. This is good integrated source evidence, but none of it establishes a traversable real harbor in the installed app.

### Save preservation

`FishingSession` distinguishes fresh, restored, invalid and I/O-failed load outcomes (`sitka_fishing_session.h:31-47`), retains dirty state on failed writes and clears it only on success (`105-112`). Module Pause retries dirty data (`sitka_terrain_module.cpp:593-597`). The actual starter-loop test injected a write failure at registration, saw `SAVE FAILED` and dirty state, then recovered through Pause and reopened without replenishing starter stock (`sitka_starter_loop_test.cpp:75-99`). This is stronger than just round-tripping a serializer.

The current authoritative record says owner confirmed no old saves and intentionally adopts clean-start `SITKA-FISH-5` (`2026-11-07-sitka-starter-harbor-loop.md:5`). Do not invent a requirement to migrate nonexistent earlier prototype saves. Future public releases will need version policy and backups.

The journal covers progression/economy. Exact walker/boat position, active cast/fight and world pot state are not established by this journal test. Define safe return/respawn behavior and extend persistence when those world systems are admitted. Current Create refuses invalid/unavailable save reads to avoid silent replacement (`sitka_terrain_module.cpp:338-341`); provide a recoverable UI path preserving originals before release.

### Missing intended functions

The latest gap table, not historical aspirational prose, is authoritative here (`docs/records/2026-11-07-sitka-starter-harbor-loop.md:31-46`):

- Approved geographic world/harbor asset assembly and native host admission.
- Terrain-matched walkable offices and entrances, visible shared avatar/characters, coherent travel/collision.
- World shellfish/pot catch integration, physical pole/line/bobber/fish visuals and richer audio.
- Runtime adoption of the 18 refined boat models; actual embark/helm/sounder/collision/dock-return acceptance in the real world.
- Pot crabbing/Deckwork remains a separate CPU prototype. `sitka_crab_work_cycle.h:4` explicitly says it is disconnected from saves, boats, world pots and sale UI. The shop correctly hides the unfinished Deckwork path.
- Gear/content unlock balance and legible headset input; sustained hardware acceptance.

### Completion track

1. **Admit one real connected world slice:** assemble approved terrain, water, fishing sites and harbor/desk inputs; prove Create succeeds; register in Android.mk, host and gallery together; establish entry, menu/exit and recovery. Do not expose an inert playable icon.
2. **Complete the shore journey in that actual world:** arrival, readable controls, walk, cast/land/retain, harbor sale, register find, unlock and buy gear, close/reopen to retained state. Add physical fishing presentation and coherent audio. Capture both eyes at landmarks and interaction distances.
3. **Integrate intended boat and crabbing systems:** hull assets/collision/mounts and helm; safe embark/dock; owned hull/fitting state, bathymetry and sounder; pots/bait/catch/Deckwork tied to one journal/wallet and sale loop. Prove no duplicate harvesting or loss after interruption.
4. **Finish content and QoL:** reachable progression prerequisites, nonrepeating/find identity records, taught interactions, navigation recovery, readable costs, safe return when saves/world versions differ, performance-appropriate material/LOD delivery. Preserve the infinite Tier0 recovery path.
5. **Candidate release:** complete end-to-end routes in the final APK, all admitted world files present/licensed/scoped, resource loss/lifecycle/corrupt-save tests, thermal/frame/input acceptance on S9 + Daydream. The full intended Sitka scope remains open until the boat/crabbing/character work is integrated; a shore-only release would need an explicit scope decision.

Sitka is the largest integration risk of these four. A calendar estimate before inspecting/licensing/assembling the actual world inputs would imply unsupported precision. Use the above exit gates to burn down scope.

## 4. Test Chamber

### Scope and current functionality

This is a regression/showcase and VR authoring utility. Stable completion means dependable creation, editing, auditioning, saving and recovery; it does not require a campaign ending. The native scene loads 30 authored primitive parts from a strict bundled REVSCENE (`docs/project/TEST_CHAMBER_SHOWCASE.md:3-15`), plus procedural texture and shared avatar/wardrobe. Six primary-select modes demonstrate lighting/material/audio using registered host cues. Companion application's five GLB objects and WAV bank are **not imported** into this native module (`TEST_CHAMBER_SHOWCASE.md:11-15`).

The workshop supports three projects, up to 48 editable pieces and 16 undo steps, with a 64KiB slot bound (`reverie_vr_workshop.h:19-20`). It has move/rotate/scale, primitive add/delete, save/play switching, authorable motions/light/sound bundle persistence and legacy upgrades. All six focused scene/workshop/motion/light/sound/scope executables passed in this audit. Release claims should describe this bounded editor rather than imply a general GLB or game-authoring engine.

### Preservation and recovery

Load parses into candidates and replaces state only after validation; corrupt data does not partially overwrite the project (`reverie_vr_workshop.h:99-133`). Failed saves retain dirty edits and report `SAVE FAILED - CHANGES RETAINED` (`135-147`); switching projects or exiting edit mode is blocked until successful save (`149-151,378-388,448-450`). Module Pause/Destroy try to flush edits and log failures (`test_chamber_module.cpp:790-792,832-840`). These are sensible boundaries and tested positive behavior.

**TC-01 — corrupt first project prevents entry to every project.** `VrWorkshop::Open` always loads slot0 (`reverie_vr_workshop.h:95-97`); native Create returns null if it cannot (`test_chamber_module.cpp:748-751`). Because the project selector is inside the module, an unreadable first slot blocks access to otherwise valid slots1/2. This is a source-established recovery gap, not a production data-loss reproduction. Add a non-destructive recovery/slot-selection screen and preserve the offending bytes. A last failed exit write is still only an in-memory change; the stable utility must present status before users rely on persistence.

**TC-02 — boot instruction clipping.** The actual Android boot PNG crops the left of the title and `PRIMARY`/`SECONDARY` guidance. Initial labels are hard-positioned at `test_chamber_module.cpp:121-128`, workshop panels span a wider field (`146-208`), and subsequent demo labels use another layout (`913-923`). Repeat actual state captures for showcase, editing, selected object, motion/light/audio pages, empty slots, errors and dirty warnings. This must be checked before calling the UI release readable.

### Completion track

1. Establish a clear stable-utility scope and consistent title; distinguish the integrated native chamber from companion content.
2. Add corrupt-slot startup recovery with explicit slot selection/backup/reset, and visible actionable persistence state through pause/exit. Retain the existing fail-before-switch behavior.
3. Exercise the actual module UI for move/rotate/scale/add/delete/undo; all three projects; save/play; motion/light/audio pages; failure recovery. Header tests cover the data model, not every rendered control.
4. Fix per-eye text/selection layout, overlap/occlusion and touch/gaze/controller feedback. Prove each audio and material mode is distinguishable.
5. Ship only after same-APK two-eye/context/lifecycle/launch-exit verification and target-device performance/input acceptance. It may reach stable development-utility status sooner than the larger games once these bounded gates pass.

## Checkpoint and continuation package

The parent owns durable publication/checkpoints. This folder contains `PROGRESS.md`, `report.md`, `findings.json`, `results.json`, raw logs, three audit probe/capture C++ files and actual raw/PNG framebuffers. Both successful and failed results were recorded immediately after each test. The structured findings distinguish reproduced failures, source-established gaps, visual observations and unverified hardware gates. A resumed worker should reuse these results at the same source SHA and rerun only after relevant code changes; do not mistake a previously completed CPU test or a boot frame for a completed game.
