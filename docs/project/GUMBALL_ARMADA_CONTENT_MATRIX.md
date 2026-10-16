# Gumball Armada — Content and Production Matrix

**Status:** proposed *release content targets* and integration definitions. These numbers are **NOT** implemented content counts. Every entry must be tagged PLANNED / AUTHORING / CODED / INTEGRATED / DEVICE-VERIFIED with artifact IDs and proof.

**Parent roadmap:** GUMBALL_ARMADA_STAGED_PLAN.md. **Target:** original Galaxy S9/Daydream game, single ReverieVR APK, offline after install. Original candy-warfare work, not a Kingdom Hearts or Candy Crush asset port.

## 1. Minimum content evolution

| Content family | M1 first run | M2 combat+ship | M3 sandbox | M4 first general | M5 four generals | M6 release target |
|---|---:|---:|---:|---:|---:|---:|
| Functioning equipable block archetypes | 0–4 fixed | 10+ | 24+ | 32+ | 56+ | **64+** |
| Distinct working primary weapon families | 1–2 | 3+ | 6+ | 8+ | 10+ | **12+** |
| Defensive/support/active families | 1 | 6+ | 10+ | 12+ | 18+ | **22+** |
| Mechanically unique enemy archetypes | 3+ | 6+ | 12+ | 16+ | 28+ | **32+** |
| Unique general flagship bosses | 0 | 0 | 0 | 1 | 4 | **4**, ≥3 phases each |
| Authored main story missions | 0–1 | 3–4 | 6 | 18 | 54 | **60 target** |
| Traversable/revisitable chart nodes | 1 combat route | several | 4096-address audited graph | same | same | **4096 addressable** |
| Shop/vendor archetypes | none or starter garage | starter purchase | 5 | 5 | 5+ | **5+** |
| Authoritative voice announcer triggers | text only → 3 audible | 8–12 | 16 | 20 | 30 | **40+ original lines** |
| Distinct soundtrack cues | 0–1 | 3 | 5 | 8 | 16 | **18+ loops/stingers** |

These are planning scale targets, subject to measured memory/build/performance budgets; changing a target requires a recorded decision and preserving the gameplay function. A counted variant requires a distinct real gameplay/visual/audio identity, not a renamed stat-only duplicate. Narrative mission target model: **6 prologue + 12 per general × 4 + 6 finale = 60**. Side contracts and seeded encounters are extra and do not fake-authored mission count.

## 2. Modular block catalog — minimum 64 functional archetypes

Each archetype must have: stable part ID; inventory icon; flight-rendered geometry/sprite; build-grid footprint; class/hardpoint/connection type; mass; structure HP; shop/salvage acquisition; cost; power/heat behavior; repair/quality rules; sound/VFX hooks if active; data validation; save/load; and a real test scenario. Mere texture variants do not count toward functional block total.

| Family / planned count | Candidate parts (first-to-late examples) | Gameplay reason |
|---|---|---|
| **Hull and structure — 12** | gumball core, licorice truss, wafer flat panel, nougat bulkhead, caramel brace, frosted nose cone, honeycomb connector, pocky winglet, biscuit keel, peppermint socket, sugar lattice, hardened candied prow | Allowed structural connectivity, armor layout, silhouettes and mass |
| **Power and propulsion — 10** | fizzy thruster, cola booster, bubble ion nacelle, sugar reactor, mint condenser, hard-candy battery, taffy torque engine, heat vent, waffle coolant manifold, fusion sherbet core | Energy, thrust, heat, sustained fire and boost performance |
| **Weapons and mounts — 14** | sugar-pea repeater, rock-candy cannon, licorice lance, syrup beam, gumdrop missile rack, popping burst mortar, peppermint scattergun, fizzy flak turret, caramel railgun, sugarburst chaingun, lemon-neutralizing prism, ribbon mine launcher, sour scrambler gun, royal siege launcher | Real distinct fire modes, trajectories, status effects and intercept capability |
| **Defenses — 10** | nougat plate, ablative waffle shield, bubble bubble-generator, crystal deflector, sourproof coat generator, gumdrop reactive panel, caramel repair foam, hard-candy point defense, reflective lollipop dish, emergency icing bunker | Threat-counter variety and localized ship survivability |
| **Utilities — 10** | taffy tractor, radar gumdrop, hidden-route scanner, cargo jar, repair drone cradle, loot magnet, trader transponder, salvage grappler, fuel condenser, beacon relay | Exploration, capture, trade, recon and salvage |
| **Elite / faction / specials — 8** | Scrunch battering ram, Cringe cloak emitter, Cringe fake beacon, Pucker acid scrubber, Pucker corrosion projector, Sauer command uplink, Sauer interference field, Royal crown overdrive | Boss-derived unlocks, alternate builds and mission solutions |

**Important accounting:** weapons/defenses/specials above are *physical block archetypes*. Their associated attack/active **mechanic** is counted as functional only when used in battle, with a distinct outcome, supported counterplay and real effects. Variant skins are a separate cosmetic library, not extra working blocks.

**Ship architecture rules:** bounded construction grid for Galaxy S9; connected core and legal mount sockets; mass/engine thrust budget, generator load, capacitor/heat, aim arc and cargo volume. Different arrangement must change flight behavior, not just shape. Save component instance state, not only blueprint summary. Early stat-only upgrades require deliberate migration to physical components so user progress survives.

## 3. Weapons, defenses and abilities: feature-completeness checklist

Weapons to prove in combat (not only purchase menus): rapid-fire repeaters, heavy direct cannon, penetrating lance, constant beam, homing missiles, explosive mortar, pellet spread, flak anti-projectile, charged railgun, deployed mine, corrosion counterbeam and siege artillery. Weapon balance axes are real effective DPS, burst, energy, heat, effective distance, enemy tracking, target resistances, ammo cost, cooldown and mount weight.

Defenses must include physical hull armor, regenerating shield, ablative layer, reflection, anti-corrosion, active interception, emergency repair and at least one risk/reward defense (e.g. overload plating). Incoming hit types and weakness must be readable; no invisible resistances.

Active ability suite (at least 8 options by release): Gummy Dash, Bubble Burst, Mint EMP, Sugar Overdrive, Taffy Tractor, Decoy Sweets, Icing Repair, Royal Barrier. Some may be contained in elite special block mechanics rather than arbitrary free powers. At least 3 accessible in M2; every ability consumes declared charge/cooldown/resource and has actual visuals, sound, UI feedback and enemy interactions.

**Controller gate:** Daydream has ONE clickable pad and no dedicated second trigger. Steering works on pad contact regardless of click. Click is held fire. Abilities need a deliberate selectable overlay, short-command combination proven not to conflict with fire, or supported remappable alternate controller. Menu/Home are shell-reserved; do not commandeer volume rocker by default. Make controls fully discoverable via contextual HUD and pause-safe quick reference.

## 4. Enemy fleets and Sour regional differences

| Campaign | Region look/environment | Distinct opponents and tactics | Flagship phases / counterplay | Key component rewards |
|---|---|---|---|---|
| **Sally Scrunch — The Crushing March** | compressed nougat bulwarks, striped fortress lanes, candy-metal wreckfields, lumbering convoy architecture | ram skiff, armor turtle, candy barricade gunship, heavy escort, bunker carrier, siege cannon, crushing mine, elite juggernaut | 1 armored perimeter, 2 crushing dash, 3 exposed core and heavy escort reinforcement; armor piercing / weak points | ram prow, siege hardpoints, heavy plates |
| **Captain Cringe — The Twisted Passage** | warped candy tunnels, false neon signals, optical taffy distortions, purple ambush webs | mimic beacon, cloaked fighter, trap planter, stealth bomber, jammer escort, decoy cruiser, flank interceptor, elite illusionist | 1 decoys, 2 blinded chart and mines, 3 reveal true command bridge; scanners/flares/ECM | cloaking, scanner, deception gear |
| **Pete Pucker — The Citric Front** | acidic lemon seas, effervescent clouds, citric refineries, luminous corrosive rivers | lemon acid skiff, caustic torpedo ship, chemical tanker, refinery turret, corrosion bomber, bubble drone, sour mist carrier, elite citric gunship | 1 long-range barrage, 2 corrosive pursuit, 3 catalytic vulnerability; coating/repair/cooling | sourproof plating, acid scrubber, corrosion weapons |
| **Senator Sauer — Bitter Parliament** | bitter black-gold capital, state propaganda towers, bureaucratic docking rings, mixed fleets | elite royal-turncoat guard, customs interceptor, jamming cruiser, shield command carrier, political blockade platform, artillery frigate, mixed gunship, capital escort | 1 coordinated fleet, 2 relay sabotage, 3 senator flagship core; allies/reputation/targeted strike | command uplink, jamming, highest tier reactors |

Eight roles per territory is the planned **32** distinct enemy-role catalog. Shared baseline AI may be reused for basic physics, but palette swaps cannot satisfy "distinct roles." Each has visual ID, collider, motion logic, attack, status/resistance, death, drop, danger icon, mission usage and test.

**Boss gate:** four individually authored flagship silhouettes (not enlarged ordinary fighters); at least three real phases each, component targets, transitions, telegraphs, unique voice/music, flags and campaign aftermath. At least one boss in each region must exploit the region's new gameplay system, not simple numerical HP scaling.

## 5. Exploration and procedural generation without hollow scale

**4096 addressed chart nodes** remain a design target inherited from the prototype, with four sectors of 1024 addresses. In M3 they must become a stable persisted **connected graph** with safe backtracking, fog-of-war, optional shortcuts, stations, navigation, faction control and events. Do not imply 4096 manually built 3D scenes. Generate a bounded mission scene on entering a node with deterministic seed + stored changes.

Suggested node/scene grammar, minimum 12 classes: safe port, neutral market, Royal checkpoint, Sour customs, salvage graveyard, syrup river, asteroid candy mine, crystalline nebula, ambush passage, defense station, trader convoy crossroads, fortress lane, hidden vault and story landmark. (More than 12 examples are available; document each class actually implemented.)

For each lane, generate from independently configurable tags: environment skin, hazard physics, enemy formation, mission objective, encounter/event outcome, salvage table, faction response and soundtrack stem. Procedural selection must avoid **mechanically identical runs** even when lanes use the same map asset.

**Authored campaign spine:** 6 introductory missions, 12 missions for each general and 6 finale missions (target 60). Each authored mission requires intro, objective, meaningful enemy/tactical context, win/loss criteria, rewards/unlocks, dialogue/caption, world-state change and evidence. Side jobs (escort, hunt, intercept, salvage, defend, scout, raid, bounty, courier and blockade) should be truly reusable systems but do not inflate the authored count.

Travel screens must show location and risk/reward with minimal nested UI. Use a zoomable/scrollable map and a compact detail rail; avoid cluttered tabs for every feature. A player should see the next mission, vendors, ship readiness, cash and route choices at a glance.

## 6. Economy, stores and actual ownership

**Five vendor types:** Royal Outfitters (reliable baseline), Scrap Bazaar (damaged cheap equipment), Citric Black Market (illegal/high-end), Mobile Trader (changing route inventory), Faction Quartermaster (conditional unlocks). Inventory differs by quality, tier, license, region and reputation.

**Gumball Cash rules:** mission payout + verified combat combo + legal salvage + contracts + trade; sink through new blocks, ammo, repairs, hulls, cosmetics, special consumables, research and travel fees. No arbitrary unlimited free credits, time-loop shop refresh exploit or duplicate save-item transactions.

**Cargo and piracy:** physical cargo capacity affects collection. Equipment found during combat has acquisition/quality/damage state. Stealing involves an enemy ship capture/boarding/tractor encounter, risk, detection and possibly fencing restrictions; no inert "steal" button.

**Fail-safe progression:** player can recover a stripped ship via a baseline loaner/starter contract or cheap repair, never end up permanently unable to buy fuel/repair. Progression cannot demand a singular ultra-rare drop from an optional branch.

**Commander experience:** plan 1–50 rank curve (proposal, not hard requirement); six skill disciplines: piloting, gunnery, engineering, leadership, salvage and mercantile. Tier gates should expose better strategies without making enemies turn into disproportionate HP sponges. Ship tiers I–V form a separate construction/mount budget.

## 7. Audiovisual asset and integration manifest targets

| Asset class | Release planning target | Device proof requirement |
|---|---|---|
| Player hulls / build-class presentations | 5 hull classes + assembled variants | Compare two distinctly built, flight-rendered and performing ships |
| Functional block icons/geometry | ≥64 item icons and correctly placed render representations | Trace component ID → inventory → shipyard → flight → damage |
| Enemy silhouettes, animations | ≥32 recognizable archetypes, necessary firing/hit/destruction frames | Screenshot matched to spawned enemy ID |
| Major flagship art/animations | 4 ships, ≥3 authored phase presentations each | Capture each phase, destroyed weak points and explosion |
| Region environment sets | 4 themed region palettes, ≥12 route/setpiece families | Screenshots on actual headset / both eyes |
| VFX sheets | ≥40 distinct functional events: fire, trails, impacts, buffs, block failures, bosses, loot | Event triggered at gameplay time, bounded GPU/CPU |
| Interface/portraits | menu, chart, shipyard, store, mission, upgrades, results, narrative portraits and captions | Headset-readable, game-state truthful |
| SFX | ≥60 original/right-cleared cues with variation for weapons/blocks/UI/events | Audible capture, player preference controls |
| Deep-announcer VO | ≥40 finished original voiced callouts, multiple combo levels and events | Real game event → voice stream and caption |
| Music | ≥18 original/right-cleared loops/stingers: areas, battles, bosses, finale, credits | Loop/transitions audible, no missing stems |
| Story illustrations | prologue, 4 general reveals/defeats, finale and selected events | Shipped and actually triggered in context |

**Manifest requirements:** approved source art path, transformed/runtime path, SHA-256, license/provenance, work/artist prompt notes where applicable, type/format/resolution, GL texture compatibility, atlas coordinates or audio bank event ID, gameplay owner, status and evidence capture. Do not mistake a generated visual in a chat for a repo-owned game asset. Each batch is committed to main, read back, then runtime-binding-checked. Missing render IDs and silent fallback art fail the gate.

**Performance method:** prove useful sprite/impostor scaling. Enemy craft, aircraft, projectiles, far scenery, explosions, character portraits and HUD may primarily use 2D sprites, in a genuine stereo low-poly 3D cockpit / tunnel / large capital ship spatial scene. Use atlases/packed geometry and explicit scene streaming; never instantiate 4096 transit routes in memory at once.

## 8. Voice, announcer timing and sample content

The announcer should be extremely deep, absurdly serious and slightly theatrical, giving big moments rhythm like an arcade combat sport commentator. Use an **original recording/performance** and phrases, not ripped Candy Crush clips or imitation of an identifiable actor. Seed voice levels:
- Combo tier 3: "SUGAR RUSH!"
- Tier 6: "SPECTACULAR!"
- Tier 10: "ABSOLUTELY CONFECTIONARY!"
- Tier 15 or general death: "SOUR ANNIHILATION!"
- Clean run: "UNTOUCHED BY THE SOUR!"
- Counterattack: "TASTE THE REVENGE!"
- First boss phase: "THE ADMIRAL APPROACHES!"
- Flawless general defeat: "A MOST DISTURBINGLY SWEET VICTORY!"

Define idempotent event IDs and bounded priority: fatal hull warning > mission instruction > boss tell > victory > high combo > pickup > routine score. Lower-priority phrases are skipped or delayed, never chatter over serious warnings. Duck music briefly, obey separate VO slider and mute, maintain text captions with speaker marker and time-out. Audio is updated from authoritative game events once per tick, not double triggered by two rendered eyes. Record actual audio event-to-mixer trace and headset sound verification.

**Important current constraint:** existing native host sound cues are a small fixed menu, not a packaged arbitrary-voice bank. Roadmap must allocate a platform-safe sound integration with ABI/data review and Android lifecycle tests; do not label VO as implemented because screen text exists.

## 9. Per-stage asset bundle and entry/exit discipline

At each stage, deliver a coherent bundle: **(a)** working mechanics, **(b)** original art/audio needed to recognize them, **(c)** persistence/economy changes, **(d)** regression tests, **(e)** device/build evidence and **(f)** source/provenance manifest. A content item is counted only when every relevant dependency is satisfied.

Minimum sample milestones:
- **M1:** one skybox, cockpit, player ship, 3 enemy types, mini-boss, first effects/audio and cash/run UI.
- **M2:** first working shipyard of ≥10 blocks, ≥3 distinct guns, ≥3 active abilities, multiple defenses, 8–12 voiced line seed bank and themed first atlas.
- **M3:** ≥24 functional parts, multiple shops/mission types, working inventory, connected 4096-address graph, persistent regions and reproducible salvage.
- **M4:** all Sally art/VO/boss/music, first authored chapter and post-defeat changes.
- **M5:** all Cringe, Pucker and Sauer campaigns with their unique enemy/parts/areas/music/bosses and progression.
- **M6:** ≥64 functional part archetypes, ≥32 enemies, 4 flagships, completed 60-mission mainline target, ending, full asset/voice/audio pack and shipped phone evidence.

No requirement to produce everything in one model response. Deliver one verified vertical slice per working window; build depth before multiplying catalog breadth.
