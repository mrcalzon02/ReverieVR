# Native Game Project Pointers

**P3k Waterworks locality repair (2026-10-16):** Corrected shared tactical pump-HP aliasing: the actual S011 Lantern Pump persists damage only from M01 and M03. Other first-theater operations use healthy independent local objective installations (18 HP) and preserve Hearthwell pump condition even on tactical defeat. `omega_front_infrastructure.h` defines the production decision shared by battle creation and after-action settlement; strict standalone GCC/Clang/ASan/UBSan passes, while a new six-operation save-roundtrip regression is registered in the 21-program host runner pending full checkout. The fixed 2,048-byte `frontier.v1` schema v6, host ABI and Daydream controls are untouched. Full runner, Android APK, and physical S9 acceptance pending.


**P3j paid passage rights (2026-10-16):** `omega_front_access.h` makes M05 Meridian transit and M06 Lost Ninth passage confer actual ration-free movement **into their own target sectors only**, conditional on a valid outstanding, honored or legacy accord. Broken installments revoke the benefit and restore normal movement costs; Scrapline salvage licences are not transit permits. `Frontier::Travel` and the native map label now use this derived permission, requiring no save schema change (existing `frontier.v1` v6). `omega_front_access_test.cpp` passed strict GCC/Clang/ASan/UBSan; `omega_front_access_campaign_test.cpp` is registered but **has not been compiled against the full checkout in this run**. Android APK and S9/Daydream testing remain open.


**P3i border enforcement (2026-10-16):** `omega_front_border.h` now applies exactly one genuine ration confiscation and faction claim/trust/relationship consequence when the commander *physically enters* a sector after breaking its M04–M06 installment. It cannot softlock retreat or repeatedly levy the same old debt. `Frontier::Travel` and the VR war-table incident label are source-integrated. `frontier.v1` stays at 2,048 bytes with current internal schema v6 and transactional validation, with v1–v5 migration grandfathering breaches predating patrol enforcement. Standalone C++17 GCC/Clang and sanitizer tests pass; integrated save/UI regression is added but not executed against the full checkout. Android NDK/APK/S9 acceptance remains unverified.


**P3h treaty obligations and faction pressure (2026-10-16):** `omega_front_obligations.h` now enforces three-day follow-up installments after first-contact accords, with finite physical transfers, fulfilled/default status, faction relation changes, trust loss and bounded territorial claim pressure. The source-wired `Frontier::AdvanceDay` settles them once; the native map shows the latest deadline or result. `frontier.v1` keeps its 2,048-byte slot but advances internal schema to v5, recognizing earlier v1–v4 files and treating old treaties as LEGACY. A 15-test host runner includes standalone and integrated treaty tests; **only the standalone new helper has been compiled and sanitizer-tested in this run**, and Android/Daydream verification remains pending.


**P3g source integration (2026-10-16):** First noncombat alternatives to M04/M05/M06 are wired through omega_front_accord.h, Frontier, Session, and the native map. Negotiated salvage licence/charter access/ford passage require reconnaissance, one prior parley, sufficient relations/trust and real resource payments; they advance a mission without fabricating a battle result or ownership transfer. The existing 2,048-byte frontier.v1 record advances to internal schema v4 with separate treaty history and v1-v3 loading. GCC/Clang and ASan/UBSan pass for the standalone accord helper; full integrated host, Android APK and Daydream verification remain pending.


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

### Current October 2026 source-only usability pass

The existing Red Ledger GLES2 room now places the bar top approximately 0.31 m lower, with counter-area selection bounds, tap, cups, washer, ledger, purchasing papers and payment props moved with it. Fixed readable station names and a title are baked as actual world-space character geometry once per GL context (one signage draw per eye), not simulated text-colored boxes or per-frame glyph calls. The tap, sink and patron have additional recognizable primitive-era details; mugs now use one shared 12-sided hollow mesh with a rolled lip and visible handle/liquid rather than rectangular cup bodies. The rear-corner exit can now be selected at seated range (4.25 m work-ray cap versus 3.5 m previously). The economic simulation, native ABI and v2 save schema are unchanged. All labels and reach/occlusion, stereo size, controller aim and performance remain **device-unverified**, not device accepted. Sign plaques designate workstation locations; they do not promise an currently available action or display live economic numbers.

### Three-room and faction-service expansion — source integrated, hardware unverified

The ground floor now implements three distinct selectable areas: front bar, stockroom with supplier NEXT/BUY and beer/cup crates, and living quarters with a readable financial ledger and a bed that calls the established end-day settlement only after patrons and payment are cleared. Each area has room-specific procedural floor/wall/furniture geometry, sign text and hit targets. Physical entrance/back routes change rooms only with empty hands, protecting held cup and outstanding payment states. The live in-world board identifies patron faction, requested drink/snack and the operator's selected products, inventory costs, money, stock and faction standing. Four faction archetypes currently own eight drink varieties and six snacks through a finite, checked trade inventory. Exact catalog, save migration and remaining device gates are documented in [the room/faction implementation record](../records/2026-10-16-between-deliveries-room-factions.md). This is **not** a full upstairs/faction campaign, high-fidelity imported art pack, or device-accepted build.

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

## Lantern Desk native high-resolution photographic renderer — 2026-11-20

**Implemented in actual Lantern Desktop/Android module, provisional art:**
`recon_projects_module.cpp` now creates and checks a 512×512
near-vertical/near-nadir grayscale **registered photographic reference
plate** for each operational sector, with two exposure conditions sharing
the identical fixed map/road/river/field positions. The source generator
`lantern_nadir_scene.h` deliberately changes only localized subject
details (river bridge center span, rail convoy, orchard vehicle/decoy)
between exposure states; there is no floating 3D battlefield and no
independently invented camera/perspective for the paired exposure.

The real `lantern_photo_capture_quality.h` weather masks and
persistent camera grime are applied **once when the photo returns**,
with stable per-exposure conditions derived from the already-saved
capture/sector index. The master is developed at the current provisional
512-pixel source tier, and the derived RGB texture embeds the same
fixed-coordinate gold photo pins without changing the master negative.
The `lantern_photo_gl.h` GLES2 program displays the source frame and
magnifies the correct 8×8/16×16 portion of **the same 512px image**
in the original 2× loupe. Revisited-sector prior/current plates share
one geographic camera and each has an independently weather-obscured
recording; the loupe crop and both stereo eye matrices are preserved.
A new optional `prepare_frame` callback stages changed textures **once
ahead of stereo** and reinstates them after GL context loss. All
simulation grading, the V4/128-byte save, old pin coordinates and
Forward Detachment logic remain unchanged. The legacy 16×16 geometry
is retained underneath as an explicit GL failure fallback, not as the
normal visible photo when the renderer is available.

The old visible `WINDOW CHANGED N/64` from symbolic black/white data
is suppressed for actual high-resolution comparisons; player-facing
text invites visual inspection rather than falsely claiming the
symbolic counter measures a physically weather-obscured photograph.
The unchanged symbolic evidence and grading remain separate from
the photographic display while the game evolves; a true gameplay
recognition migration and revised scoring are future work.

**Source quality boundary:** This is a **registered, deterministic
procedural photographic REFERENCE**, not a professionally authored
historical high-resolution aerial negative. The preceding separately
generated oblique bridge/convoy/cloud pieces remain rejected.
The user-approved end-state requires proper aligned, authored photo
assets that can replace the reference plates without altering the
mission/camera/GL sampler contract. The generated current reference
is a real GPU texture and should be inspected in the existing Mesa
and installed Android screenshot artifacts before treating its
photographic plausibility as adequate. This is **not yet a production
photorealistic picture library** or a purchased HAP-upgrade economy.

The native test now samples real 512px source pixels in all 64
magnifier cells (both historical/current plates and stereo views),
checks exact save/context-recreate image parity and gold-pin render,
and runs `lantern_nadir_scene_test.cpp` in both GCC and Clang to
reject sector-map shifts between exposures. Physical Daydream optical
and thermal validation remains open.

## Lantern Desk camera, weather and HAP quality architecture — 2026-11-20

**Verified original design authority:** The original *Ministry of
Intelligence* introduction and tutorial describes High Altitude Photography
delivering five images per day, with separately purchased resolution,
magnification, measuring equipment, quick-reference charts and reliability.
The Intelligence Model requires the player to interpret the captured evidence
without receiving a hidden World Truth answer. This development pass does
not substitute new upgrade costs, future mission tiers or fake recognition
labels for those established rules.

**Implemented as a camera kernel and used by the live native image path:**
The game-private `lantern_photo_capture_quality.h` accepts one
camera-registered high-resolution grayscale plate and never generates
a separately framed environment or queries World Truth. The *current
reference plate* is spatially generated by `lantern_nadir_scene.h`; it
is not yet an approved authored historical photonegative. Once approved
assets are available the same capture kernel can consume their
registered source pixels instead of synthesizing terrain.
This distinguishes the photographic **master capture** from the
upgrade-dependent **developed print**. The captured master preserves genuine
weather obscuration, lens dirt and sensor noise as recorded at exposure
time. After HAP resolution improvement, the *same master* can be developed
with finer sampling and less blur. It never removes recorded cloud cover,
cleans an already dirty lens or hallucinates information unavailable in
the captured master. Provisional display ceilings (128,256,512,1024,
native up to 4096 pixels on the longest edge) are engineering test tiers,
**not** claims of established upgrade prices or player inventory.

The atmospheric compositor supplies five **independent low-memory masks**:
cloud opacity, world-offset physically displaced cloud shadows,
precipitation, haze, and the aircraft's lens grime. Cloud families: clear, scattered/broken
cumulus, cirrus, stratus, overcast, squall. Precipitation: none, rain,
storm, heavy storm, snow and hail. Family/precipitation, weather seed,
world-space photograph registration, wind vector and exposure time
determine **repeatable** weather; changing time or wind moves patterns
without changing terrain, roads or target coordinates. Lens grime uses
only the specific camera identity/condition and image-space coordinates;
it remains stable across weather changes until the camera is serviced.
Capture masks are limited to 256x256 per channel even for 4096x4096
source images and are generated once per photo. Tier development uses
area prefiltering and bilinear reconstruction, not giant binary squares.
The renderer never regenerates masks for each stereo eye.

**Verification:** `lantern_photo_capture_quality_test.cpp` runs under
the existing `scripts/verify-recon-persistence.sh` GCC/Clang native CI,
checking deterministic exposure replay, distinct patterns, genuine
world-wind/time advection, stable lens dirt, rain/storm/snow/hail layers,
larger storm interference, grayscale master retention, measurable
postprocess upgrade detail, strict bounds, and rejected invalid states.
All tests use a synthetic photographic input *test pattern*, not a
pretend gameplay image. No automatic recognition or visual-rule
rebalancing is added.

**Completed runtime wiring, remaining gaps:** `recon_projects_module.cpp`
now uses the registered 512px reference capture and actual GLES2
source/loupe textures. The existing 16x16 bitfield is retained only as
simulation evidence, checked V4 pin-save provenance, and a drawing fallback
if the optional photographic renderer cannot initialize. This is not
yet production photorealistic source photography, high-resolution
recognition-based grading, purchasable HAP resolution/magnifier upgrades,
persistent randomized weather/camera-service state, or the intended
five distinct daily photographs. The presently derived per-sector
weather seeds and fixed lens condition ensure repeatability after
relaunch but still need to be promoted into the persistent case
record for greater day-to-day variation. The original independently
rendered oblique bridge/convoy assets remain REJECTED. No save migration
or simulated hidden answer exposure has been performed. Installed
Android 10 and actual Mesa source/loupe runs exist; physical Galaxy
S9/Daydream optics, performance, input and thermal acceptance remain
open.

## Lantern Desk readable evidence comparison — 2026-11-20

The actual native GLES2 desk previously squeezed the gold PHOTO PINS
counter against SOURCE FRAME in its 80-degree headset-oriented view.
The Lato layout now spaces four separate rows: photo source/loupe labels
(at -0.64), factual status (at -0.85), concise Minus+pad and
Minus+Select instructions (at -1.05), and the actionable FILE assessment
(at -1.30); the shell-owned bottom menu/exit legend remains untouched.
Text sizes remain legible within the existing panel, without extending
hit targets or adding an extra page or inert control.

When a previous filed exposure is available, the status row shows
`WINDOW CHANGED N / 64   PINS P / 12`. N is the actual number of
photographic pixel differences in the synchronized visible 8x8 loupe
region, counted from the existing two 16-row image bitfields.
It changes when the player pans the window, never from annotation marks.
This is a descriptive visual comparison and **not** an inference of
military contact, evidence correctness or a recommendation for a report.
A first-time photograph displays only its real pin count.
The count is computed only when the existing screen is rebuilt, not every
stereo eye render; no network, texture, GPU readback, extra save field,
mission rule or Forward Detachment change is needed.

The host native Mesa suite checks the stated window count against the
independently observed 64 source pixels in both GL eyes, changes crop
to a different region, verifies pin overlays cannot change the count,
and retains the exact saved-photo/score result. Physical Galaxy S9
optical/comfort acceptance is still a separate gate.

## Lantern Desk current photograph pins — 2026-11-20

The registered Lantern Desk native game now lets analysts bookmark up to
**twelve actual cells of the current 16x16 photographed evidence**.
On a live 8x8 loupe, hold **Minus**, then tap **Select** to toggle a gold
hollow outline on the southeastern central magnified pixel. Panning the
lens (Minus + pad) permits marking other cells. The original source
image and 2x current loupe show exactly the same annotations; the
previous filed photo remains unaltered. The in-world Lato report panel
displays `PHOTO PINS N / 12` and the chord. Pinning does not select a
report, infer an answer, or alter photograph pixels, enemy truth,
credibility, or sortie progression. Releasing Minus after pinning
cannot accidentally advance the report selector.

Single-film loupes retain their established **center-on-relaunch** behavior:
the pin itself is anchored to the original source pixel and survives the
recenter. Revisited-sector comparison loupes continue to restore their
saved inspection area; no previous-exposure markers are invented.

Pins use 32 bytes of **already reserved** space in the checked 128-byte
Lantern `session-v1.bin` record: Lantern-only V4 encodes a bounded
16-row bitmask and preserves all V1-V3 reading with no annotations.
Forward Detachment remains **V3 byte-for-byte**, with no new fields.
Malformed tails, checksum errors, out-of-phase pins and counts above
12 are refused transactionally. A pin checkpoints immediately;
failed writes keep the in-memory pin and block ordinary exit until
a successful retry. The same photo's filed report retains its pins,
and starting a new sortie clears them to prevent false attribution.

The real hosted native module receives the Minus/Select sequence,
writes an exact source-backed checkpoint, survives Pause/Destroy/Reload
and renders the annotated source and loupe in real GLES2.
The existing installed Android recon test now verifies the V4 row-8
pin through Java input → JNI → checked native bytes and captures the
resulting GLES2 frame (within `recon-shell-ministry-lantern-desk.png`).
These are host/emulator proof, not physical Galaxy S9 optical, audio
or controller acceptance. No separate game APK or new workflow.

## RV-GAME-02 — Ministry of Intelligence: Lantern Desk VR

**Current implementation boundary.** The native board supports three named sectors and two authored photographic conditions per sector. Filing a report and opening the next sortie advances the visited sector. On a sector's first assignment the analyst sees a field guide, the 16 × 16 source and a 2× loupe. On subsequent visits the desk shows the previous filed exposure and the current exposure through synchronized 8 × 8 loupes. This view remains available after working on other sectors. Aim at the central film or hold Minus and nudge the pad to move both windows. The assessment selector remains live. Reopening a saved comparison restores both films and the inspected area. The original single-film view starts centered on relaunch. Previous builds retain their active cases and scores. Their odd sector-visit counts establish a prior exposure, while an ambiguous older history starts its archive with the next filed sortie. Annotations, a physical desk and additional cases remain open.

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

**Current implementation boundary:** The native board runs the road and ridge patrols from briefing through contact, return, debrief and base service. Selecting a route at briefing shows its last filed intelligence result. Separate route histories, fuel and damage survive refit, relaunch and checkpoint retries. Ridge Bypass includes an eight-step foot survey through a rendered stream crossing, shelter and turnout. The player can nudge the pad right to walk or left to retrace the route. Select walks forward, logs a visible water mark or shelter condition at its location, or opens the original contact at the turnout. Reports for the two nonviolent decisions gain two intelligence points per observation. The debrief and next ridge briefing keep the filed notes. Forward checkpoint v3 keeps the location and notebook in the existing 128-byte slot. A v1 or v2 ridge patrol that was already travelling resumes at the turnout with its original contact available. Main Road, driveable vehicles and squad traversal remain future outdoor work.

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

**Status:** Development-only 4×4 Hearthfall map now routes six real tactical operations: the initial three Hearthward/waterworks battles plus Scrapline's finite wreckage recovery, Meridian's contested checkpoint and Lost Ninth's fortified ford. One-time consequences alter resources, occupation, claims and faction standings, which persist in an upgraded versioned 2,048-byte `frontier.v1` save. Nine GCC/Clang host suites and mock-GL lifecycle pass; **Android NDK/APK, physical S9/Daydream, the 600-sector world and full 70-arc campaign remain unverified or unfinished.** This is an original tactics RPG, not an adaptation of a commercial game.

**Setting authority:** [OMEGA_FRONT_WORLD_BIBLE.md](OMEGA_FRONT_WORLD_BIBLE.md) — Ouroboros Corporation, planetary bisection, past/future rifts, baseline-human Eden Zones, wastelands, barren zones, anomaly fields, the canonical count of 17 factions and their unresolved individual identities.

**Faction design:** [OMEGA_FRONT_FACTIONS.md](OMEGA_FRONT_FACTIONS.md) — 17 named but provisional distinct faction dossiers, including five opening-theater rivals, their real economic and tactical asymmetries and treaty/AI concepts.

**Environment design:** [OMEGA_FRONT_ENVIRONMENTS.md](OMEGA_FRONT_ENVIRONMENTS.md) — 12 theaters × 50 campaign sectors, four environmental classes, first 16-sector Hearthfall March, five proposed opening operations, tiles, hazards, logistics and acceptance gates.

**Faction history:** [OMEGA_FRONT_FACTION_CHRONICLES.md](OMEGA_FRONT_FACTION_CHRONICLES.md) — seventeen distinct proposed histories, internal divisions and territorial claims.

**Origin:** [OMEGA_FRONT_HEARTHWELL_PROLOGUE.md](OMEGA_FRONT_HEARTHWELL_PROLOGUE.md) — an isolated contingent's acting commander, new Eden morphology and emergent civil compact.

**Campaign:** [OMEGA_FRONT_GRAND_CAMPAIGN.md](OMEGA_FRONT_GRAND_CAMPAIGN.md) — seventy multi-operation faction campaigns across level 1–100 and ten-world-prestige longer-term covenant.

**Mechanics authority:** [WAR_WITHOUT_END.md](WAR_WITHOUT_END.md) (retained earlier filename) — combat anatomy, progression calibration, 600-sector/4,096-inspection-room address space, seven encounter scales, independent commander/troop/squad-room/weapon/armor/accessory/base/world/doctrine progression, deterministic simulation, tenfold prestige covenant, and staged P0–P11 roadmap.

**Current P3 headless frontier:** `app/src/main/jni/native/omega_front_frontier.h` and `app/src/test/native/omega_front_frontier_test.cpp` implement real seeded 16-sector territory state, adjacency/viaduct gates, stored supplies, guards/casualties, one forecast ecology transition and three sequential tactical operations for F01's opening `The Water We Own` arc. These scenarios are now navigable through a source-wired Development-only GLES2 4×4 VR map and saved within the new `frontier.v1` full-campaign snapshot, including live tactical checkpoint state. See [implementation boundaries](../native/OMEGA_FRONT_TACTICS.md).

**Current P3f reconnaissance and diplomacy source slice:** `omega_front_intelligence.h` and `omega_front_intelligence_test.cpp` implement/verify finite-ration adjacent surveys and once-only real-material faction concessions, with actual relationship/claim changes. `omega_front_frontier.h`, `omega_front_session.h` and the Development-only GLES2 renderer are source-wired to seven active field actions and real map-intelligence masking. The unchanged `frontier.v1` slot has internal schema v3, retaining backward decoding of v1/v2 and storing all 16 scouting levels and one-time concessions. Standalone helper GCC, Clang and ASan/UBSan passed; **integrated host/NDK/APK and physical Galaxy S9/Daydream checks remain pending**, and this is not complete treaty diplomacy or a 70-arc campaign.

**Current P3e faction-connected first-contact operations:** `omega_front_frontier.h`, `omega_front_campaign_save.h`, `omega_front_module.cpp`, and `omega_front_operations_test.cpp` now link F04 Scrapline at Ashrail Yard, F02 Meridian at Charter Gate and F06 Lost Ninth at Caravan Ford to actual campaign travel/deployment, faction doctrine, finite after-action consequences and 17 persisted diplomatic standings. The campaign save is still `frontier.v1` but internal **schema 2**, with schema-1 compatibility. A nine-suite C++17 host runner and mock native module pass; hardware admission not yet established. These are first-contact battles, not three fully authored additional campaign arcs.

**Current P3d faction simulation:** `omega_front_tactics.h` now supports four implemented faction doctrines (F01 Hearthward, F02 Meridian, F04 Scrapline and F06 Lost Ninth), plus preserved unaligned raiders, three new combat roles (Sentinel, Salvager, Legionary), sight-limited targeting, finite depot theft and formation protection. `omega_front_faction_encounters.h` and `omega_front_factions_test.cpp` supply reproducible encounter fixtures. The original first arc uses F01; F02/F04/F06 now also appear in three subsequent strategical first-contact battles, but their entire multi-operation faction arcs are not yet implemented. The existing module renders dedicated procedural silhouettes and faction colors. Combat checkpoint schema v3 supports backward-compatible v1/v2 loads, with unchanged byte size. Eight host regressions and mock GL lifecycle passed; no NDK/APK or S9 headset acceptance yet.

**Current P3c objective variety:** `omega_front_tactics.h` and `omega_front_module.cpp` now implement and visibly distinguish pump defense, contested orchard seizure and finite-component relay sabotage in the first campaign. `omega_front_checkpoint.h` writes version 2 and reads old version 1; `omega_front_objectives_test.cpp` checks objectives, migration and resistance to false victory/corrupt mission saves. Seven strict host regression suites pass; Galaxy S9 APK/Daydream acceptance remains outstanding.

**Current P3 strategic bridge:** `omega_front_session.h` and `omega_front_campaign_save.h`, tested by `omega_front_session_test.cpp` and `omega_front_campaign_save_test.cpp`, add the real 16-sector map input/deployment/after-action cycle and atomic first-frontier save. Six host suites and mock GL exercised state transitions, but signed APK and S9 testing remain pending.

**Current P2 source:** `app/src/main/jni/native/omega_front_controls.h`, `app/src/main/jni/native/omega_front_module.cpp`, `app/src/test/native/omega_front_controls_test.cpp` — real touchpad/pointer tile navigation, queued orders, 250-ms combat pulses and procedural board. Registered for Development-only Native Games. NDK/APK/physical S9 validation pending.

**Current P1 source:** `app/src/main/jni/native/omega_front_tactics.h`, `app/src/test/native/omega_front_tactics_test.cpp` and [docs/native/OMEGA_FRONT_TACTICS.md](../native/OMEGA_FRONT_TACTICS.md) — actual 24×24 combat rules, three unit roles, finite ammunition and waterworks repair, LOS/cover, opponent objective AI and deterministic win/loss; GCC/Clang/ASan+UBSan host regression passed. **The Development-only module is source-registered, but no built APK or Galaxy S9 launch has been validated.**

**Native identity:** Comfortable 3DoF VR tabletop with tiny crisp chibi pixel sprites on logical square tiles. Shell-normalized select/secondary/touchpad inputs; O/Home stays shell-owned. Bounded, reproducible order-pulse simulator independent of GLES; rendering only consumes the simulation. The strategic economy and room inspection are real tactical gameplay rather than passive timers. Retain S9 performance and offline-first single-APK operation.

**First admissible development slice:** Headless deterministic 24×24 skirmish, terrain/LOS, two factions, three distinct unit archetypes, order queue, movement, damage, victory and defeat, followed by an actual hosted war-table interaction test. A sketch, static board, reduced-rules mini-game, or menu with nonfunctional choices is **not** completion.

**Scale rule:** 100 levels and 10 full world prestiges are release targets, not excuses to load/render all rooms simultaneously. The 24×24 starter is a test fixture; full tactics, campaign and room structure must not be quietly cut to accommodate UI or hardware.

## RV-GAME-SITKA — Sitka

**Status:** original native game concept with a **source-integrated desktop editor, geodata baker and standalone C++ water/weather adapter**; no real surveyed Sitka terrain asset, GLES2 game module, catalog button, APK or S9 runtime proof. See [editor/geodata integration](SITKA_EDITOR_PIPELINE.md). **Official player-facing title: Sitka**, with no subtitle. Canonical design and coastal-geodata/rights plan: [Sitka](SITKA.md).

**Identity:** a slow, peaceful, geography-grounded Sitka Sound, Alaska fishing and beachcombing game. Tour recognizable beaches and coves on foot or by boat, fish, find crabs and unusual shells, observe local wildlife, keep a field journal, and upgrade rods, bait, tackle and vessels. No combat, timers or punitive progression.

**Visual/platform approach:** adapt low-poly PSX-inspired techniques from Beckenburg and Malefic Veil to ReverieVR's own GLES2/Daydream renderer, with offline processed licensed USGS elevation, NOAA coastal/bathymetry and compliant OpenStreetMap features. Google Earth may be used only within its permitted terms for reference or a separately licensed online integration; **do not scrape or trace Google map imagery or 3D tiles into game meshes**.

**First playable gate:** two real Sitka-area shoreline pockets, reliable 3DoF cast/bite/reel/land mechanic, crab or shell find, useful tackle upgrade, comfortable boat/shore travel, persistent journal/save, sound, honest native-app launch and physical Galaxy S9 stereo/thermal acceptance. Do not add an inert Native Apps launch option before there is a functioning game.

---

## Recommended implementation order

1. **Between Deliveries** — smallest space, richest direct hand interaction, strongest test of the native-module interaction layer.
2. **Ministry of Intelligence** — naturally seated and interface-heavy; ideal for proving persistent simulation, document interaction, and high-resolution VR UI.
3. **Iron Sight** — consumes the proven interaction, persistence, AI, rendering, and reporting foundations while adding outdoor traversal, vehicles, squad state, and combat.
4. **Breakwater Battery** — prioritize its compact stationary-shooter loop for immediate Development device playtesting, without waiving the usual device and thermal gates.
5. **Omega Front** — build the headless tactical kernel first, then the chibi VR tabletop. Its seventeen distinct factions, newly emerged Eden frontier, **70 multi-operation main campaigns**, territory ecology, layered progression and tenfold prestige requirements remain full release scope.

These are roadmap pointers only. Implementation should begin only after RV-0400/RV-0402 are sufficiently stable and the first tiny native gameplay prototype (RV-0600/RV-0601) has established a safe thermal/performance envelope.

## November 16, 2026 gameplay continuations

Red Ledger's living-quarters ledger cycles through closed-shift accounts, faction accounts and renovations. Closing a day itemizes opening cash, sales, stock, debt payments, protection, fines and the closing balance in cents. The final installment is limited to the remaining balance. The first shift can be completed through the stockroom, bar and living-quarters plaques, and its accounts remain readable the next day after relaunch.

Chutes & Ladders retains the two players' last rolls, landings and destinations. Its v2 match payload imports v1 positions and pending CPU turns. Go previews Chinese area scores, including White's 7.5 komi, as the player marks groups. Reversi identifies the player who passed, Pong labels each side's score and winner, and Snake identifies a loss or a cleared board. Breakout's narrowed paddle can reach both walls. Chess's end-of-match Restart button starts a fresh match in the chosen orientation.

The eight classic match-ending replays use the same module input, render and save entry points as the hub. Build `scripts/audit/probes/classic_match_endings.cpp` once per Android game-kind value (0 through 7), with `-std=c++17 -pthread -Iscripts/audit/probes -Iapp/src/main/jni/native -lEGL -lGLESv2 -lz`, and pass an existing capture directory. Guess the Door's `scripts/audit/probes/guess_full_maze.cpp` uses the same host to follow the visible door-design attributes through twelve rooms and a pit retry.
