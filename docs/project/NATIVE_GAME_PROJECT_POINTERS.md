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

## Recommended implementation order

1. **Between Deliveries** — smallest space, richest direct hand interaction, strongest test of the native-module interaction layer.
2. **Ministry of Intelligence** — naturally seated and interface-heavy; ideal for proving persistent simulation, document interaction, and high-resolution VR UI.
3. **Iron Sight** — consumes the proven interaction, persistence, AI, rendering, and reporting foundations while adding outdoor traversal, vehicles, squad state, and combat.

These are roadmap pointers only. Implementation should begin only after RV-0400/RV-0402 are sufficiently stable and the first tiny native gameplay prototype (RV-0600/RV-0601) has established a safe thermal/performance envelope.
