# Omega Front — Hearthfall Strategy and Tactical Implementation

**Implementation state:** host-tested combat, six-suite C++17 regression runner, 16-sector GLES2 campaign map, actual battle deployment, after-action consequences and a versioned full-frontier save are source-integrated in the Development-only native module. **No Android NDK/APK build or physical Galaxy S9/Daydream input, stereo or thermal acceptance has been run for this change.**
**Program:** RV-GAME-06; first bounded implementation toward P1 (Hearthwell waterworks defense).
**Authority:** `app/src/main/jni/native/omega_front_tactics.h` is the game-specific C++17 rules source. This is NOT a shared ReverieVR SDK helper or an ABI change.

## Implemented now

- Fixed 24×24 logical tactical grid, with road, rubble, wall, deep water, and ordinary ground; walls block fire and deep water blocks foot movement.
- Eight combatants in the default Hearthwell seed fixture (four defenders, four attackers); per-side unit identity, HP, ammunition, armor and three distinct roles (rifle, engineer, heavy).
- Input-facing bounded per-unit FIFO order queues: move, attack, repair the pump and resupply at the ammunition crate; order cancel; invalid/hostile orders rejected. Tactical pause halts the clock while still allowing the player to select soldiers, change command modes and queue orders; deselecting a unit exposes secondary/App resume.
- Deterministic cardinal BFS occupancy-aware movement, seeded attack variance, target-range checks, shared Bresenham line of sight, rubble damage reduction, finite ammo, finite repair materials and depot rounds.
- Faction behavior now depends on the real mission: raiders attack the waterworks, the Hearthward orchard watch protects its capture site, and a receivership field engineer guards the radio relay. Each AI still shoots only visible opponents and spends finite ammunition.
- A win requires all hostile troops disabled while defenders and the pump survive; losses include pump destruction, all defenders lost, or the bounded 160-pulse encounter timeout.
- Simulation advances **only through `Battle::Step()`** (one fixed 250-ms logical pulse); reading ground, unit status or deterministic checksum does not mutate state. Ended battles are frozen.
- Seed/state/terrain/units/queues/stock/result fields contribute to a stable explicit-field FNV-1a checksum for replay comparisons.

## Persistent Hearthwell tactical checkpoint — implemented 2026-10-15

`omega_front_checkpoint.h` is a versioned, deterministic **1,152-byte** save codec for one live 24×24 battle. It writes explicit little-endian integer/float fields and an FNV-1a-64 integrity trailer; it stores seeded RNG, fractional pulse clock, all 576 ground tiles, troops and HP/ammo, pending order queues, waterworks health, depot stock, result/event state, cursor, selection, pause and restart seed. Decoder checks length, magic/version, checksum, enums, coordinates, unit occupancy, terrain, inventories, queue validity and battle outcome **before** assigning into live state. Missing/corrupted/incompatible checkpoint data never partially replaces the in-memory battle.

The old standalone combat proof saved to `hearthwell.v1`; the active native module now reads/writes **`frontier.v1`** instead. Its complete 16-sector strategic ledger includes the nested tactical state, saved immediately after accepted field actions or orders, pause and finished battles, every 16 logical battle pulses, and native Pause. Both rendered eyes remain read-only, and failed/corrupt saves are logged and visibly identified. The old skirmish file is not silently interpreted as a larger campaign. This is **one persistent first-frontier slice**, not the full 70-arc world.

The first three host suites cover tactical rules, controller input and the original skirmish checkpoint; the current runner additionally covers the 16-sector frontier, strategic session and versioned full-campaign checkpoint, **six suites total**. Tests include deterministic continuation, after-action conservation, paused planning, corruption and version rejection; GCC/Clang and relevant sanitizer checks passed locally, plus a mock native save/destroy/recreate/load and GLES lifecycle test. **Android NDK, APK assembly, Galaxy S9 actual saves and Daydream input are not yet verified.**

## First Hearthfall March strategic simulation — implemented headless slice

`omega_front_frontier.h` now maintains the **16 specifically named T01-S001–S016 sectors** with current/prior ecology, legal owner (0 neutral, 1–17 principal factions, **18 the independent player settlement, not an eighteenth major faction**), population, finite food and water, salvage mass, stability, local claims and anomaly flags. Its deterministic first-cycle opening holds a 31-person Hearthwell foothold, 12 available guards, 24 field rations and finite stores. It implements four-direction strategic movement, inaccessible initial Barren sectors, a severed S011–S015 viaduct repairable from S015 using salvage, resupply through physical sector food stores, troop recruitment and infrastructure repair, and unsupplied retreat with a trust penalty rather than a route softlock.

A **forecast** on day one identifies Rust Orchard S009 as the day-four morphological change: its prior Wasteland condition becomes Eden. It begins with **zero manufactured food, water, or settlers**; movement of the landscape does not create stockpiles. Day advancement consumes Hearthwell's real local food/water and transfers existing S011 reserves through working pumps; shortage reduces civilian trust. The system is one deterministic, bounded authored event, **not** the intended 600-sector morphology simulator.

The opening arc **The Water We Own** now has three different constructed and executable `Battle` fixtures owned by the strategic instance, all using the actual 24×24 combat rules and bounded finite supplies: M01 defend S011 Lantern Pump against unaffiliated raiders; M02 contest S006 White Orchard against a hostile F01 Hearthward watch; M03 return to S011 to repel a further F01 receivership assault. Each operation requires physically reaching the target sector and spending field rations. Only a **genuinely terminal** tactical Battle may resolve, and repeated resolution pays nothing. Real casualties reduce available guard manpower; wreck salvage is derived from actually destroyed attackers; captured/relieved food is **transferred out of** the supplying sector rather than duplicated; pump integrity and consequences persist within the in-memory strategy session. Victories and defeats adjust civic trust, territorial claims, and mission progress. The tactical kernel now executes **three distinct objectives** rather than declaring all victories through pump defense; the authored encounters, political consequences and saved campaign progression remain intact. Full faction force variety, unique sprites, and strategic AI are still pending.

`app/src/test/native/omega_front_frontier_test.cpp` compiles against the **actual** tactical header and verifies travel/bridge/initial environment, resource conservation, seeded battle resolution, lack of duplicate mission rewards, casualties, two linked successful operations and a genuinely difficult third operation, and ecology-class transitions. The updated `scripts/verify-omega-front-tactics.sh` runs this alongside the three preexisting host suites. Strict GCC/Clang checks and GCC ASan/UBSan passed locally. **This strategic simulation is now source-connected to the native VR module and a separate versioned campaign save, but has no completed 70-arc campaign or Android/Daydream device validation.** It remains a bounded authored first-arc prototype, not the completed 70-campaign game or a device-verified experience.

## Native strategic-to-tactical gameplay integration (source implemented)

Omega Front now starts in the real **16-sector Hearthfall March** rather than an isolated pump skirmish. A compact 4×4 GLES2 map shows sector ecology, territorial ownership, troop location, highlighted cell, resource stocks and claim pressure; above/below it, the player sees day, field guards, rations, salvage, trust, next operation target and forecast day-four Eden emergence. Touchpad or normalized gaze highlights a sector without clicking. Select marches to a reachable neighboring sector or performs a site action. Secondary/App cycles five real site actions: **Deploy, Forage, Recruit, Repair, Wait**. Each action changes the actual `Frontier` ledger or fails visibly; the shell retains Home/recovery.

Deploy requires the real target sector, four guards, rations and serviceable waterworks. It opens the `Frontier::StartOperation` battle in the 24×24 tactical view. When the real combat outcome becomes terminal, Select applies `ResolveOperation` once: real troop casualties, finite recovered wreck material, transferred orchard supplies, pump damage, civic trust and regional claims persist, and the player returns to the map. The three operations remain within first arc **The Water We Own**, now with distinct victory rules and objective markers: eliminate pump attackers; secure and hold a contested orchard position for three pulses; or bring an engineer adjacent to a guarded relay and expend two field components while defending the pump. These rules are real simulation state, not UI-only labels. The other 69 faction arcs remain unimplemented.

The module's current host-owned game save is **`frontier.v1`**, a versioned, 2,048-byte record from `omega_front_campaign_save.h`. It combines all 16 sectors and campaign flags with the existing nested deterministic tactical checkpoint, including battle orders, units and a partial turn. Endian-safe encoding, a hash trailer, semantic field checks and transactional restoration reject corrupt and incompatible records without modifying the live session. Field actions/commands and pause/end changes save; combat additionally autosaves every 16 logical 250-ms pulses, and native Pause writes a checkpoint. The former standalone `hearthwell.v1` combat-fixture slot is **not** silently treated as a fully fledged strategic campaign. Neither eye writes saves or runs the simulation.

The host runner now executes eight strict C++17 suites: tactics, controls, original combat checkpoint, frontier, strategic session, strategic campaign checkpoint, authored mission objectives, and faction doctrine/specialist/migration regressions. GCC and Clang passed locally; Clang ASan/UBSan passed the two new suites. A local mock GLES2/ABI lifecycle also verified map travel, battle deployment, update/render separation, module save/destroy/recreate and graphics-context restart. **No Android NDK compiler, APK, Galaxy S9 headset or physical Daydream validation was available for these checks.** The projected 600 sectors and 70 major campaign arcs remain future work.

## Three distinct tactical win conditions — implemented and regression-tested

**M01: The Water We Own / Lantern Pump.** Defeat actual attackers before they destroy the waterworks or eliminate the last defender. The original pump-defense rules, enemy target priorities and campaign result processing remain intact.

**M02: White Orchard seizure.** The attacker formation is physically placed to protect the orchard instead of marching toward the pump. A Hearthwell soldier must occupy grid **(17,12)** and hold it for **three consecutive 250-ms pulses** with no living opponent at Manhattan distance one or less. Killing all hostile troops alone **does not complete the operation**: an actual soldier must secure the site, under the existing 160-pulse mission limit. The marker and 0–3 progress are rendered in the tactical war table.

**M03: Receivership command relay.** A hostile field engineer guards the radio installation at **(18,12)** while other receivership troops attack the pump. A surviving friendly **engineer** must approach within one Manhattan tile of the relay; if no enemy stands within one tile, they expend **two real engineering components** across two combat pulses to disable it. Killing every enemy alone does not substitute for engineering work, and destruction of the pump still defeats the player. The relay has a visible marker and progress/parts status.

The gameplay owns a typed `Mission` and serialized objective progress; all three win conditions use `Battle::Step` deterministically and are frozen after a terminal result. Checkpoint **v2** writes mission/progress in the same 1,152-byte fixed record. Prior **v1** pump-defense checkpoints still load as pump defenses without inventing a newly completed objective; nested v2 campaign saves reject a mission ID that disagrees with the currently deployed arc. The third encounter includes up to six enemies and the campaign report validator correctly supports six destroyed hostiles.

New `omega_front_objectives_test.cpp` confirms genuinely distinct wins, contested suppression, correct component consumption, defeating enemies without automatic objective victory, seeded save/resume, v1 compatibility, and corrupt/resealed cross-mission rejection. The same one-command runner now executes **seven** host regression suites; the module's original mock GLES2 lifecycle still passes. These remain **host-static acceptance**, not Android APK or Galaxy S9 hardware verification.

## Limited faction doctrines and specialist units (source-implemented)

Four proposed faction identities now have **actual, deterministic combat behaviors**, rather than mere color swaps:

- **F01 Hearthward Watch:** existing first-arc orchard garrisons hold the contested orchard rather than rushing the distant pump. The three first-arc tactical missions are source-connected to this behavior; newly created M02/M03 encounters are now marked `Hearthward` in simulation state.
- **F02 Meridian Security:** `Sentinel` specialist (12 HP, 6 finite rounds, seven-tile range and two armor). Security fire prioritizes *visible, in-range* engineers over closer riflemen, but line-of-sight walls still prevent cheating. This doctrine can be built and stepped in a deterministic combat fixture; **no Meridian campaign arc is yet accessible in the VR strategic map**.
- **F04 Scrapline Crew:** `Salvager` specialist (8 HP, starts with two of six possible rounds) moves to the actual ammunition crate and can steal one existing round per logical pulse while in reach. `ammo_stock` decreases, carried rounds increase, and a bounded stolen-round ledger is persisted. Scrapline cannot summon equipment from a timer or steal without reaching the depot. **Its campaign encounters remain headless previews**.
- **F06 Lost Ninth Cohort:** `Legionary` specialist (14 HP, armor, one-tile melee and zero firearm rounds) closes on *visible* defenders and receives two additional defense against incoming damage while adjacent to another living legionary. Shield-wall benefits disappear as formations break. Legionaries can attack without fake bullet expenditure; friendly legionary melee orders are valid but their ammunition resupply is not. **The Lost Ninth remains a preview fixture rather than an available campaign arc**.

`omega_front_faction_encounters.h` builds four distinct, reproducible tactical encounter fixtures from the same combat kernel. The active Development-only GLES2 module renders separate original procedural silhouettes for salvagers (recovery pack), Sentinels (helmet/visor) and legionaries (shield/sword), faction palette and real saved doctrine label. **Only the Hearthward doctrine is currently used in the three player-facing Hearthfall missions.** Rendering an unfamiliar faction in the generic battle table does not unlock its campaign.

The original 1,152-byte tactical checkpoint writes **schema v3**, adding doctrine and the finite stolen-round count without storing C++ struct padding or increasing the slot. Version 1 pump-only and version 2 three-mission saves still decode with their original unaligned AI, and a version-3 battle validates role HP/ammo, legal doctrine IDs, stolen-stock conservation and original win-state constraints transactionally. The enclosing `frontier.v1` strategic snapshot retains its size and independently checks its nested tactical record. Unit HP bars now scale to each role's actual maximum HP. This slice passes eight GCC/Clang strict C++17 regression suites plus the faction ASan/UBSan run and the mock GLES2 module render/save/restart test; **Android NDK linking and Galaxy S9/Daydream device tests remain pending**.

## Reproduce host checks

From the repository root:

```sh
bash scripts/verify-omega-front-tactics.sh
```

The runner builds and runs all eight independent gameplay/strategy/save suites under GCC or Clang (set `CXX=clang++`) with strict C++17 warnings. It does **not** access GitHub Actions. Additional optional sanitizers were run locally during implementation:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Iapp/src/main/jni/native app/src/test/native/omega_front_tactics_test.cpp \
  -o /tmp/omega_front_tactics_test && /tmp/omega_front_tactics_test
```

**Observed local checks on 2026-10-15:** g++ and clang++ host runners both passed; g++ ASan+UBSan passed. Remote source and test file blob SHAs were compared against exactly these locally compiled source files.

## What this deliberately does NOT prove

The Android.mk library `reverie_module_omega_front` and native host `omega-front` allowlist are implemented with `reverie_native_module_entry_v1`, a procedural GLES2 board and real battle-linked input. This remains **source-integrated only** until Android NDK/APK and device checks pass. No 70 campaign missions, progression, seventeen factions' strategic AI, world morphology, sector generator, saves, chibi atlases, civilian population, proper fog of war or accurate dynamic faction logistics are implemented by this kernel. This is only one real combat fixture and its underlying minimal combat rules.

The queued attack-order visibility rule uses current unit coordinates at order time, with a nine-tile local visibility radius and occluding walls; full faction-wide fog memory, sensor intel and ranged weapons are separate follow-up tasks.

The initial fixed sides are `Hearthwell` and `Raiders`, and the default scenario is a tactical **test fixture**, not an assertion that the player has already fought each of the canon 17 factions.

## Next technical gates

1. Prove that the registered `omega-front` native shared library links and packages in both Android ABIs, then confirm Daydream touchpad navigation, shell Home/recovery, readable board and stereo GL state on a physical S9.
2. Adapt command UI and tile cursor to seated 3DoF head tracking, with readable closest-range board placement and reliable pause/route inspection.
3. Connect the implemented Meridian, Scrapline and Lost Ninth doctrine previews to authored consequential operations, then expand the first-arc 16-sector campaign toward 600 sectors with reconnaissance/fog, civilian systems, connected depots, diplomacy and additional morphology events.
4. Run actual APK/NDK integration and signed build verification separately. Test on Galaxy S9/Daydream in short and sustained sessions before claiming device admission.

**Acceptance boundary:** the 16-sector campaign, linked first three tactical operations and full-frontier save are source/host-tested; Android NDK/APK, physical headset/controls, the remaining 584 sectors and 69 campaign arcs are incomplete or unverified.
