# ReverieVR native games audit

## Decision and scope

**There are working games here, but none of the audited projects can yet be conservatively certified complete against its intended scope and the evidence obtained.** The immediate common problem is continuity: **11 of 16 registered native modules never read or write a game save.** They are the eight classics, Guess the Door, Ministry of Intelligence: Lantern Desk, and Iron Sight: Forward Detachment. Production-module probes reproduced the lost progress. Red Ledger, Gumball Armada and Breakwater have further reproduced storage or interruption defects. Omega has the strongest tested campaign recovery. Test Chamber has useful save safeguards but a startup-recovery gap. [E1-E3]

Audit date: **8 November 2026**. Production baseline: `12d4018c90e56a3767b8ffc58a2e5394440caec1`. The audit covers all sixteen registered modules in the existing ReverieVR Android host and source-only Sitka. It includes current implementation, intended-function gaps, controls/menus, readability, quality of life, save preservation, actual diagnostic execution, completion stages and evidence limits. Production game source remains at the baseline. No new APK or workflow run was produced by this audit.

The registry contains **seven development-only entries and nine ordinary entries**. Sitka has source/tests and an inspectable project card, but no Java registry entry or Android.mk target. The Mechanist is referenced as a Guess the Door artwork source; it is not an integrated native title here. DOOM belongs to the DOS host and is outside the native count. All completion tracks retain the single ReverieVR host architecture. [E5, E10]

**Development publication and completed-game acceptance remain different decisions.** The owner's existing policy is unchanged: build/launch/run and required updater/multiplayer failures control publication; visual/style/balance findings remain diagnostics. This audit creates no additional blocking CI or publication rule. An individually completed game needs its intended functions, truthful controls and menus, durable state and demonstrated playable acceptance. Ordinary catalog visibility and a green workflow badge do not establish those things.

## Inventory and preservation

| Native project | Admission | Current stage | Save verdict |
| --- | --- | --- | --- |
| Red Ledger | Development | Playable opening economy | Write retry / integrity faults |
| Breakwater Battery | Development | Campaign alpha; rebuild unfinished | Checkpoint safety faults |
| Omega Front | Ordinary | Substantial regional foundation | Tested retry / full restore pass |
| Gumball Armada | Development | Arcade / construction foundation | Active sortie / retry gaps |
| Lantern Desk | Development | Evidence-choice demonstration | None; recreation loses state |
| Forward Detachment | Development | Patrol-decision demonstration | None; recreation loses state |
| Guess the Door | Development | Playable bounded prototype | None; recreation loses state |
| Chess | Ordinary | Compact rule loop | None; recreation loses state |
| Go | Ordinary | Compact rule loop | None; recreation loses state |
| Checkers | Ordinary | Compact rule loop | None; recreation loses state |
| Pong | Ordinary | Compact rule loop | None; recreation loses state |
| Snake | Ordinary | Compact rule loop | None; recreation loses state |
| Reversi | Ordinary | Compact rule loop | None; recreation loses state |
| Breakout | Ordinary | Compact rule loop | None; recreation loses state |
| Chutes & Ladders | Ordinary | Compact rule loop | None; recreation loses state |
| Test Chamber | Development | Development utility alpha | Startup recovery gap |
| Sitka | Source only | Unregistered integration project | Positive journal test; partial world scope |

## Highest-priority findings

### The shared menu can continue gameplay and retain stale input

Source tracing shows Quick Menu still calls native Update with `POINTER_NONE`. Elapsed time and stored joystick state still reach the module. Pong, Snake and Breakout continue stepping, and board CPU results can still resolve. This is a source-confirmed route; a fresh Android menu interaction was not executed here. Repair acceptance should prove gameplay invariance while the intended pause menu is open, neutral guest input and a clean return without a catch-up jump. [E5]

Home calls native Pause and Destroy. "RESUME NATIVE" starts a new instance. The actual classic probes each progressed, retained memory during same-instance Pause/Resume, then reset after Destroy/Create, with zero save reads/writes. Together, the source route and runtime results establish that ordinary Home/Resume loses the classic match. A remembered module identity is not a remembered game. [E1, E5]

A second source-traced path can leave a button or axis latched. Ordinary menu entry clears keyboard/mouse HID state but not native joystick bindings. While the menu is visible, binding callbacks discard events, including releases and zero-axis updates; meanwhile the controller router advances its prior physical state. Closing the menu need not emit another release. The narrow repair test is hold Select/pad direction, open Menu, release inside Menu, then close without touching anything. Repeat recovery-menu entry and disconnect/reconnect. Existing Activity-pause and controller-reconnect release/rearm protections are stronger and should inform the fix. Physical reproduction remains open. [E5]

### Preservation defects reproduced in the real modules

| Finding | Observed behavior | Consequence |
| --- | --- | --- |
| Eight classics | All progressed; all recreated at initial state; zero save callbacks | Normal Home return loses the match/run |
| Guess the Door | Room 3 recreated as room 0 with a changed seed; zero save callbacks | Remembered route and progression disappear |
| Lantern Desk | Filed report gives credibility +3/effect +2; Next Sortie resets both to 0 | No lasting intelligence campaign |
| Forward Detachment | Return at condition 75/fuel 10; New Patrol resets to 100/20 | Damage and resource consequences disappear |
| Breakwater | Read I/O error treated as empty; difficulty selection writes a fresh 80-byte save | Recoverable prior slot can be overwritten |
| Breakwater | Rejected write still produces activation success; Pause does not retry | Player receives misleading persistence feedback |
| Red Ledger | Rejected collection write: cash 2480 in memory, durable cash 2200/pending 280; no retry after 80 updates and Pause | Acknowledged collection rolls back on recreation |
| Red Ledger | A one-bit currency mutation in the current composite is accepted as 2481 | Base economic state lacks full integrity protection |
| Gumball | Paid Eclair costs 45; launch consumes booking; recreation returns to Chart with cash 55/no escort/elapsed 0 | Paid sortie benefit is lost without sortie continuity |
| Gumball | Failed Blaster purchase warns, then never retries on idle/Pause | Recreating loses the purchased upgrade |

Red Ledger's earlier payment checkpoint remained coherent and collectible once; this case did not establish double payment. Breakwater also displays a completed-wave summary before saving the earned boundary; the save waits for a later summary action. That timing gap is source-established, rather than a killed-device reproduction. [E1-E3]

**Omega provides a positive recovery control.** Actual Title/Campaign/travel/deploy/queued move/End Turn reached tactical tick 1. A deliberately rejected write set the failure state. After storage recovered, Pause wrote successfully and recreation restored exact Session checksum `2463315748711018088`, behind a safe title screen. Its 2048-byte schema-9 checkpoint includes tactical/strategy state, queues and RNG. This proves the tested path, not every power-loss timing or the entire intended campaign. Sitka also passed failed-save retry in its synthetic source-stage harbor loop. [E2-E3]

The host save service supplies game isolation, a 64 KiB bound, temporary writes, file fsync and rename. Modules must use it and honor the result. Source review did not establish parent-directory fsync or a last-known-valid backup, so broader crash/power-loss durability remains unproven. The shared development pattern should be checked versioned snapshots, transactional load, dirty retention/retry and a truthful recovery screen. Save history that affects legality and exactly-once transactions; do not serialize raw STL objects, futures or graphics resources. [E5]

Test Chamber's failed-save retention is limited to the live module: its editor blocks project/edit-mode switches and keeps dirty work in memory, but Destroy attempts Save, logs failure and still deletes that state. Unsaved edits can therefore disappear on Home/exit after a rejected final write. This is a source-established exit gap, alongside the corrupt-slot-0 startup problem; it was not newly reproduced on a device. [E3]

### Actual control and presentation defects

**Snake's visible UP and DOWN buttons fail their labels.** A fresh-instance production-module probe clicked each rendered center with release/press/release input. Both started the snake moving RIGHT. Positive controls inside the coded upper/lower regions correctly produced UP/DOWN. Drawn centers sit at y +/-1.02; activation requires beyond +/-1.38. The build passed and the probe exited 1 for two violated expectations. This is executable confirmation, with no replacement game logic. [E1]

**Guess's New Seed click also pulls the first lever.** The actual Update probe traversed all twelve rooms to Victory, released input, then clicked once. Reset fell through to the Ready handler using the same edge and entered Pulling instead of awaiting the clue. Add event consumption or a fresh-release gate. Its pure-rule tests cannot expose this module-level defect. [E3]

Real framebuffer captures show Red Ledger's ROOM plaque crossing a status row, Gumball's save warning crossing affordability text, Omega's END TURN label crossing its title, and Breakwater's HULL HIT banner crossing persistent instructions. Authentic Android boot frames crop the essential timed Guess clue and some Test Chamber guidance at that test projection. These are observed diagnostics. Physical headset comfort, font size and target recognition remain unverified. Angled Chess reference wings extending beyond a forward frame are not automatically defective; head-turn acceptance is needed. [E1-E4]

The inspected Red Ledger payload loaded 24/24 product icons, 16/16 product meshes and 4/4 owned smoking meshes, but no files for the fourteen curated room props or six patrons, producing primitive fallback. Gumball used fallback art for ten requested external artwork files. Optional external packs may exist elsewhere; these results describe the inspected default payload. Final content needs a real release manifest and installed-candidate verification. [E2]

## What the test evidence establishes

There is **no justified all-tests-green claim**. The archive retains exact commands, timings, logs and attempts, including failed harness/environment setup and corrected retries. Units below differ and should not be added into one inflated test total. A probe may exit zero because a known defect was successfully reproduced; that does not mean the game passed readiness.

| Layer | Actual result | Practical limit |
| --- | --- | --- |
| Classic production lifecycle | Eight compile; eight progress; eight fail recreation preservation | Graphics linked out in this harness; no local render claim |
| Chess legality/input | 20 reference move-tree comparisons and eight actual input/menu scenarios pass | Original reported incident not reproduced |
| Featured existing C++ programs | 49 pass: Red 11, Gumball 11, Omega 25, recon 2 | Kernel passes do not complete authored campaigns |
| Featured production GLES probes | Five real modules, both eyes, context recreation; faults and Omega recovery observed | Mesa software driver, not S9 optics/performance |
| Sitka | 34 CPU programs and three actual-module/GLES integrations pass | Synthetic geography; no packaged launch |
| Breakwater | Existing content/visual and campaign programs plus real GLES pass | Controlled scenarios, not full human campaign acceptance |
| Guess and Test Chamber | Guess rules pass across 80 seeds x 128 rooms; six editor/scene programs pass | Module interactions and startup recovery still have gaps |
| Root Java | 18 JUnit tests pass: registry, pack installer, manifest, asset validator | Fixed-count test still asserts sixteen and absent Sitka |
| Root host manifest | Nine of eleven final case statuses pass | Doc-sync and broad packaging families remain failed |
| Broad Python packages | 63 tests run; five failures and eight errors | Stale paths/fixtures and missing curated owned assets |
| Desktop | Two Java programs pass, including four prototype state/render paths | Four Java 2D games; no sixteen-module desktop parity |
| S9 sampler | Twelve parser/fixture tests pass | No new S9 performance samples |
| Retrieved Android | Thirteen instrumentation tests pass; sixteen native initial GLES frames | Parent revision, Android 10 test environment; no physical headset |

Chess's production move generator matched twenty canonical counts across six positions, including initial depth four **197,281** and Kiwipete depth three **97,862**. Actual pointer tests rejected blocked queen moves for White and rotated Black and accepted expected legal pawn moves. This narrows the reported incident investigation to its exact position/event/presentation context; it neither reproduces the incident nor proves every hardware path correct. [E1]

Breakwater's real Mesa run loaded 4/4 horizons, 16/16 materials and 5/5 models, tested both eyes at three stations, demonstrated visible guide pixels and recreated its context. Featured probes retained fifty eye captures and matching sampled per-eye hashes after GL recreation. These are genuine production framebuffer readbacks, not generated imagery. [E2-E3]

Two host diagnostic families remain failed. The native doc guard expects a direct Test Chamber CompileShader call even though the module correctly uses the shared BuildProgram helper. Broader package tests reference old private Breakwater/Omega paths and counts after shared-asset migration, and require missing curated Red Ledger patron files. One corruption fixture mutates an obsolete filename and therefore mutates no actual content; its failed assertion is not evidence of a real cross-namespace acceptance bypass. Repair the utilities/fixtures against current contracts, preserving the checks rather than manufacturing green status. [E6]

Initial Java attempts lacked JDK `ct.sym`; a real JDK development payload restored `--release 17`, after which the eighteen tests and desktop programs passed. An expanded audio/recovery subset could not compile without Android dependencies and is not counted as executed. Additional Sitka `-O1 -Werror` experiments failed on shared shader-diagnostic snprintf truncation warnings; the existing script's actual default flags passed all three integrations. This is an optimized GCC diagnostic issue, not an established Android Clang or gameplay failure. Clang, a local Android SDK/device and physical S9/Daydream were unavailable. [E3, E6]

## APK, content distribution and release truth

The baseline's [signed-phone-test run 42449220006](https://github.com/mrcalzon02/ReverieVR/actions/runs/42449220006) is green, but **build-and-release was skipped**. Its gate ran; it is not a new APK build for this revision. Baseline functional/content workflows succeeded within their selected scopes. The broader local failures remain. [E4]

The authenticated [hub integration run 42417911507](https://github.com/mrcalzon02/ReverieVR/actions/runs/42417911507), source `c10ddd72cfd5150646c855f25e44955490d233c7`, actually built/exercised the Android host, ran thirteen tests and captured sixteen native initial frames. The intervening production change to the audit baseline is Sitka-only, so the registered modules match that evidence. Downloaded artifact size: **158,042,858 bytes**; SHA-256: `e2bd401f420c0728ce0ddae487805b1cdb3e92311c806ad3f2e8ed732ad67506`. Relevant logs, captures and Omega checkpoints are retained; the large APK ZIP is excluded from the portable evidence bundle. [E4]

That run tests local synthetic per-game acquisition/update/rollback and locally repackaged real APK assets. It does not establish a populated public game download offering. The live detached-signature-verified catalog is schema 1, revision 1, **zero modules**. Native C++ binaries remain packaged in the host; data packs do not independently update their gameplay code. Finish online distribution through signed populated metadata, real release packs and fresh-device download/update/rollback/offline/save-preservation evidence, reusing the existing updater architecture. [E4, E7]

The latest observed non-draft phone prerelease is [phone-test-1961-1](https://github.com/mrcalzon02/ReverieVR/releases/tag/phone-test-1961-1), source `1c5736ceb66e92edc88c19f4a17814024e972d6f`. Its headset asset is `ReverieVR-phone-test-1961-1c5736c.apk`, **157,375,347 bytes**, SHA-256 `6c80950ab6082942d120a49872000f0865f51b6dd396f375cca7f83117439054`. It predates the audit baseline. This audit makes no newer publication or physically installed-candidate claim. [E4]

## Ordered development lanes

1. **Shared reliability:** repair Quick Menu pause/input release and Home/Continue semantics; establish truthful save outcomes and recovery UI; fix Breakwater's unreadable-slot overwrite immediately. Use narrowly targeted regressions for reproduced faults and the source-traced menu sequence.
2. **Compact completions:** persist the eight classics exactly, then close each title's own control/menu/end/readability tasks. Repair Guess restart/save/clue layout and its committed art/speech scope. Finish Test Chamber recovery and actual editor controls. Reversi and Pong are useful first finished loops; Snake's targeting defect belongs early.
3. **Current campaign reliability:** repair Red, Gumball and Breakwater preservation, then finish and accept current playable slices before expanding progression. Omega's checked state and retry model provides a useful existing pattern.
4. **Substantial intended scope:** advance Sitka's real world assembly/admission/shore slice in parallel, then boats/crabbing. Build Lantern's actual intelligence desk and Forward's actual patrol sector. Complete Red's property/calendar story, Gumball's regional campaign and Omega's executable authored arcs in playable sections. A project card, map decoration or simulation kernel is not completed gameplay.
5. **Candidate acceptance:** record exact source/APK/content versions; play the intended loop through its ending; verify all menus and actual controls, release/reconnect, pause/Home/process recreation, missing/corrupt/failing storage, expensive/once-only transactions, both-eye readability, audio and sustained S9/Daydream frame/thermal behavior. Use the same candidate for the claimed evidence.

No percentages or calendar dates are assigned. The work spans bounded persistence additions and substantial unfinished worlds/campaigns, with no physical-device observations in this session. Per-title observable exit criteria follow. Visual/style/balance remain diagnostic for development publication; completed-game sign-off should state exactly what scope, hardware and interruption contract passed. Do not invent multiplayer work for a single-player title.

## Checkpointing and continuation

Each workstream preserved command/result metadata and raw attempts. The audit runner writes source/command identity and running state before each case, flushes logs, atomically replaces JSON with file/directory fsync, preserves retries and verifies completed-log hashes. An actual SIGTERM/resume test preserved completed work, retained the interrupted attempt and resumed without rerunning the finished case. Review found an unselected-log verification gap and an abrupt-kill orphan risk; both received focused runner hardening and follow-up acceptance. The final tooling record specifies the supported boundaries. [E8]

The reviewed checks rejected a corrupted unselected log, blocked both resume forms while an orphan retained the output lock, required explicit recovery after cleanup, and terminated a TERM-ignoring descendant on timeout and SIGTERM. The source fingerprint covers tracked code excluding docs, art and workflow files; it does not authenticate external/untracked inputs, toolchain versions or environment. Changed unrecorded inputs require a fresh evidence directory. No exactly-once guarantee is made across loss before the final checkpoint commit. [E8]

Local checkpoints do not guarantee survival of a lost workspace. The standalone evidence ZIP is therefore preserved independently in successive versions, with every included file hashed from the same captured bytes and the full ZIP CRC/hash manifest verified. An uncertain upload is not treated as successful. A hard-killed command requires process cleanup before explicit recovery; unfinished work is not silently relabeled passed.

Repository changes are audit-only and local. **Automatic approval review rejected publishing to shared main**, stating that this audit request does not explicitly authorize the remote mutation. No alternate remote-write route was attempted. The report, matrix and evidence can be delivered while the reviewable repository update awaits explicit approval.

Next implementation: shared menu pause/release and Breakwater save safety, then exact classic preservation; Sitka admission/world assembly can proceed as a separate bounded lane. Next repository publication: approve the completed audit-only update, refresh remote main, reconcile without force, publish and verify the resulting remote commit. Reuse this evidence at its production revision instead of restarting the audit.

## Evidence index

The archive contains full workstream reports, exact command/result JSON, failed and successful logs, probes, real PNG/PPM framebuffers and an integrity manifest. Paths below are relative to that archive. All source anchors refer to the production baseline, whose game code was unchanged by local audit checkpoints.

- **E1 - classics:** `evidence/classics/classics-audit.md`, lifecycle results/log, Chess perft/input logs and Snake pointer follow-up.
- **E2 - larger games:** `evidence/featured/FEATURED_NATIVE_AUDIT.md`, findings, existing-run metadata, five production probes, save-boundary evidence and paired eye captures.
- **E3 - recent projects:** `evidence/recent/report.md`, findings/results, Guess/Breakwater probes, Sitka attempts, editor logs and coastal captures.
- **E4 - remote Android/release:** `evidence/remote/provenance.json`, `evidence/remote/hub/` instrumentation, captures, logcat and Omega checkpoints; linked GitHub runs/releases above.
- **E5 - host:** `evidence/classics/host-menu-followup.md`; baseline shell/runtime/activity/JNI and binding/router source, with exact anchored routes in the report.
- **E6 - root diagnostics:** `evidence/host/results/checkpoint.json`, all log attempts, and `evidence/host/java/` compilation/test metadata.
- **E7 - catalog:** `evidence/host/live-catalog-verification.json`, including signature verification and exact catalog digest.
- **E8 - checkpoint tooling:** `reproduction/checkpoint_runner.py`, README, test manifest and `evidence/runner-acceptance/`; reviewed acceptance and the independent runner review are retained alongside them.
- **E9 - matrix:** `release_matrix.json`, containing all seventeen verified projects, evidence types and ordered completion criteria. It is an audit view linked from the current project tracker.
- **E10 - scope authority:** baseline project instructions/execution contract, native handbook/contracts and game-specific scope documents, anchored in the detailed reports and matrix. Current source and executed evidence outrank historical completion claims.


## Individual game completion tracks

Each track preserves the intended project scope. The final common acceptance stage is stated once above; per-title requirements below specify the actual state and interactions that must survive it.

### 1. Between Deliveries: The Red Ledger

**Admission:** Development-only. **Stage:** Playable opening economy. [E2, E9]

**Current function:** Playable three-room bar economy; full property/district/monthly campaign remains unfinished.

Implemented: Cup/tap/serve/payment/wash work loop; Finite branded goods, suppliers and faction favor; Paid renovation tiers, clientele condition and day-close ledger.

**Save verdict:** 372-byte composite restores current economy/room, but rejected writes are ignored/no pause retry, and base economic bytes lack integrity protection.

**Principal open issues:** Payment action yields live cash 2480, but rejected save leaves durable cash 2200/pending payment280. Restored availability, 80 idle updates and Pause perform no retry; recreation rolls back acknowledged collection. Prior checkpoint is coherent and permits one later collection. Flipping bit 0 of byte 12 in valid 372-byte composite changes restored cash 2480 to 2481 and is accepted; standalone 160-byte serializer similarly accepts2200→2201. This checkout loads 24 icons, 16 product meshes, 4 owned smoking meshes;0/14 curated room props and 0/6 patrons. Existing runner omits two pertinent asset tests; one requires external real files.

**Ordered completion track:**

**1. Repair current save and room interaction.** Authenticate the entire composite, retain pending writes, show retry/status and separate plaque/status layout.

Acceptance: Single-bit cash mutation is rejected without changing the last valid save. After write rejection then recovery, paid service/stock/room actions persist exactly once across pause/recreation. Each seated work target is reachable and its state/error text is readable.

**2. Complete bar economics.** Implement meaningful storage/quality/perishability, affinity-driven patron consequences, accounting, calendar pressure and promised upgrade machinery.

Acceptance: From-new operating days demonstrate scarcity, genuine supplier choice, condition and arrival effects. Every purchased upgrade provides its documented function and survives restart.

**3. Complete property and district.** Add genuine upper-floor/private activities and district commerce; deliver declared room/patron assets.

Acceptance: Every included location is reachable and useful; transactions obey real inventory/permissions. All release asset IDs are installed, decoded and visibly used; no silent required-content fallbacks.

**4. Complete named narrative and endings.** Implement monthly cast transitions, neutrality pressure and six saved faction-dependent Month Five endings; resolve documented continuity ambiguities.

Acceptance: Each required monthly beat occurs at its proper saved prerequisite. Representative legitimate playthroughs reach every defined ending without softlocks or invented neutral ending.

**5. Establish hosted and physical acceptance.** Complete the full intended Between Deliveries campaign with current-save integrity, controls, audio, packaging and reference-device acceptance. Pre1.0 legacy-save migration is not presumed required.

Apply the common candidate-acceptance requirements in the development lanes above; retain the exact package, device and interruption evidence for this title.

Source anchors: [GameModuleRegistry.java:25](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/java/io/github/mrcalzon02/reverievr/GameModuleRegistry.java#L25); [red_ledger_module.cpp:539](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/red_ledger_module.cpp#L539); [red_ledger_simulation.cpp:215](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/red_ledger_simulation.cpp#L215). Additional scope/issue anchors and exact evidence are in the matrix and workstream reports.

### 2. Breakwater Battery

**Admission:** Development-only. **Stage:** Campaign alpha; rebuild unfinished. [E3, E9]

**Current function:** Four-location coastal defense campaign with shops, weapons/upgrades/support and a new incomplete coastal renderer.

Implemented: Four locations and four difficulties; day/night schedules; Five selected weapons, SAM/artillery/airstrike/support; Transport, landing infantry, armor and aircraft; Real menu/hit geometry and deliberate reset confirmation.

**Save verdict:** 80-byte Shop checkpoint; actual read I/O failure is treated as empty and can be overwritten. Failed writes still activate with no Pause retry. Wave-clear saving is source-traced as deferred until summary action.

**Principal open issues:** Save read I/O failure is treated as empty; choosing difficulty writes a fresh 80-byte checkpoint over the unreadable slot. Failed write returns activation feedback; zero data persisted; Pause performs no retry. Newly cleared wave is shown at summary without saving; checkpoint is delayed until summary action.

**Ordered completion track:**

**1. Harden checkpoint and slot safety.** Separate absent/invalid/I/O-failed reads; retain originals, require explicit replacement, retry failed writes and checkpoint completed waves/victory immediately.

Acceptance: Unreadable existing slot cannot be overwritten as empty by ordinary selection/difficulty. Failed purchase/write retains dirty state and retries with truthful status. Leaving at displayed wave result preserves the earned result; mid-wave restart/resume contract is explicit.

**2. Finish intended coastal battlefield rebuild.** Finish mounts/pivots/reload/recoil/UVs, model identity, unloading paths, range/speed/targeting and shore detail for all four theaters.

Acceptance: Weapon render/pick/collision transforms agree and advertised mount/reload/recoil behavior works. Ground/sea/air units and unloading are coherent at approved range; all four locations have intended usable content.

**3. Complete console/armory/shop/station interaction.** Separate alarm/instruction layout; teach reload, locked weapon/AA conditions; verify slots, summaries, reset and support with held/released input.

Acceptance: All three slots and explicit reset behave consistently without double actions. Important damage/aim/next-wave feedback and controls remain readable in actual two-eye views.

**4. Establish hosted and physical acceptance.** Exercise actual campaigns across four theaters/difficulties, shop/purchase/wave-clear/defeat/victory interruptions, target recognition and long coastal-combat sessions.

Apply the common candidate-acceptance requirements in the development lanes above; retain the exact package, device and interruption evidence for this title.

Source anchors: [GameModuleRegistry.java:27](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/java/io/github/mrcalzon02/reverievr/GameModuleRegistry.java#L27); [breakwater_module.cpp:322](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/breakwater_module.cpp#L322). Additional scope/issue anchors and exact evidence are in the matrix and workstream reports.

### 3. Omega Front

**Admission:** Ordinary registry entry. **Stage:** Substantial regional foundation. [E2, E9]

**Current function:** Real 16-sector Hearthfall region and six tactical operations; larger documented campaign remains unfinished.

Implemented: Explicit turn-based battle with queued orders; Travel/deploy, finite logistics, recruitment/production/convoy and scouting; 17 major-faction relationships, accords/obligations and consequences; Title/Campaign/Continue/New-confirm/Pause/Orders/Gallery and full strategic/tactical saves.

**Save verdict:** 2048-byte schema 9 checked strategic/tactical checkpoint. Actual rejected EndTurn write became visible; recovered-storage Pause retried, then recreation restored checksum 2463315748711018088/tick 1 behind safe Title.

**Principal open issues:** 16 playable sectors/six operations;30×20 overview cells are geography, not600 playable sectors. Full70-arc campaign and broader management UX remain open.

**Ordered completion track:**

**1. Complete existing region interaction.** Separate tactical title/EndTurn, explain legal actions/costs/goals and finish real six-operation input/aftermath paths.

Acceptance: New player can start, understand, negotiate/fight, complete and resume all current operations without console intervention. EndTurn control is readable and deliberate; denied actions explain applicable reasons.

**2. Complete actual campaign management.** Provide meaningful planning, roster/equipment/tech/logistics/history/objective pages using authoritative state.

Acceptance: Each displayed action works or explains its unmet prerequisites. Planning/launch/abandon and management choices persist and have observable simulation effects.

**3. Produce the documented campaign.** Implement additional authored arcs/theaters region by region; preserve chronology, progression and consequences.

Acceptance: Each claimed arc is executable and complete; geographic overview cells are not counted as playable sectors. Legitimate campaign routes cover advancement, defeat/recovery and repeated settlement without softlocks.

**4. Establish hosted and physical acceptance.** Validate the full intended Omega campaign, save version/failure behavior, art/audio/credits, controller/stereo and sustained device behavior. A regional prologue requires an explicit separate scope decision.

Apply the common candidate-acceptance requirements in the development lanes above; retain the exact package, device and interruption evidence for this title.

Source anchors: [GameModuleRegistry.java:29](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/java/io/github/mrcalzon02/reverievr/GameModuleRegistry.java#L29); [omega_front_campaign_save.h:18](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/omega_front_campaign_save.h#L18); [omega_front_module.cpp:923](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/omega_front_module.cpp#L923). Additional scope/issue anchors and exact evidence are in the matrix and workstream reports.

### 4. Gumball Armada: The Search for the Candy King

**Admission:** Development-only. **Stage:** Arcade / construction foundation. [E2, E9]

**Current function:** Functional route/arcade/ship-building foundation; full revisitable sandbox and authored King campaign remain incomplete.

Implemented: Guarded route arm/wait/launch and real preparation menus; Controller-steered combat, weapons/upgrades, enemies/formations/objectives/salvage; Layered ship assembly/readiness/profile and paid support contracts.

**Save verdict:** 56-byte checksummed campaign totals and separate ship construction state; active sortie is absent. Deployed paid escort is lost on recreate; failed purchase save is visible but not retried on idle/Pause.

**Principal open issues:** Actual Eclair purchase costs45. After guarded launch and 0.55s flight, module recreation restores Chart, elapsed0, cash 55, no booked/deployed support. Blaster purchase gives level1/cash 30 and visible save error after rejected write. Recovery, 80 idle updates and Pause do not retry; recreation restores level0/cash 100. 4096 sequential lane addresses are not revisitable persistent graph; four flags do not provide four authored campaigns or King finale/credits.

**Ordered completion track:**

**1. Make interruption conserve progress and paid resources.** Save active sortie or adopt explicit safe restart transactions; add bounded dirty retry for campaign/shipyard; reserve warning space.

Acceptance: Hire→launch→interrupt preserves or coherently restarts paid support without double rewards. Failed purchase/construction writes remain recoverable and are retried; persistent and error text do not overlap. Win/loss/kit consumption/result boundaries survive recreation under the declared contract.

**2. Complete combat, construction and parts economy.** Finish functional components, weapons/defenses/actives and intended buy/sell/steal/earn loop.

Acceptance: Different legal builds create different playable combat choices. Parts transactions and physical readiness rules are understandable and resistant to duplication.

**3. Build the revisitable regional graph.** Replace forward address progression with actual node/edge routes, return travel, vendors, persistent territory and story paths.

Acceptance: Declared return routes and critical nodes are reachable. Visited/cleared/commercial/story state persists without locking the mandatory route.

**4. Finish four authored general campaigns.** Finish Sally as a full chapter, then Cringe/Pucker/Sauer with their distinct mechanics and boss phases.

Acceptance: Each chapter is playable from legitimate preceding progress and provides its documented unlock/consequence. Boss phases respond to real fitted tools and saved story requirements.

**5. Resolve the King campaign and finish production content.** Implement mystery reveal, finale, credits/postgame; finish approved art, music/announcer/voice and functioning comfort options.

Acceptance: Full cold-start-to-credits route is possible without optional secrets or rare-drop lockouts. Required narrative/audio/art events are present, attributable and recoverable; postgame state saves.

**6. Establish hosted and physical acceptance.** Validate complete Gumball campaign with actual steer/fire, build variety, accessibility, interruption, sustained combat/boss load and licensed content.

Apply the common candidate-acceptance requirements in the development lanes above; retain the exact package, device and interruption evidence for this title.

Source anchors: [GameModuleRegistry.java:31](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/java/io/github/mrcalzon02/reverievr/GameModuleRegistry.java#L31); [gumball_armada_simulation.h:41](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/gumball_armada_simulation.h#L41); [gumball_armada_module.cpp:866](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/gumball_armada_module.cpp#L866). Additional scope/issue anchors and exact evidence are in the matrix and workstream reports.

### 5. Ministry of Intelligence: Lantern Desk

**Admission:** Development-only. **Stage:** Evidence-choice demonstration. [E2, E9]

**Current function:** Three-sector evidence/assessment board with deterministic 16×16 photos and report grading.

Implemented: Sector assignment and low-quality evidence generation; Four assessment choices and graded report; Civilian false-positive candidate.

**Save verdict:** No host save reads/writes; next sortie replaces Simulation and discards the just-earned credibility/effect.

**Principal open issues:** Correct report gives credibility3/effect2; next sortie resets both0. No read_save or write_save callbacks during tested lifecycle.

**Ordered completion track:**

**1. Persist operational state.** Create checked world/evidence/knowledge/aircraft/report session state and Continue/New/recovery flow.

Acceptance: Pause/recreate at assignment, evidence and report preserves intended state. A filed report applies its consequence once and influences the next task without leaking hidden truth.

**2. Implement the physical intelligence desk.** Add seated maps, photos, dossier, zoom/pan, annotation and report composition.

Acceptance: A player inspects and cites visible evidence through accessible physical controls. Annotations/reference/report work without unimplemented buttons or truth-revealing convenience labels.

**3. Finish a changing operational area.** Make reconnaissance conditions and underlying causes evolve, with ambiguous cases and operational consequences.

Acceptance: Positive/negative/false-positive cases arise from real world conditions. Different reports produce distinct saved future operations in a bounded authored sequence.

**4. Establish hosted and physical acceptance.** Validate the focused intelligence workspace and its bounded operational outcome; do not expand acceptance into an unintended tactical RTS or full continental war.

Apply the common candidate-acceptance requirements in the development lanes above; retain the exact package, device and interruption evidence for this title.

Source anchors: [GameModuleRegistry.java:33](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/java/io/github/mrcalzon02/reverievr/GameModuleRegistry.java#L33); [recon_projects_module.cpp:169](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/recon_projects_module.cpp#L169). Additional scope/issue anchors and exact evidence are in the matrix and workstream reports.

### 6. Iron Sight: Forward Detachment

**Admission:** Development-only. **Stage:** Patrol-decision demonstration. [E2, E9]

**Current function:** Two-route patrol choice board with ambiguous clues, decisions, fuel/damage costs and guarded debrief.

Implemented: Main road/ridge fuel differences; Observable contact clues and three decision choices; Return and once-only debrief.

**Save verdict:** No host save callbacks; new patrol resets post-engagement condition 75/fuel 10 to 100/20.

**Principal open issues:** Engage outcome reaches condition 75/fuel 10; New Patrol resets100/20. No save reads/writes across lifecycle.

**Ordered completion track:**

**1. Persist detachment and legitimate recovery.** Save patrol/task/evidence/debrief and make repair/resupply explicit finite operations.

Acceptance: Damage and spent resources remain until a legitimate replenishment action. Debrief/reward applies once across interruption; new task cannot freely reset costs.

**2. Build the compact physical sector.** Implement holding/JON, approach, checkpoint/bridge, industrial pocket and bypass, one scout/APC and a small dismount team.

Acceptance: Player can prepare, traverse, observe and return through actual spatial controls. Vehicle/team state, range/fuel and route alternatives affect a usable patrol.

**3. Integrate the intended encounter vocabulary.** Add negative recon, civilian ambiguity, IED indicators, scouts, damaged infrastructure and liaison with differentiated avoid/report/engage consequences.

Acceptance: Observable evidence supports identification without exposing hidden truth. Information quality and survival matter to result; ROE/resource consequences are legible and persistent.

**4. Close consecutive patrol and operation flow.** Complete briefing→preparation→patrol→decision→return→debrief→repair/resupply→next task and an operation outcome.

Acceptance: Several consecutive tasks preserve mistakes, intelligence and recovery opportunities. Operation success/failure has defined state and no once-only accounting duplication.

**5. Establish hosted and physical acceptance.** Validate the compact intended detachment game; no full 80-person formation or giant combined-arms scope is added.

Apply the common candidate-acceptance requirements in the development lanes above; retain the exact package, device and interruption evidence for this title.

Source anchors: [GameModuleRegistry.java:35](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/java/io/github/mrcalzon02/reverievr/GameModuleRegistry.java#L35); [recon_projects_module.cpp:186](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/recon_projects_module.cpp#L186). Additional scope/issue anchors and exact evidence are in the matrix and workstream reports.

### 7. Guess the Door

**Admission:** Development-only. **Stage:** Playable bounded prototype. [E3, E9]

**Current function:** Twelve-room seeded physical-feature clue maze with timed clues, inspection, lever/open/walk/fall transitions and retry.

Implemented: Exactly one matching door using five two-valued feature families; 2.3-second clue and inspection popup; Same-seed in-instance retry; legitimate twelve-room victory.

**Save verdict:** Actual room 3/remembered seed are lost on Destroy/Create; recreated room 0 has new steady-clock seed. Same-seed retry is only in-memory; no save reads/writes.

**Principal open issues:** Victory new-seed primary edge resets then immediately pulls first left lever when no pointer; Ready was expected. Room3 and remembered seed lost; recreated room 0 with new seed, zero host save reads/writes. Mechanist artwork not imported; requested spoken line remains caption-only; temporary borrowed sound cues.

**Ordered completion track:**

**1. Consume transition inputs correctly.** Consume victory/new-seed click or add release gate; interrogate all phase boundaries.

Acceptance: One press after Victory produces Ready with a new maze and no automatic first lever pull. Held/released inputs never select a new puzzle as a side effect of previous phase completion.

**2. Preserve maze and progression.** Save root seed, room/falls/progression and declared safe phase; add Continue/New Maze with corruption/failure recovery.

Acceptance: Recreate retains exact remembered maze/progress and supports intentional new seed. Interrupted animation resumes or restarts a documented safe room boundary without lost/duplicated result.

**3. Make essential clues and controls usable.** Fit all timed clue/status/inspection/result strings within actual visible regions and verify pointer/rocker/gaze guidance.

Acceptance: All ten clue strings and inspection/fall/victory views are readable in both eyes. Switching doors and pointer/pad fallback behave consistently with taught controls.

**4. Deliver requested artwork and speech.** Integrate approved Mechanist closed/open pairs while preserving five-feature clue truth; implement required approved spoken line and coherent transition audio.

Acceptance: Every displayed matching door has the stated physical features. Required voice/audio exists, is attributable and plays at the intended event.

**5. Establish hosted and physical acceptance.** Capture a full twelve-room same-APK route including fall/retry/victory/new-seed and process recovery with real controller/headset input.

Apply the common candidate-acceptance requirements in the development lanes above; retain the exact package, device and interruption evidence for this title.

Source anchors: [GameModuleRegistry.java:37](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/java/io/github/mrcalzon02/reverievr/GameModuleRegistry.java#L37); [guess_the_door_module.cpp:262](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/guess_the_door_module.cpp#L262). Additional scope/issue anchors and exact evidence are in the matrix and workstream reports.

### 8. Chess

**Admission:** Ordinary registry entry. **Stage:** Implemented compact rule loop; finish work remains. [E1, E9]

**Current function:** Human/CPU Chess with both player colors, rotated board, full special-move input, draw rules, SAN/reference UI and bounded CPU.

Implemented: King-safe legal moves; castling; en passant; four promotion choices; Checkmate/stalemate and conservative dead-position detection; Threefold/50-move claims and prospective claims; automatic fivefold/75-move draws; SAN, capture tallies, reference panels, attack overlays, difficulty and presentation-only tween.

**Save verdict:** In-memory Pause/Resume retains progress; Destroy/Create restores the initial game. Persistent host fixture saw 0 reads and 0 writes in each variant.

**Principal open issues:** Actual module loses progressed match on Destroy/Create; zero save reads/writes. Home→RESUME NATIVE uses this boundary. Reported illegal move not reproduced:20 canonical perft counts and tested real-module White/rotated-Black pointer paths pass. Exact reported position/input sequence is still unavailable.

**Ordered completion track:**

**1. Preserve complete match state.** Versioned validated bounded save of board, side, castling, en passant, halfmove clock, repetition map, SAN, human color, difficulty, capture tallies, promotion choice, result. Preserve last good data on write/read failure and rebuild transient workers/GL objects safely.

Acceptance: Progress through actual input, checkpoint, recreate and compare the complete state plus next deterministic legal transition. Home→RESUME restores the defined match; corrupt/failed reads do not silently overwrite recoverable data. Rejected writes remain dirty with truthful status and succeed after recovery; no duplicate score, move or reward.

**2. Complete match-management and diagnostic interaction paths.** Retain both-color New/Cancel/Continue, promotion, claims and references; capture position/action/worker serial/tween for reported move anomalies.

Acceptance: Both colors complete legal special moves, promotion, draw claims, New/Cancel and reference open/close without double actions. Exact reported anomaly can be reconstructed from compact position/action/worker/presentation evidence before legal rules are changed. Head turn reaches intended 45-degree side panels; long SAN/reference content remains usable.

**3. Finish full-match and interruption coverage.** Exercise actual hosted gameplay and all special/terminal phases.

Acceptance: Complete matches and interruption cases include both colors, a pending promotion, an active CPU computation and draw-history boundaries. Independent perft and the corrected real module pointer checks remain passing for relevant changes.

**4. Establish hosted and physical acceptance.** Validate this compact match on the supported host and physical reference devices; include the shared menu/release repairs.

Apply the common candidate-acceptance requirements in the development lanes above; retain the exact package, device and interruption evidence for this title.

Source anchors: [GameModuleRegistry.java:39](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/java/io/github/mrcalzon02/reverievr/GameModuleRegistry.java#L39); [classic_games_module.cpp:1249](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/classic_games_module.cpp#L1249). Additional scope/issue anchors and exact evidence are in the matrix and workstream reports.

### 9. Go

**Admission:** Ordinary registry entry. **Stage:** Implemented compact rule loop; finish work remains. [E1, E9]

**Current function:** 19×19 human Black/CPU White Go with captures, positional superko, pass, dead-group review and Chinese area scoring.

Implemented: Suicide rejection and exact superko history; Two passes enter review; dead-group marking; 7.5 komi; score acceptance; resume with preserved side.

**Save verdict:** In-memory Pause/Resume retains progress; Destroy/Create restores the initial game. Persistent host fixture saw 0 reads and 0 writes in each variant.

**Principal open issues:** Actual module loses progressed match on Destroy/Create; zero save reads/writes. Home→RESUME NATIVE uses this boundary.

**Ordered completion track:**

**1. Preserve complete match state.** Versioned validated bounded save of board, side, pass count, exact superko history, dead marks/review, resume side, scores, result, RNG. Preserve last good data on write/read failure and rebuild transient workers/GL objects safely.

Acceptance: Progress through actual input, checkpoint, recreate and compare the complete state plus next deterministic legal transition. Home→RESUME restores the defined match; corrupt/failed reads do not silently overwrite recoverable data. Rejected writes remain dirty with truthful status and succeed after recovery; no duplicate score, move or reward.

**2. Explain review and dense-board selection.** Show pass/review/mark/accept/resume effects, scoring ownership and komi; establish reliable feedback on the 19×19 target grid.

Acceptance: Users can mark/unmark a whole dead group, understand score ownership and choose Accept or Resume with correct next side. Selected and legal positions can be distinguished on dense boards in both eyes.

**3. Finish full-match and interruption coverage.** Exercise actual hosted gameplay and all special/terminal phases.

Acceptance: Full sessions cover both passing orders, contested dead groups, live ko/superko and resumed scoring. History encoding stays exact at its declared bound; no silent truncation changes move legality.

**4. Establish hosted and physical acceptance.** Validate this compact match on the supported host and physical reference devices; include the shared menu/release repairs.

Apply the common candidate-acceptance requirements in the development lanes above; retain the exact package, device and interruption evidence for this title.

Source anchors: [GameModuleRegistry.java:41](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/java/io/github/mrcalzon02/reverievr/GameModuleRegistry.java#L41); [classic_games_module.cpp:1249](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/classic_games_module.cpp#L1249). Additional scope/issue anchors and exact evidence are in the matrix and workstream reports.

### 10. Checkers

**Admission:** Ordinary registry entry. **Stage:** Implemented compact rule loop; finish work remains. [E1, E9]

**Current function:** 8×8 human/CPU Checkers with forced captures, multi-jumps, kinging, wins and supported draw claims.

Implemented: Mandatory capture and continued jumps; Three CPU difficulties with bounded search; Repetition and quiet-move draw claims, including prospective repetition.

**Save verdict:** In-memory Pause/Resume retains progress; Destroy/Create restores the initial game. Persistent host fixture saw 0 reads and 0 writes in each variant.

**Principal open issues:** Actual module loses progressed match on Destroy/Create; zero save reads/writes. Home→RESUME NATIVE uses this boundary.

**Ordered completion track:**

**1. Preserve complete match state.** Versioned validated bounded save of board/kings, side, forced continuation, quiet ply count, repetition map, difficulty, last move, result, RNG. Preserve last good data on write/read failure and rebuild transient workers/GL objects safely.

Acceptance: Progress through actual input, checkpoint, recreate and compare the complete state plus next deterministic legal transition. Home→RESUME restores the defined match; corrupt/failed reads do not silently overwrite recoverable data. Rejected writes remain dirty with truthful status and succeed after recovery; no duplicate score, move or reward.

**2. Provide durable match management and capture explanations.** Add explicit new-match/abandon behavior and explain forced continuations, kings and available draw claims.

Acceptance: New/abandon never silently replaces a retained match; cancel returns to the same state. Forced multi-jump and draw claim affordances match legal state; kings are identifiable.

**3. Finish full-match and interruption coverage.** Exercise actual hosted gameplay and all special/terminal phases.

Acceptance: Complete matches across all difficulties include mid-multi-jump reload, CPU-thinking reload and blocked-player wins. King endgames preserve repetition and quiet-ply draw entitlement.

**4. Establish hosted and physical acceptance.** Validate this compact match on the supported host and physical reference devices; include the shared menu/release repairs.

Apply the common candidate-acceptance requirements in the development lanes above; retain the exact package, device and interruption evidence for this title.

Source anchors: [GameModuleRegistry.java:43](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/java/io/github/mrcalzon02/reverievr/GameModuleRegistry.java#L43); [classic_games_module.cpp:1249](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/classic_games_module.cpp#L1249). Additional scope/issue anchors and exact evidence are in the matrix and workstream reports.

### 11. Pong

**Admission:** Ordinary registry entry. **Stage:** Implemented compact rule loop; finish work remains. [E1, E9]

**Current function:** First-to-eleven human/CPU paddle match with serves, rebound dynamics, scores, winner and restart.

Implemented: Imperfect CPU and swing-dependent bounded rebounds; Pointer/pad paddle movement; Point serves and first-to-eleven terminal loop.

**Save verdict:** In-memory Pause/Resume retains progress; Destroy/Create restores the initial game. Persistent host fixture saw 0 reads and 0 writes in each variant.

**Principal open issues:** Actual module loses progressed match on Destroy/Create; zero save reads/writes. Home→RESUME NATIVE uses this boundary.

**Ordered completion track:**

**1. Preserve complete match state.** Versioned validated bounded save of scores, ball position/velocity, paddles, moving/finished, clock, RNG. Preserve last good data on write/read failure and rebuild transient workers/GL objects safely.

Acceptance: Progress through actual input, checkpoint, recreate and compare the complete state plus next deterministic legal transition. Home→RESUME restores the defined match; corrupt/failed reads do not silently overwrite recoverable data. Rejected writes remain dirty with truthful status and succeed after recovery; no duplicate score, move or reward.

**2. Explain controls and match progression.** Show paddle control method, target score and explicit winner/replay/abandon behavior that respects retained matches.

Acceptance: First screen tells the player how to move/serve and that eleven wins. Both win and loss show a clear result and separate resume/replay/new-match actions.

**3. Finish full-match and interruption coverage.** Exercise actual hosted gameplay and all special/terminal phases.

Acceptance: Both winner paths complete through actual input. Interruption at serve, live rally and final point resumes the declared deterministic safe state.

**4. Establish hosted and physical acceptance.** Validate this compact match on the supported host and physical reference devices; include the shared menu/release repairs.

Apply the common candidate-acceptance requirements in the development lanes above; retain the exact package, device and interruption evidence for this title.

Source anchors: [GameModuleRegistry.java:45](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/java/io/github/mrcalzon02/reverievr/GameModuleRegistry.java#L45); [classic_games_module.cpp:1249](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/classic_games_module.cpp#L1249). Additional scope/issue anchors and exact evidence are in the matrix and workstream reports.

### 12. Snake

**Admission:** Ordinary registry entry. **Stage:** Implemented compact rule loop; finish work remains. [E1, E9]

**Current function:** 12×12 survival Snake with growth, forbidden reversal, wall/self loss, restart and true full-board completion.

Implemented: Food selected from actual free cells; Ordered body and grid rules; Pad/pointer direction interface and restart.

**Save verdict:** In-memory Pause/Resume retains progress; Destroy/Create restores the initial game. Persistent host fixture saw 0 reads and 0 writes in each variant.

**Principal open issues:** Actual module loses progressed match on Destroy/Create; zero save reads/writes. Home→RESUME NATIVE uses this boundary. Executed unchanged production module: clicking the visible UP or DOWN background center starts Snake but leaves desired and actual heading RIGHT (head77→78). Expected UP head65 and DOWN head89 both fail. Fresh-instance clicks in the coded upper/lower hit regions correctly turn UP/DOWN. Rendered centers at y=±1.02,z=−2.97 intersect the input plane at y≈±0.999394, below the ±1.38 activation thresholds.

**Ordered completion track:**

**1. Preserve complete match state.** Versioned validated bounded save of ordered body, length, heading/pending turn, food, score, clock, moving/finished, RNG. Preserve last good data on write/read failure and rebuild transient workers/GL objects safely.

Acceptance: Progress through actual input, checkpoint, recreate and compare the complete state plus next deterministic legal transition. Home→RESUME restores the defined match; corrupt/failed reads do not silently overwrite recoverable data. Rejected writes remain dirty with truthful status and succeed after recovery; no duplicate score, move or reward.

**2. Align visible controls and safe input transitions.** Fix UP/DOWN visual-to-hit mismatch; test rapid turns, held/released input and occupied cells under directional labels.

Acceptance: A ray at each visible UP/RIGHT/DOWN/LEFT button center selects that exact permitted direction. Quick Menu release and close do not cause stale movement or an unintended turn. Loss and board completion have distinguishable cues; controls do not hide necessary occupied-cell information.

**3. Finish full-match and interruption coverage.** Exercise actual hosted gameplay and all special/terminal phases.

Acceptance: Near-full/full board and wall/self collision paths complete with correct result. Recreation at a tick boundary preserves body, direction, food and deterministic next step.

**4. Establish hosted and physical acceptance.** Validate this compact match on the supported host and physical reference devices; include the shared menu/release repairs.

Apply the common candidate-acceptance requirements in the development lanes above; retain the exact package, device and interruption evidence for this title.

Source anchors: [GameModuleRegistry.java:47](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/java/io/github/mrcalzon02/reverievr/GameModuleRegistry.java#L47); [classic_games_module.cpp:1249](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/classic_games_module.cpp#L1249). Additional scope/issue anchors and exact evidence are in the matrix and workstream reports.

### 13. Reversi

**Admission:** Ordinary registry entry. **Stage:** Implemented compact rule loop; finish work remains. [E1, E9]

**Current function:** 8×8 human/CPU Reversi with legal markers, flips, forced passes, disk totals and correct both-blocked end.

Implemented: Multi-direction captures; Automatic forced pass; Win/loss/draw even if empty squares remain; bounded CPU.

**Save verdict:** In-memory Pause/Resume retains progress; Destroy/Create restores the initial game. Persistent host fixture saw 0 reads and 0 writes in each variant.

**Principal open issues:** Actual module loses progressed match on Destroy/Create; zero save reads/writes. Home→RESUME NATIVE uses this boundary.

**Ordered completion track:**

**1. Preserve complete match state.** Versioned validated bounded save of board, side, passes, ply, result, RNG. Preserve last good data on write/read failure and rebuild transient workers/GL objects safely.

Acceptance: Progress through actual input, checkpoint, recreate and compare the complete state plus next deterministic legal transition. Home→RESUME restores the defined match; corrupt/failed reads do not silently overwrite recoverable data. Rejected writes remain dirty with truthful status and succeed after recovery; no duplicate score, move or reward.

**2. Make turn exceptions and match management explicit.** Explain automatic pass, terminal score and new-match behavior; verify legal-square marker contrast.

Acceptance: A consecutive turn caused by forced pass is explicitly explained. Final disk totals/result are clear and New requires deliberate action. Legal markers are recognized and hit accurately in both eyes.

**3. Finish full-match and interruption coverage.** Exercise actual hosted gameplay and all special/terminal phases.

Acceptance: Full games include forced-pass chains and both-blocked ending with vacant squares. Restoration at pass and terminal states does not change turn ownership or disk totals.

**4. Establish hosted and physical acceptance.** Validate this compact match on the supported host and physical reference devices; include the shared menu/release repairs.

Apply the common candidate-acceptance requirements in the development lanes above; retain the exact package, device and interruption evidence for this title.

Source anchors: [GameModuleRegistry.java:49](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/java/io/github/mrcalzon02/reverievr/GameModuleRegistry.java#L49); [classic_games_module.cpp:1249](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/classic_games_module.cpp#L1249). Additional scope/issue anchors and exact evidence are in the matrix and workstream reports.

### 14. Breakout

**Admission:** Ordinary registry entry. **Stage:** Implemented compact rule loop; finish work remains. [E1, E9]

**Current function:** Two-wall, three-life Breakout with row scores, serves, speed/paddle changes and fixed-step ball physics.

Implemented: 14×8 brick mask; Fixed .01-second simulation tick; Wall progression, life loss and two-wall win.

**Save verdict:** In-memory Pause/Resume retains progress; Destroy/Create restores the initial game. Persistent host fixture saw 0 reads and 0 writes in each variant.

**Principal open issues:** Actual module loses progressed match on Destroy/Create; zero save reads/writes. Home→RESUME NATIVE uses this boundary.

**Ordered completion track:**

**1. Preserve complete match state.** Versioned validated bounded save of brick mask, wall, lives, score, paddle/ball/velocity, fixed tick remainder, paddle hits, narrow/top-row flags, result. Preserve last good data on write/read failure and rebuild transient workers/GL objects safely.

Acceptance: Progress through actual input, checkpoint, recreate and compare the complete state plus next deterministic legal transition. Home→RESUME restores the defined match; corrupt/failed reads do not silently overwrite recoverable data. Rejected writes remain dirty with truthful status and succeed after recovery; no duplicate score, move or reward.

**2. Clarify the court and terminal transitions.** Verify isolated remaining bricks and ball visibility, serve/life/wall cues, winner/loser and replay decisions.

Acceptance: Sparse brick configurations remain visually identifiable. Life loss, next wall and final result each show the correct state without consuming a stale click.

**3. Finish full-match and interruption coverage.** Exercise actual hosted gameplay and all special/terminal phases.

Acceptance: Both walls and exhausted-lives paths finish through actual control. Interruption between life loss/serve and wall changes preserves complete brick mask and run state.

**4. Establish hosted and physical acceptance.** Validate this compact match on the supported host and physical reference devices; include the shared menu/release repairs.

Apply the common candidate-acceptance requirements in the development lanes above; retain the exact package, device and interruption evidence for this title.

Source anchors: [GameModuleRegistry.java:51](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/java/io/github/mrcalzon02/reverievr/GameModuleRegistry.java#L51); [classic_games_module.cpp:1249](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/classic_games_module.cpp#L1249). Additional scope/issue anchors and exact evidence are in the matrix and workstream reports.

### 15. Chutes & Ladders

**Admission:** Ordinary registry entry. **Stage:** Implemented compact rule loop; finish work remains. [E1, E9]

**Current function:** 100-square human/CPU chance game with nine ladders, ten chutes, die rolls, exact finish, overshoot and restart.

Implemented: Single authoritative chute/ladder table; Exact landing at100; overshoot forfeits; Human/CPU turns and deterministic full-match rule tests.

**Save verdict:** In-memory Pause/Resume retains progress; Destroy/Create restores the initial game. Persistent host fixture saw 0 reads and 0 writes in each variant.

**Principal open issues:** Actual module loses progressed match on Destroy/Create; zero save reads/writes. Home→RESUME NATIVE uses this boundary.

**Ordered completion track:**

**1. Preserve complete match state.** Versioned validated bounded save of positions, last die, turn, CPU delay clock, winner, RNG. Preserve last good data on write/read failure and rebuild transient workers/GL objects safely.

Acceptance: Progress through actual input, checkpoint, recreate and compare the complete state plus next deterministic legal transition. Home→RESUME restores the defined match; corrupt/failed reads do not silently overwrite recoverable data. Rejected writes remain dirty with truthful status and succeed after recovery; no duplicate score, move or reward.

**2. Explain roll and path outcomes legibly.** Make roll→landing→link→final square and exact-finish rule comprehensible; improve dense numerals/endpoints as needed.

Acceptance: A player can identify the rolled number, landing square, link effect and final square. Chutes/ladders endpoints and exact-finish outcomes are readable in both eyes.

**3. Finish full-match and interruption coverage.** Exercise actual hosted gameplay and all special/terminal phases.

Acceptance: Complete matches cover overshoot, shared square, every transition class, both winners and replay. Reload during CPU delay preserves the pending random outcome/turn instead of granting a reroll.

**4. Establish hosted and physical acceptance.** Validate this compact match on the supported host and physical reference devices; include the shared menu/release repairs.

Apply the common candidate-acceptance requirements in the development lanes above; retain the exact package, device and interruption evidence for this title.

Source anchors: [GameModuleRegistry.java:53](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/java/io/github/mrcalzon02/reverievr/GameModuleRegistry.java#L53); [classic_games_module.cpp:1249](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/classic_games_module.cpp#L1249). Additional scope/issue anchors and exact evidence are in the matrix and workstream reports.

### 16. Test Chamber

**Admission:** Development-only. **Stage:** Development utility alpha. [E3, E9]

**Current function:** Native material/light/audio showcase and bounded three-project primitive VR workshop.

Implemented: 30 authored REVSCENE parts plus avatar/wardrobe; Up to 48 editable pieces and 16 undo steps; three slots/64KiB bound; Move/rotate/scale/add/delete/save/play; Persisted motion/light/sound authoring bundles and validated format upgrades.

**Save verdict:** Three bounded workshop slots and scene/motion/light/sound bundles. Failed saves retain dirty edits while the module lives and block project-slot/edit-mode switches. Pause and Destroy attempt a save and log failure, but Destroy still deletes state after a rejected write, so unsaved edits can be lost on Home/exit. Startup always loads slot 0; its corruption/I/O failure prevents entry to valid other slots.

**Principal open issues:** Open always loads slot 0 and module Create fails on its corruption/I/O failure; inaccessible selector prevents reaching valid other slots. Pause and Destroy only attempt a save. Destroy deletes the module even after a rejected final write; dirty in-memory edits are not retained across Home/exit. Provide truthful pre-exit status and a safe recovery/abandon path.

**Ordered completion track:**

**1. State the stable utility scope.** Describe native showcase and bounded primitive editor consistently; distinguish unimported companion GLB/WAV material.

Acceptance: User-facing claims match integrated 30-part scene, three project slots, 48-piece/16-undo editor and actual modes. No implied general-purpose asset importer or campaign ending is required.

**2. Provide non-destructive startup recovery.** Allow alternate slot selection/backup/explicit reset when first project cannot load; retain fail-before-switch and visible save state. Ensure ordinary exit exposes an unresolved write and supports a deliberate safe recovery/abandon choice; never imply in-memory edits survive destruction.

Acceptance: A corrupt/unreadable slot 0 does not prevent access to valid slots 1/2. Original invalid bytes remain available until explicit replacement; failed exit writes show truthful recovery status. Rejected final writes cannot silently claim that destroyed in-memory edits are retained; the last durable project remains recoverable and ordinary exit presents the supported recovery choice.

**3. Complete actual editor interaction coverage.** Exercise move/rotate/scale/add/delete/undo, projects, save/play and motion/light/audio pages through the module UI.

Acceptance: Every documented editor action produces the intended persisted scene/bundle change. Switching/exit fails safely while dirty data cannot be saved; successful recovery restores exact authored work.

**4. Finish per-eye interaction clarity.** Fit labels/selection/page controls and verify distinct audio/material/light modes.

Acceptance: Showcase/editor/selection/motion/light/audio/empty/error/dirty states are readable and reachable in both eyes. Each audition mode has observable output and reliable feedback without repeated side effects per eye.

**5. Establish hosted and physical acceptance.** Validate stable utility creation/edit/save/recovery and launch/exit/context flow in the same APK; no campaign-style completion requirement is added.

Apply the common candidate-acceptance requirements in the development lanes above; retain the exact package, device and interruption evidence for this title.

Source anchors: [GameModuleRegistry.java:55](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/java/io/github/mrcalzon02/reverievr/GameModuleRegistry.java#L55); [reverie_vr_workshop.h:95](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/reverie_vr_workshop.h#L95); [test_chamber_module.cpp:748](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/test_chamber_module.cpp#L748). Additional scope/issue anchors and exact evidence are in the matrix and workstream reports.

### 17. Sitka

**Admission:** Unregistered. **Stage:** Unregistered integration project. [E3, E9]

**Current function:** Substantial source terrain/walking/fishing/economy/boat systems with synthetic module tests; no admitted packaged native world.

Implemented: Tier0 recovery supplies/infinite basic bait; retained fish/shellfish sale; Pending finds/token registration;48 gear levels and four functional training tracks; Hull/fitting economy and source navigation/boat systems; Tested dirty-save retry and read-only two-eye rendering with synthetic fixtures.

**Save verdict:** Clean-start SITKA-FISH-5 journal distinguishes absent/invalid/I/O reads, retains dirty changes and visible failure, retries on Pause and reopens without granting starter stock again. Exact position/active cast/boat/world pots are not established.

**Principal open issues:** Native Create needs approved real world inputs that are not assembled/admitted in the APK. Physical fishing presentation, real-world offices/avatar/characters, refined boat model adoption and actual-world helm/docking, pot crabbing/Deckwork integration remain incomplete. -O1 -Werror promotes shared shader diagnostic snprintf truncation to error. No Android build failure inferred.

**Ordered completion track:**

**1. Admit one real connected world slice.** Assemble approved bounded terrain/water/fishing/harbor/desk assets and register build/host/gallery together.

Acceptance: Packaged native Create succeeds on real approved world files; entry/menu/exit/recovery work. No inert playable icon is exposed before the actual route is usable.

**2. Finish the shore journey in that world.** Connect arrival/walk/cast/land/retain/sale/register/unlock/purchase; implement physical fishing presentation and coherent sound.

Acceptance: Player completes the whole shore economy journey through actual spatial controls. Reopen retains journal/wallet/gear; interrupted transactions conserve money and stock.

**3. Integrate intended boat and crabbing systems.** Adopt refined hull assets/collision/mounts/helm, embark/dock, sounder/bathymetry, pots/bait/catch/Deckwork in one world/journal/wallet.

Acceptance: Owned hull/fitting effects and actual-world embark/helm/dock work. Pots and catch/sale cannot be harvested twice or lost across interruption.

**4. Complete world/content recovery and QoL.** Deliver walkable offices, visible character/avatar representation, progression teaching/navigation and safe return across world/save changes.

Acceptance: Reachable prerequisites and identity records support retained progression without forced lockouts. Infinite basic Tier0 recovery remains available; state returns safely when world/save data cannot be used.

**5. Establish hosted and physical acceptance.** Run final admitted world, shore/boat/crabbing routes, resource/lifecycle/corrupt-save cases and physical controller/stereo/performance acceptance. A shore-only product needs explicit scope agreement.

Apply the common candidate-acceptance requirements in the development lanes above; retain the exact package, device and interruption evidence for this title.

Source anchors: [SITKA.md:3](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/docs/project/SITKA.md#L3); [sitka_terrain_module.cpp:1095](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/sitka_terrain_module.cpp#L1095); [sitka_fishing_session.h:31](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/main/jni/native/sitka_fishing_session.h#L31); [sitka_starter_loop_test.cpp:75](https://github.com/mrcalzon02/ReverieVR/blob/12d4018c90e56a3767b8ffc58a2e5394440caec1/app/src/test/native/sitka_starter_loop_test.cpp#L75). Additional scope/issue anchors and exact evidence are in the matrix and workstream reports.
