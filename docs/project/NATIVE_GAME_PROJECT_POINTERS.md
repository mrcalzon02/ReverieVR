# Native Game Project Pointers

**Status:** foundation implementation started; user-visible launch remains deferred until the native interaction/runtime gates below are satisfied.

**Purpose:** preserve the intended direction for small, deliberately pre-PSX-scale native games derived from existing project concepts without turning ReverieVR into a general-purpose modern game engine.

These are **new ReverieVR-native adaptations**, not ports of existing desktop builds and not ROM/libretro content. They should use the versioned native module lifecycle defined by RV-0400 and consume shell-owned stereoscopic rendering, input, recenter/recovery, global comfort settings, logging, saves, and performance controls.

## Shared scope rule — "pre-PSX native"

"Pre-PSX" is a deliberate production ceiling, not a requirement to imitate one exact historical machine.

The content layer should favor:
- very low polygon counts and small authored environments;
- texture atlases, nearest/low-cost filtering where aesthetically appropriate, vertex color and baked/static lighting;
- small numbers of active actors with simple, readable state machines;
- short draw distances, aggressive culling, fog, portals/rooms, and authored visibility control;
- crunchy/low-rate source assets where stylistically useful while the ReverieVR compositor and text/UI remain readable at headset resolution;
- no PBR requirement, no expensive dynamic shadows, no large seamless worlds, and no "modern game first, optimize later" architecture;
- seated/controller-first play compatible with a Daydream-class 3DoF controller, with gaze fallback only where the shell contract permits it;
- one compact vertical slice before campaign-scale simulation is considered.

The target is not nostalgia for its own sake. The ceiling exists so these games remain thermally sustainable on the Galaxy S9 while still gaining the spatial presence, scale, and hand interaction that make native VR worthwhile.

## RV-GAME-01 — Between Deliveries: The Red Ledger VR

**Source concept:** *Between Deliveries*, a slow-burn economic survival game in the IronSight universe centered on operating a debt-financed bar in the contested Barforsograd/Mininska region.

### Native VR identity

This should be the first serious native ReverieVR game candidate because the core loop already wants to happen at arm's length.

The player occupies the bar physically from a mostly fixed/seated working position. Customers enter, order, argue, leave mess, bring rumors, attract inspectors, and occasionally bring trouble. The player's hands/controller operate taps, bottles, cups, a cash drawer, ledger, radio, switches, and basic cleaning/repair interactions.

The game is not an action shooter disguised as a bar simulator. Its pressure comes from **margin, scarcity, reputation, protection, faction attention, and the fact that every improvement makes the establishment more valuable to somebody else**.

### First playable slice

Build only the ground-floor opening state:
- one concrete room;
- one working tap;
- one stool;
- mismatched cups;
- flickering light;
- empty backroom with ledger/mattress;
- a tiny supplier catalogue;
- a handful of recurring patron archetypes;
- one protection payment cycle;
- one inspection event;
- one supply interruption;
- a simple day-close ledger.

A successful prototype must prove that manually serving drinks, handling money/stock, reading the room, and making uncomfortable business choices is compelling before adding upper floors, private rooms, card games, laundering, or faction infrastructure.

### Reusable technology it should exercise

- grab/use interaction abstraction over 3DoF controller input;
- fixed work-surface interaction and reach assistance;
- lightweight NPC queueing/seating/state machines;
- inventory containers and consumable stock;
- diegetic ledger/menu UI;
- persistent economic state;
- small-room occlusion/culling;
- positional voice/ambient audio;
- event-driven narrative state without cinematic dependency.

### Scope guard

Do not begin with the complete supplier/faction/cast catalogue. Do not implement the second floor until the one-room economy produces meaningful decisions.

---

## RV-GAME-02 — Ministry of Intelligence: Lantern Desk VR

**Current implementation boundary:** a deterministic pure C++ evidence kernel now implements sector assignment, sortie completion, procedurally rasterized 16×16 photos, an Orchard civilian false-positive candidate, graded assessments and operational consequences. The public photo exposes observations, not hidden World Truth. A new Development-only native GLES2 board now connects these observations to sector selection and report filing; no full desk, annotations, persistence, signed APK proof or device acceptance yet.

**Source concept:** *Ministry of Intelligence*, the standalone intelligence-command game formerly developed as *Second Lantern*. It must remain separate from older IronSight material.

### Native VR identity

The player is an intelligence commander and analyst, **not a battlefield commander**. ReverieVR should turn the farmhouse headquarters into a physical intelligence workspace: maps, reconnaissance photographs, dossiers, radio traffic, mission slips, pins/markers, and reports arranged around a seated desk.

The essential invariant remains:

**World Truth -> physical manifestation -> reconnaissance capture -> imperfect evidence -> player interpretation -> assessment/report -> war effect.**

The VR adaptation should preserve the separation between hidden World Truth and Ministry Knowledge. The player's advantage comes from learning to recognize patterns in evidence, not from the UI quietly revealing the simulation.

### First playable slice

Implement one small operational area and one short intelligence cycle:
- choose among several accessible sectors;
- assign the damaged Pattern aircraft to a reconnaissance pass;
- receive a low-quality image;
- inspect it physically on the desk with zoom/magnification assistance;
- annotate or tag observed evidence;
- compare against a tiny reference dossier;
- file **NO CONTACT**, general contact, unit-type, or exact identification;
- advance the hidden situation and show the consequence of report accuracy.

The first scenario should support at least one learnable visual signature and one benign/misleading scene, such as the established treehouse-scale tutorial concept.

### Reusable technology it should exercise

- high-resolution diegetic documents inside a low-complexity 3D room;
- image inspection, pan/zoom, markup, pinning, sorting, and comparison;
- map-sector interaction;
- provenance-aware observation records;
- save separation between hidden simulation state and player-known intelligence;
- time advancement and asynchronous mission-return events;
- lightweight dynamic war state that does not require rendering the battlefield in real time;
- advisor/radio presentation that assists without becoming an answer key.

### Scope guard

Do not convert it into a tactical RTS. Do not expose World Truth through convenience UI. The first native version needs enough simulation to generate honest evidence, not the entire eventual continental war.

---

## RV-GAME-03 — Iron Sight: Forward Detachment VR

**Current implementation boundary:** a deterministic pure C++ patrol kernel now exercises main-road versus ridge-route resource costs, ambiguous observable contact clues, action consequences, safe return and once-only debrief. A new Development-only native GLES2 choice board now exposes route selection, observations, decisions, return and debrief; no 3D sector, squad handling, persistence, signed APK proof or device acceptance yet.

**Source concept:** *Iron Sight*, centered on a forward reconnaissance detachment operating in the ambiguous Barsofagrod conflict: patrols, route security, reconnaissance, interdiction, logistics, liaison, ROE, uncertain civilians, deception, IED risk, and survival through discipline rather than indiscriminate combat.

### Native VR identity

The pre-PSX ReverieVR adaptation should **not** attempt the whole campaign first. It should be a compact reconnaissance/patrol game built around one forward operating area and a small number of vehicles and dismounts.

The player operates as the lead element of a small reconnaissance detachment. The interesting loop is:

**brief -> prepare -> patrol/recon -> identify uncertainty -> decide whether to engage/avoid/report -> return -> debrief -> repair/resupply -> next task.**

Combat exists, but information, route choice, fuel/ammunition, vehicle condition, civilian handling, and positive identification should matter as much as marksmanship.

### First playable slice

Use one compact sector containing:
- the holding area/JON support point;
- one road/rail approach;
- one checkpoint or bridge objective;
- one abandoned industrial/settlement pocket;
- one patrol route with an alternate bypass.

Provide one APC or scout vehicle, a very small dismount team represented with simple AI, a radio/report interface, and a handful of encounter types:
- empty route / negative reconnaissance;
- suspicious civilian or false-civilian contact;
- IED indicator requiring halt/report/bypass;
- small hostile scout element;
- damaged infrastructure;
- friendly liaison encounter.

Success should depend on bringing back accurate information and preserving the detachment, not merely maximizing kills.

### Reusable technology it should exercise

- low-poly outdoor rendering with fog and hard visibility limits;
- simple drivable vehicle or constrained vehicle navigation;
- squad follower/state abstraction;
- line-of-sight and identification states;
- radio/report UI;
- lightweight damage, fuel, ammunition, and repair state;
- authored encounter director with uncertainty/deception;
- mission briefing/debrief loop;
- persistent sector consequences without a seamless open world.

### Scope guard

No full ~80-person formation simulation for the first native game. No giant combined-arms battlefield. No attempt to model every JON function, faction, logistics chain, drone, artillery system, and liaison office at once.

---

## RV-GAME-04 — Breakwater Battery VR

**Canonical spec:** docs/project/BREAKWATER_BATTERY_VR.md. Prioritized stationary coastal-defense game with day/night/location campaigns and inter-wave purchases. First enemies are one-passenger rowboats, followed by 3–6-person military landing craft whose occupants only disembark if their boat survives to reach shore. Later surface gunboats and aircraft require higher-tier countermeasures.

**Implemented boundary:** Development-only GLES2 module, deterministic native campaign loop, generated world/menus and multiple purchasable combat upgrades. **Missing acceptance:** actual physical headset proof, campaign persistence, sophisticated effects and shared MIDI synth/audio service.

---

## RV-GAME-05 — Native Classics Shelf

**Implemented:** a dedicated **Native Games** destination in the in-headset Home menu, separate from DOS Games and Native Apps. It filters original packaged native modules through the existing allowlisted native-module host; there are no external ROMs, emulators or imported game assets.

The first three games are **Classic Pong**, **Classic Snake**, and **Chutes and Ladders** (original generated visuals and generic historic ladder/chute rules, not commercial board artwork). Each builds as its own GLES2 native module from a common small C++ source and a pure, deterministic gameplay kernel:

- Pong: point vertically or touch/slide up/down on the Daydream pad to move the left paddle vertically; Select serves or restarts, the CPU defends the right side, first to eleven wins. The source restores opposing vertical paddles, wall bounces and contact angle; full original two-human play is still an optional future mode.
- Snake: touch the upper/lower/left/right portion of the Daydream pad to turn, use a gamepad stick, or point-and-Select on the in-scene direction pads; consume food, grow, avoid walls and the tail, Select restarts after losing.
- Chutes and Ladders: Select rolls the die on your turn, a deterministic CPU opponent takes the following turn automatically, and the generated 100-cell board includes original green ladders and red chutes. First to cell 100 wins, Select restarts.
  
The eighth original native classic, **Classic Reversi**, is the full
8×8 disc-flipping board game. A four-disc opening, all eight capturing
directions, legal-move markers, automatic forced passes, score-based
win/draw results and bounded four-ply White CPU are implemented in
`classic-reversi`. It uses the same shared board cursor, VR ray
and independent packaged native game host as Chess and Checkers.

The seventh original game, **Classic Breakout**, runs two full 112-brick walls, three-life
paddle-and-ball arcade match in its own packaged GLES2 module. Move the
paddle with touchpad horizontal input, gamepad axis, or the tracked-controller
pointer; Select serves and restarts. Geometry and frame-state changes use the
same shared low-poly pipeline and do not advance the simulation during stereo
drawing. Full physics and contact checks run as deterministic C++ tests.

Chess now also accepts underpromotion by selecting a seventh-rank pawn
and cycling queen/rook/bishop/knight with the existing native Secondary
action. A conditional on-board CLAIM DRAW command supports an earned
FIDE draw claim without taking over the shell Home button. Fivefold
repetition and 75 moves without pawn/capture end automatically; weak
CPU now searches actual legal human replies with alpha-beta pruning,
increasing to three plies when fewer pieces remain; stronger difficulty
settings and S9 runtime profiling remain development items.

Checkers records WCDF-style repetition and no-progress draw eligibility.
When a valid draw claim is available, a single temporary CLAIM DRAW
target appears on the board; ordinary click-to-move, compulsory captures
and continued jumps are unchanged. The CPU searches full forced-jump
sequences through three alternating complete turns with alpha-beta pruning,
rather than evaluating a single capture landing. Difficulty controls and
the S9 response-time check remain outstanding.

The Checkers, full-size 19×19 Go and Chess boards additionally allow Daydream touchpad direction to move a visible cell cursor and Select to choose a piece/destination; tracked-controller ray selection still works. The Go PASS target is point-and-Select or the existing native secondary action. The `native-classics` hosted input profile exists specifically to make these actions reach the games; immersive native projects retain `native-standard` and their shell locomotion.

All gameplay advances in native Update once per frame, with common cached vertex-color geometry rendered for each stereo eye. No gameplay runs in per-eye rendering. **2026-10-15 navigation correction:** **Classic Games** exclusively lists the packaged `classic-` modules; **Native Apps / Projects** exclusively lists the other native modules and functional shell applications. The previously duplicated classic entries have been removed from Native Apps, and Development-only nonclassic native projects remain in Native Apps when admitted. Both Home entry points retain their original hit regions. Red Ledger and Breakwater Battery remain Development-logging-gated and appear only in a Development session; the runtime independently preserves that gate. Ministry of Intelligence and Forward Detachment now have Development-only native decision boards registered in the build and launch catalogues; these are limited kernel demonstrations, not full games or device-accepted builds. Controller hardware/comfort/interaction and thermal acceptance on the Galaxy S9 remain pending; signed APK build status must be checked independently.

## RV-GAME-06 — Omega Front

**Status:** First deterministic **headless combat kernel and regression test source committed**; **no hosted native module, launch tile, game art or APK integration exists yet**. This is a new original top-down, tile-based military tactics/RPG hybrid, not an adaptation of an existing commercial game.

**Setting authority:** [OMEGA_FRONT_WORLD_BIBLE.md](OMEGA_FRONT_WORLD_BIBLE.md) — Ouroboros Corporation, planetary bisection, past/future rifts, baseline-human Eden Zones, wastelands, barren zones, anomaly fields, the canonical count of 17 factions and their unresolved individual identities.

**Faction design:** [OMEGA_FRONT_FACTIONS.md](OMEGA_FRONT_FACTIONS.md) — 17 named but provisional distinct faction dossiers, including five opening-theater rivals, their real economic and tactical asymmetries and treaty/AI concepts.

**Environment design:** [OMEGA_FRONT_ENVIRONMENTS.md](OMEGA_FRONT_ENVIRONMENTS.md) — 12 theaters × 50 campaign sectors, four environmental classes, first 16-sector Hearthfall March, five proposed opening operations, tiles, hazards, logistics and acceptance gates.

**Faction history:** [OMEGA_FRONT_FACTION_CHRONICLES.md](OMEGA_FRONT_FACTION_CHRONICLES.md) — seventeen distinct proposed histories, internal divisions and territorial claims.

**Origin:** [OMEGA_FRONT_HEARTHWELL_PROLOGUE.md](OMEGA_FRONT_HEARTHWELL_PROLOGUE.md) — an isolated contingent's acting commander, new Eden morphology and emergent civil compact.

**Campaign:** [OMEGA_FRONT_GRAND_CAMPAIGN.md](OMEGA_FRONT_GRAND_CAMPAIGN.md) — seventy multi-operation faction campaigns across level 1–100 and ten-world-prestige longer-term covenant.

**Mechanics authority:** [WAR_WITHOUT_END.md](WAR_WITHOUT_END.md) (retained earlier filename) — combat anatomy, progression calibration, 600-sector/4,096-inspection-room address space, seven encounter scales, independent commander/troop/squad-room/weapon/armor/accessory/base/world/doctrine progression, deterministic simulation, tenfold prestige covenant, and staged P0–P11 roadmap.

**Current P1 source:** `app/src/main/jni/native/omega_front_tactics.h`, `app/src/test/native/omega_front_tactics_test.cpp` and [docs/native/OMEGA_FRONT_TACTICS.md](../native/OMEGA_FRONT_TACTICS.md) — actual 24×24 combat rules, three unit roles, finite ammunition and waterworks repair, LOS/cover, opponent objective AI and deterministic win/loss; GCC/Clang/ASan+UBSan host regression passed. **This source is not yet registered or packaged as an APK module.**

**Native identity:** Comfortable 3DoF VR tabletop with tiny crisp chibi pixel sprites on logical square tiles. Shell-normalized select/secondary/touchpad inputs; O/Home stays shell-owned. Bounded, reproducible order-pulse simulator independent of GLES; rendering only consumes the simulation. The strategic economy and room inspection are real tactical gameplay rather than passive timers. Retain S9 performance and offline-first single-APK operation.

**First admissible development slice:** Headless deterministic 24×24 skirmish, terrain/LOS, two factions, three distinct unit archetypes, order queue, movement, damage, victory and defeat, followed by an actual hosted war-table interaction test. A sketch, static board, reduced-rules mini-game, or menu with nonfunctional choices is **not** completion.

**Scale rule:** 100 levels and 10 full world prestiges are release targets, not excuses to load/render all rooms simultaneously. The 24×24 starter is a test fixture; full tactics, campaign and room structure must not be quietly cut to accommodate UI or hardware.

## Recommended implementation order

1. **Between Deliveries** — smallest space, richest direct hand interaction, strongest test of the native-module interaction layer.
2. **Ministry of Intelligence** — naturally seated and interface-heavy; ideal for proving persistent simulation, document interaction, and high-resolution VR UI.
3. **Iron Sight** — consumes the proven interaction, persistence, AI, rendering, and reporting foundations while adding outdoor traversal, vehicles, squad state, and combat.
4. **Breakwater Battery** — prioritize its compact stationary-shooter loop for immediate Development device playtesting, without waiving the usual device and thermal gates.
5. **Omega Front** — build the headless tactical kernel first, then the chibi VR tabletop. Its seventeen distinct factions, newly emerged Eden frontier, **70 multi-operation main campaigns**, territory ecology, layered progression and tenfold prestige requirements remain full release scope.

These are roadmap pointers only. Implementation should begin only after RV-0400/RV-0402 are sufficiently stable and the first tiny native gameplay prototype (RV-0600/RV-0601) has established a safe thermal/performance envelope.
