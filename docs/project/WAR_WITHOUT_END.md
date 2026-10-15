# WAR WITHOUT END — Native Tactical Strategy RPG Design Charter
**Project ID:** RV-GAME-06 (proposed)  
**Status:** CONCEPT / DESIGN ONLY; no playable module or APK inclusion claimed  
**Date:** 2026-10-15  
**Platform:** ReverieVR native module, one offline-capable Android APK; Samsung Galaxy S9 + Daydream View/controller as the reference target  
**Working title:** War Without End (original identity; final title/art direction subject to revision)

## 1. Vision, references, and non-negotiables

Create an original, visually miniature, strategically colossal combat RPG: the territorial warfare, bases, factories and command economy of classic RTS games; the legible orthogonal grid, capture decisions and unit matchups of handheld tactics games; the improbable statistical depth, reincarnation and explorable equipment of over-the-top tactical RPGs; and formations/rooms whose interior layers can be inspected and improved.

Reference inspirations **for mechanics only**, not copyrighted characters, maps, names, sounds, icons, art, story or code: Command & Conquer; Advance Wars; Disgaea; Soul Nomad & the World Eaters (Gig, Room Inspection). Core identity must remain original. No pay-to-skip system, microtransactions, forced login, idle leveling, loot-box economy, calendar timers or premium-stat purchases. Grind is expressed through dangerous playable battles and improvements earned by performance, not arbitrary real-time waiting.

Fantasy: begin commanding disposable militia, end commanding a world whose physics, economy, armories, infrastructure and inhabitants have been rewritten by a tenfold chain of cataclysmic military successions. A single veteran's equipment should eventually be an entire theater of combat to explore.

Game-long rule: **every strategic advantage has an operational cost, an enemy response, and a progression pathway**. There must be no shallow "levels" that differ only in displayed numbers.

## 2. Core play loop and time scales

1. **Headquarters:** muster squads, attach officers, equip troops, configure formations, train support staff, order production, repair losses, spend materials and choose deployment.
2. **Strategic territory map:** conquer resource districts, control supply routes, watch enemy movement, manage logistics and choose operations; turn the economy into capacity to wage future battles.
3. **Tactical tile field:** issue orders to squads and individual specialists; build/repair field defenses; conduct synchronized attacks, retreats, ambushes and combined-arms operations. Terrain and enemy intentions matter throughout.
4. **After-action:** casualty treatment, salvage disputes, repair quality, prisoners, equipment degradation, character trauma and veteran legacy, faction responses, operating expenditure; no universal costless resurrection.
5. **Inspection runs:** enter a squad formation's interior, a weapon's micro-world, an armor plate's memory, a base facility's substructure or the world's strata; fight through contiguous rooms to upgrade the owning object.
6. **Reclamation / prestige:** at level 100 of all required campaign keystones, permanently rewrite one law of the next world and begin a harsher cycle. True ending only after at least ten complete world-cycle prestiges.

A **command pulse** is the atomic combat step (proposed: 250 ms logical time). Units can run orders continuously for classic RTS intensity, but accessible tactical pause and step-by-step order execution are first-class modes using the same simulator. The game cannot depend on head/hand twitch reflexes; tactical skill is route planning, economy, situational awareness, squad composition and precisely timed commitments. Both play modes use one authoritative, deterministic order-resolution kernel.

An operation usually lasts 15–45 minutes early, and escalating campaigns include consecutive deployments with attrition, restricted resupply, and multi-stage sieges. Inspectable rooms may be short engagements (2–10 minutes) but their composition becomes harder, not merely more numerous.

## 3. Content depth and world structure

Design targets, **not claims of produced content**:
- **100 character levels** per commander, troop and several independent mastery tracks.
- **12 war theaters × 50 strategic sectors = 600 unique sector nodes** with territory ownership, adjacency, fixed seed and persistent scars.
- **64 inspection labyrinths × 64 rooms = 4,096 addressable room-depth nodes per ascension**, with bounded deterministic generation and distinct boss/terrain/objective tables. This establishes thousands of rooms even without replaying a single node.
- **10 or more complete ascension cycles**; each cycle changes invasion patterns, faction diplomacy, enemy counters, structural restrictions, terrain anomalies and world rules, not merely a multiplier.
- **Seven encounter scales:** skirmish, raid, base defense, interdiction, siege, theater offensive, world fracture.
- Battle-map sizes proposed: 24×24 training, 48×48 normal, 64×64 epic, optional streamed sectors beyond; do not render all nodes at once.
- Strong fixed catalogs at first, then seed-based composition. Stage generation must reject unwinnable objective placements, inaccessible spawns and impossible extraction routes.
- No fabricated uniqueness: a seeded room is a variation unless its enemy doctrine/objective/environment meaningfully changes the tactical decision.

Persistent world revision: captured roads, ruined bridges, contamination, depleted deposits, morale, diplomacy and infrastructure survive battles and influence later encounters.

## 4. Deep tactical rule system

### 4.1 Strategic and tactical layers
The overworld displays owned land, supply capacity and faction intentions. Clicking a sector reveals its battle map; an operation carries a logistical manifest (fuel/ammo/rations/medic/repair, engineering slots, reserve squads, artillery tokens). Supply in the field is physically routed via depot/convoy/routes rather than teleported.

Each formation has a 3×3 **deployment room** (front/middle/rear ranks). Slots represent troop cells with class, equipment, fatigue, wound, adjacency, formation aura, sight and suppression. An officer commands the formation but may be wounded, killed or replaced. A map token is one formation, and expanded command view exposes its members. Area weapons damage multiple room slots and line-of-sight geometry. Routing and retreats can break a formation into survivors.

### 4.2 Command resolution contract
A proposed pulse resolves in an explicit immutable order:
1. Validate inputs and reserve shared resources.
2. Deliver orders by command network; delay, jam or reject beyond command capacity.
3. Pay movement, fuel and transport costs; resolve deterministic occupancy/contention.
4. Discover contacts, recompute fog for authorized observers; report concealment/detection.
5. Apply stances, suppression effects, charge/windup, overwatch and reaction-fire windows.
6. Resolve ranged attacks, penetration, barriers, shields, cover destruction, fire/poison/EMP.
7. Resolve close assaults, counterattacks, tackles, grapples and position displacement.
8. Evaluate morale/cohesion, order breakage, retreat, surrender and commander casualties.
9. Apply environment damage, weather, fires, contamination, structural collapse and supply drain.
10. Adjudicate objectives, control/capture, turn transitions, casualty/loot records and seeded events.

Order conflict and simultaneous kill behavior must be specified and tested. Nothing in per-eye render callbacks changes simulation or RNG.

### 4.3 Attack anatomy
An attack is not a scalar punch. Proposed ordered layers:
**target visibility → aim confidence → range/falloff → movement/evasion → cover → directional guard → shield interaction → armor material vs damage vector → penetration/ricochet → elemental/arcane status → morale/cohesion → wounds/critical subsystem failure → residual hazards → counter/reaction.**
Damage vector channels: kinetic, piercing, explosive, thermal, cryogenic, electrical, chemical, psychic, siege. Defense materials: cloth, ceramic, composite, plate, force barrier, organic, crystalline, fortified. Fully data-driven interaction tables with deliberately nontransitive counters.

Deterministic formula sketch (subject to balance proofs):
`hit = clamp(0.05,0.95,accuracy + training + command_bonus + flank - cover - motion - weather - suppression)`.
`effective_penetration = weapon_pen + angle_bonus - material_resist - range_penalty`.
`post_armor_damage = raw_damage * matchup_multiplier * cover_multiplier * angle_multiplier * penetration_function(effective_penetration)`.
Every input is inspectable in a compact battle explanation log. Maintain bounded calculations and use wide fixed-point / saturating integers for extreme endgame stats; never allow wraps, NaN, zero-divisors or RNG-dependent desync.

### 4.4 Attrition and command pressure
Track ammunition magazines and reloads, heat/cooling, barrel wear, jam chance, armor condition, wounds, bleeding/medical stabilization, fatigue, supply, initiative, suppression, cohesion, veterancy shock and local leadership. Units may rout, become separated, hold without ammunition, surrender or disobey unsafe orders. Commander survival matters: kill the command relay and tactics degrade even if hit points remain high.

Resource pressures: requisition, alloys, relic components, fuel, medical supplies, munitions, manpower, political authority and infrastructure throughput. No single currency purchases victory. Factories have throughput, storage, power needs, queues and vulnerabilities.

### 4.5 Combined-arms interactions
Infantry captures/holds and enters hard cover; engineers breach/bridge/mine; artillery uses observers and risks counterbattery; armor controls open lanes but fears flank and terrain; scouts reveal and spoof; air assets need local logistics/anti-air suppression; support units relay commands/heal/recover; elite named squads unlock formation techniques rather than effortless raw dominance.

Terrain properties are composable: elevation, slope, cover angle, traversability by movement class, support strength, water/mud, burning, electricity conduction, stealth, visibility, destructibility, supply passability and environmental resonance. Terrain reactions propagate with event budgets so chain explosions do not freeze a headset.

### 4.6 The cruel but tactical systems
- **Initiative debt:** borrowing extra actions pushes future orders back; high-power combos create dangerous openings.
- **Counterdoctrine:** repeated player tactics improve enemy counter-intelligence and spawn appropriate counters.
- **Command bandwidth:** orders per pulse limited by radios, signal clarity, field officers and relay logistics.
- **Casualty legacy:** defeated veterans leave recoverable tags, lineage knowledge and scars; taking the risky extraction route matters.
- **Chain command:** coordinated encirclements, artillery marking, morale collapse and terrain chains generate escalating tactical multipliers that are earned, exhaustible, disruptable and visible.
- **Risk/resolve:** defend wounded veterans or abandon them to claim objectives before enemy relief.
- **World law mutations:** late-game biome rules alter physics (e.g. reversal of thermal exchange, acidic rain, hostile gravity) with concrete interaction-table changes.
- **Fatigue/trauma:** veteran expertise does not remove the cost of indefinite battle; treatment and leave are operational choices, not calendar timers.

## 5. Progression: independent, connected, absurd

### 5.1 Level-bearing systems
Every layer levels independently through relevant *active deeds*; none levels just because the player's commander gained experience:
1. **Commander** — operational planning, kills, captures, successful withdrawals, command quality, leadership under fire.
2. **Individual soldiers** — weapon proficiency, class mastery, endurance, discipline, survival, personal battle honors and scars.
3. **Squad/formation room** — rank slots, adjacency synergy, room passive, command capacity, formation shape, interior floor mastery.
4. **Weapons** — receiver, barrel, ammunition affinity, reliability, range, handling, attachments, inner world, bound specialties.
5. **Armor** — materials, seam integrity, directional guard, resistances, shielding, internal chambers.
6. **Accessories** — radio relays, relics, insignia, field packs, talismans; each has an explorable imprint and progression.
7. **Base** — facilities, specialists, grids, defense works, resource flows, fabrication methods, recovery beds and doctrine archives.
8. **World** — theater security, corruption, civilization restoration/destruction, planetary infrastructure and law.
9. **Faction/doctrine** — diplomatic standing, reputation, research, counter-intelligence and command philosophy.
10. **Legacy** — across-cycle permanent unlocked possibilities with narrow stat inheritance and new enemy adaptations.

Each system needs at least one *mechanically new capability* at a tier breakpoint (suggest 5/10/25/50/75/100), not only bigger stat numbers. Between breakpoints, modest measurable improvements should still matter.

### 5.2 Effective grind-time specification
This is a **calibration target for active dangerous combat**, not a wall-clock cooldown or a fake promise about real player schedules. For one isolated mastery track of levels 1–100 at a reference equal-threat win rate:

`H(n→n+1) = 5.70084455 * exp(0.0336257141*(n-1)) effective combat hours`, n=1..99.

Under this benchmark:
- Level 1→5: **24 effective combat hours**.
- Level 5→10: about **35 hours**.
- Level 20→25: about **58 hours**.
- Level 50→55: about **159 hours**.
- Level 95→100: **720 hours (~30 uninterrupted days)**.
- Level 1→100: about **4,486 hours on one track** at that calibration.

These are intentionally extreme *initial design settings*. Independent levels can rise concurrently from one battle, so summed pillar-hours are not total gameplay time. Skilled wins and dangerous targets can yield faster progress; poor tactical performance can be much slower. A tenfold complete progression at constant speed would imply about **44,860 hours**, before prestige acceleration, overlap, and later hostility. Profile honest player-time telemetry and revise to preserve the intended exponential feeling without adding artificial wait gates.

Implementation rule: express experience as event-weighted "effective hours" in balance data, not real-clock hours. Rewards depend on encounter risk, active participation, objective quality and depleted resource opportunity cost. Repeat farming a trivial enemy yields diminishing mastery, but a genuinely difficult repeat encounter remains useful. No hidden punishment for being offline.

### 5.3 Incremental power and equipment
One battle might improve: +0.1% stability, +1 armor seam quality, +1 command relay integrity, weapon calibration progress, a recruit's proficiency, a material research fraction and a faction's tactical dossier. The player can open one consolidated **After Action Ledger** showing only gains and meaningful losses; no sprawling nested modal for every increment.

Weapons/armor/accessories have rarity, manufacturing origin, temper, failure mode, sockets, bonded traits, inspection depth, resident anomalies, durability history and skill compatibility. A weapon can be level 100 with a badly trained barrel or specialized versus infantry but weak against armor. A low-tier weapon with perfected internals can outperform unrefined legendary loot in its niche. Reincarnation moves selected earned qualities into a re-forged base but carries material and identity tradeoffs.

### 5.4 Room inspection and the Thousand Chambers
Every formation and capital item has a deterministic interior campaign keyed by object ID, creation seed, ascension number, room depth, local mutation and state of prior inspections. A room is an actual tactical encounter, puzzle, hazard, ambush, supply/survival decision or boss; not a click-to-claim percentage. Conquering room guardians expands slot capacity and increases the formation/object effect. Defeated guardians may return altered if the parent item is damaged.

Room types: workshop, furnace, memory vault, sealed barracks, parasite nests, tribunal, fractured simulator, inverted armory, clockroom, reservoir, logistics maze, sentry chamber, tyrant bastion and ancestral command bridge. Each has rules that affect tactical plans.

Deep inspection requires an **expedition manifest**: what can be carried inside, how many squads can follow, how retreat works, which equipment can be recovered after wipe, and which resource shortages remain on extraction. Long runs have safe exits at known intervals; player death must be costly, but not irrecoverably destroy all campaign data.

## 6. Full-world prestige and the true ending

A **World Reclamation** is not available until the following cycle keystones independently reach 100: main commander, selected active expedition force (all deployed core squads and their active members), every equipped weapon/armor/accessory on that core force, flagship formation room, central base, campaign world, and primary doctrine. Individual junk items and undeployed reserve recruits do not block the cycle: those are irrelevant inventory, not an authentic requirement.

At the first Reclamation, the world fractures and rebuilds as a more aggressive ascension. Retained: earned legacy points, unlocked classes/doctrines, immutable codex records, chosen narrow heirloom imprints, lineage and disclosed truths. Reset: territories, front lines, world level, base level, ordinary experience, most item levels, resources and local diplomacy. Hostile additions increase enemy intelligence, resource disruption, attrition and world-law anomalies; inheritance buys *options*, not an effortless bypass.

**True-ending gate:** World Reclamation count ≥10, each with a verified completed Level-100 covenant; all ten unique Law Seals owned; final hidden world-sector branch found; ultimate confrontation won with an assembled level-100 multi-domain expedition at cycle ≥10. A standard ending is available sooner. Never silently mark an incomplete covenant complete.

Prestige is an explicit player-confirmed irreversible campaign action with a reversible pre-commit preview and an automatically created restore checkpoint. The game must not hard-lock if the user loses a one-of-a-kind item; required unique proofs are reconstructible through very difficult deterministic routes.

## 7. Enemy AI and campaign pressure

Four AI layers, each deterministic and budgeted:
- Strategic director: doctrine, production, diplomacy, recon priorities and target selection; no knowledge of undiscovered fog positions.
- Operational planner: route/supply/capture/relief/siege decisions across nodes with bounded lookahead.
- Tactical formation AI: cover use, suppression, flanking, retreat, counterbattery and protection of logistics; individual squads follow priorities, not omniscience.
- Room adversary: floor-specific guardian scripts and mutation parameters with objective-seeking behavior.

Difficulty emerges through combined arms, reserves, commander replacement, supporting fire, position, resources, doctrine and reinforcements. Increasing numerical stats without more varied decisions does not qualify as a new difficulty tier. The AI cannot run an unbounded search on the render thread.

Campaign factions: doctrinally different rather than just recolored. Proposed archetypes: industrial attrition state, mobile raider coalition, salvage technocracy, mystic siege court, subterranean infestation, rogue logistics machine. Original proper nouns to be coined during narrative/art pass.

## 8. Art, audio, VR interaction, accessibility

**Art:** chibi/chip-mini at deliberately low pixel density, square tile boards, tiny legible silhouettes, 4–8 direction sprite facings, idle/attack/damage/action variants, faction-shape distinctions beyond palette colors, crisp pixel-art atlas, occasional ultra-low-poly terrain extrusions to convey height. Do not replace sprites with ASCII placeholders. No modern high-cost shading; keep the graphics cute enough to make the militarized statistical cruelty absurdly funny.

**Camera:** overhead orthographic-feeling tabletop in comfortable seated VR. The board may be positioned like a war table, but head translation must not be required; Daydream is 3DoF. Distant scenery is static and lightweight. A flat non-VR tactical presentation can share the same deterministic simulator later without cloning combat logic.

**Input:** shell-normalized controller actions. Head-gaze/compatible ray points at tile; touchpad holds move a tile cursor or scroll list; touchpad click selects/commands; minus/secondary opens compact command palette or cancels; circle/O Home remains shell quick menu, never a game control; volume rocker remains shell/device-owned unless platform remapping explicitly permits otherwise. Critical controls always work without tiny precision movements; pause during inaccessible input; shell recovery remains in charge.

**HUD:** one dense status band (turn/pulse, supply, selected squad health/morale/ammo/ability, forecast), optional expand-on-select inspector and scrollable roster. Tooltips expose mechanics without drowning the map. Useful commands are buttons with labels, not buttons followed by paragraphs explaining the labels. Provide log filters and exact causes for missed shots/status outcomes.

**Audio:** chip-military chiptune with escalating layered tension, synthetic bleeps, clipped unit voice barks, distinct command-accepted/rejected sounds, hit material and suppression cues; audio cues are redundant with text/visual states. No unlicensed sampled game music/effects.

**Comfort:** no artificial head jolt, flashing chains, uncontrolled camera pans, or text too small to read through the S9 headset. Always support readable-scale UI, colorblind-safe icons/shapes, motion-reduction and game action pause.

## 9. Native architecture, persistence and performance gates

- Implement as a self-contained **ReverieVR native ABI v1 module** once implementation begins; use existing docs/native contracts. Keep content and simulation game-private unless a helper is proven shared.
- Deterministic C++17 simulation library independent of GLES, headset and Android input; log `seed + initial snapshot + player orders` for reproducible battles. Enforce one logical update; two render passes merely observe immutable state.
- ECS-like fixed-handle pools or packed data arrays, not a graph of thousands of Android objects. Cache sprite atlases and immutable tile meshes once per GL context, upload dynamic overlays only when dirty.
- Lazy-load one tactical map or room and its neighbor metadata; keep the thousands of other rooms as small procedural descriptors and persisted deltas. Background strategic AI ticks infrequently with bounded per-frame work; simulations can run in private worker threads producing owned snapshots and no GLES access.
- Multilayer save schema with version, checksum, migration, recovery slot, replay header, unit/item IDs, inventory ownership invariants, ten-cycle covenant ledger, and dual-slot atomic journal. Serialization order canonical. Never tie RNG to wall clock in authoritative simulation.
- Extreme values require explicit fixed-point range policy and saturation. Preserve a human-readable display abbreviation while authoritative numbers retain precision. Test max-level, repeated prestige, invalid IDs, corrupted chunks and exhaustion cases.
- No internet, Google servers, additional APK, new heavyweight engine, or unmeasured image/postprocessing dependencies.
- Device gates: stable stereo, pause/recovery, controller loss, module exit, accessibility, memory ceiling, thermal 30/60/120-minute runs, per-eye identical tactical state, input latency, large-room seed generation, save/load and reliable offline cold boot.
- No GitHub Actions changes unless separately requested. Existing repository builds/tests should be used as they are.

## 10. Implementation roadmap and gates

Every stage creates playable, testable behavior. A menu option stays absent until it does something real.

**P0 — charter/design (this document):** define simulator contracts, XP calibration, prestige covenant, performance budget and content scale. No runtime claim.

**P1 — first battle kernel:** grid occupancy, terrain, 2 factions, 3 unit archetypes, order queue, movement, line-of-sight, 2 weapon classes, damage, objective and defeat; deterministic headless tests. Gate: identical seeds/orders produce identical checksums; full game-state after each pulse inspectable.

**P2 — native playable war table:** host module entry, 24×24 sprite board, map pan/select, command palette, enemy AI, one real win and loss, pause/quick-menu return, stereo/state separation. Gate: S9 hardware controls and readability, not only host tests.

**P3 — operational base & attrition:** factories, economy, logistics, field structures, capture, ammunition, supply routes, repairs, salvage and casualty preservation; after-action ledger. Gate: exploit-safe accounting and win/loss campaign continuity.

**P4 — formation rooms & first inspection labyrinth:** 3×3 members, rank bonuses, room levels, guardian, 16 floor types and deterministic room persistence. Gate: leaving and resuming an inspection preserves exact state and equipment ownership.

**P5 — multi-track RPG:** commanders, units, weapons, armor, accessories, base, doctrine and world XP, equipment durability, specialization, scars and inherited traits. Gate: every XP event owns its source; no double awards; no numerical overflow.

**P6 — broad tactical systems:** suppress/morale/fatigue, cover angles, flanks, ammo/heat, destructible terrain, terrain chains, overwatch, communication jamming, AI counters. Gate: comprehensive reproducible interaction-matrix tests.

**P7 — content multiplication:** reusable tilesets, 6 original factions, generators for 600 sectors and 4,096 inspection address nodes, different objectives and physically supported siege sequences. Gate: seeded structural reachability, unique-key and softlock audits across large sample sets.

**P8 — full campaign & enemy warfare:** persistent faction strategic AI, theater invasions, roads, depots, siege AI, storytelling choices, standard ending. Gate: reachable campaign win and recoverable defeat.

**P9 — endgame economy & item depth:** full 1–100 progression across all required domains, threshold unlocks, deep equip/reforge rooms, extensive balance simulation; late battles remain dangerous, not just inflated HP. Gate: modeled 1→5 and 95→100 times, honest target-vs-measured evidence.

**P10 — ascensions 1–10:** deterministic world-law remixes, Law Seals, legacy inheritance, covenant verification, no softlocks, world rewrite checkpoint. Gate: scripted simulated ten-cycle progression reaches a unique final route, while nine cycles cannot unlock it.

**P11 — finish and release:** full content/art/audio accessibility pass, dense UI cleanup, regression, S9 thermal/VR testing, save migrations, stable signed APK integration, player-facing build and full true-ending verification.

Scope rule: **Do not call P1 or P2 a completed game.** 100 levels, thousands of inspectable rooms, the full combat rules, and ten prestiges are release requirements, not optional stretch goals. Smaller maps are valid for dev/testing; reduced rules or maps are not substitutes for final scope.

## 11. Verification ledger — pending by design

- [ ] Working original name, faction identity and production art bible.
- [ ] Testable simulation data contracts and exact event ordering.
- [ ] Full interaction-table balance spec and ability lexicon.
- [ ] 1–5 / 95–100 XP profile calibrated using actual combat outcomes.
- [ ] One deterministic reproducible sample skirmish including loss/recovery.
- [ ] Formation inspection persistence and hundred-room stress tests.
- [ ] Thousands-of-rooms generation, solvability and collision checks.
- [ ] Tenfold covenant/true-ending gating and migration checks.
- [ ] S9 headtracked table readability, 3DoF Daydream input and heat checks.
- [ ] Actual compiled module, APK integration, on-device smoke tests.

## 12. Source references (mechanical inspiration only)

- Soul Nomad & the World Eaters, Inspection FAQ: https://gamefaqs.gamespot.com/ps2/935509-soul-nomad-and-the-world-eaters/faqs/50459
- Soul Nomad & the World Eaters, walkthrough on randomized inspection dungeons: https://gamefaqs.gamespot.com/ps2/935509-soul-nomad-and-the-world-eaters/faqs/80435
- Disgaea Item World mechanics reference: https://strategywiki.org/wiki/Disgaea%3A_Hour_of_Darkness/Item_World
- Governing repository context: `AGENTS.md`, `docs/project/INSTRUCTIONS.md`, `docs/project/EXECUTION_CONTRACT.md`, `docs/native/README.md`, `docs/native/CONTRACT_INDEX.md`.

**Project decision boundary:** this is a planning charter and implementation target, not a statement that any game code, art, rooms, APK or source tests are complete.
