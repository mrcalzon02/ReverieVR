# Omega Front — Fractured World and Environment Design
**Project:** RV-GAME-06 / Omega Front  
**Maturity:** PROPOSED design; neither map content nor game mechanics are implemented  
**World canon:** [OMEGA_FRONT_WORLD_BIBLE.md](OMEGA_FRONT_WORLD_BIBLE.md)  
**Faction design:** [OMEGA_FRONT_FACTIONS.md](OMEGA_FRONT_FACTIONS.md)  
**Hearthwell Emergence / commander:** [OMEGA_FRONT_HEARTHWELL_PROLOGUE.md](OMEGA_FRONT_HEARTHWELL_PROLOGUE.md)  
**70-campaign war:** [OMEGA_FRONT_GRAND_CAMPAIGN.md](OMEGA_FRONT_GRAND_CAMPAIGN.md)  
**Mechanics:** [WAR_WITHOUT_END.md](WAR_WITHOUT_END.md)

## 1. A physically broken planet, not a purple force wall

The planet was **literally torn in half** in the technomantic Rupture associated with the Ouroboros Corporation. The starting **Hearthwell Emergence** is explicitly a recent morphological transformation of remote **wasteland into Eden**, not an Eden Zone that somehow survived from before the Rupture. The exact mechanics holding the remnants together, whether the halves are mutually visible, how gravity works and how oceans are retained **remain unresolved world canon**. Artists should explore exposed continental strata, unnatural horizons, enormous rift-scar cliffs and strange sky phenomena, without declaring speculative astrophysics to be established history.

Three alternate concept directions, NOT simultaneous facts:
- **Worldscar:** a broken edge with massive exposed geology descending into a disorienting gap. Most visually direct; useful as remote terrain art and late campaign battlefield edge.
- **Counter-Horizon:** a second tremendous planetary fragment beyond the torn sky. Only use as a draft; implies visibility and relative orientation that has not been approved.
- **Seam Weather:** shock-distorted cloud layers, suspended debris and shafts of impossible light above the fractured boundary. Can establish menace without explaining cosmic topology.

The key visual juxtaposition is **an almost idyllic farm, a ruined tank, ancient infantry marching past a distant spacetime wound**. Chibi/chip-mini sprites are deliberately adorable; the land is not. The reference platform is a Galaxy S9 Daydream tabletop, so the edge/horizon is a compact low-poly static backdrop or baked card, never a globally rendered 3D planet.

## 2. The four world environment categories

Four **canon names** determine distinct gameplay systems. The implementation below is proposed:

| Region | Canon definition | Economic role | Environmental battle constraint |
|---|---|---|---|
| **Eden Zone** | Habitable by unprotected, unshielded, unaugmented baseline humans | Agriculture, water, civilian life, settlement, legitimacy and regional industry | Fragile production, populated terrain, contested waterworks, evacuation, sieges |
| **Wasteland** | Ruined land outside comfortable safe territory | Salvage, highways, wrecked industries, forward bases | Toxic pockets, difficult roads, irregular concealment, ambushes, scrap-fed enemies |
| **Barren Zone** | Extreme hostile/unlivable territory | Rare minerals, shortcuts, sealed relic vaults | Hydration and respiration requirements, thermal burden, fuel consumption, exposed lanes |
| **Anomaly Field** | Territory distorted by rifts and unnatural phenomena | Technomantic study, temporal materials, endgame penetration | Local rule changes, temporally distorted orders, false contact, rift reinforcement |

**Proposed data model:** every sector has exactly one primary ecological category Eden/Wasteland/Barren plus zero-or-more anomaly-field overlays. This permits a damaged Eden Zone to acquire anomalies without automatically becoming "not Eden" everywhere. Whether geographical categories always overlap remains a world-authority decision.

**Habitability invariant:** a clean Eden Zone remains naturally breathable. Pump failure can starve or dehydrate its citizens, but cannot magically change natural air composition. Contamination, warfare or a genuine rift event can alter air safety. Ownership is not habitability: capturing Eden land by destroying its water system can give you a strategically worthless ruin.

**Population invariant:** civilian residents are not an infinite production faucet. Food, water, housing, health, trust and migration matter independently. A settlement can survive political conquest but lose population; it can be liberated yet economically ruined. Survivors don't instantly regenerate after a battle.

## 3. Twelve theaters × fifty sectors = 600 campaign territories

This retains the existing 600-sector design scale. Each theater contains **50 addressable strategic nodes**, not fifty simultaneously loaded tactical maps. Initial projected ecological budget across 600: **80 predominantly Eden, 290 Wasteland, 230 Barren; 130 of any class may carry anomaly overlays**. These are balance targets, NOT newly established canonical geography.

| Theater | Candidate name | Ecology | Main strategic contest |
|---|---|---|---|
| T01 | **Hearthfall March** | Eden/Wasteland | Player's first settlement, coveted water and first invading forces |
| T02 | **Cinderworks Belt** | Wasteland | Ruined industrial complexes, recyclable war machinery |
| T03 | **Breadlands** | Eden | Vast agricultural basin, irrigation states, refugees |
| T04 | **Chalkgrave Uplands** | Barren/Wasteland | Mountain passes, stone fortifications, mineral transport |
| T05 | **Brassline Corridor** | Wasteland/Eden | Rail artillery, strategic bridges and city chokepoints |
| T06 | **Sapphire Shelter** | Eden/Anomaly | Rare healthy ecosystem endangered by expanding rift influence |
| T07 | **Bloodroot Expanse** | Wasteland/Anomaly | Biological colonization and contaminated waterways |
| T08 | **White Salt Basin** | Barren | Dry inland sea, hot salt fields, stripmining campaigns |
| T09 | **Farwake Shelf** | Barren/Anomaly | Incomprehensible future ruins, energy dependence |
| T10 | **Mirrorstep Labyrinth** | Wasteland + anomaly | Repeating reflections and altered navigation geometry |
| T11 | **Last Escarpment** | Barren/world-edge | Visible planetary fracture scars and siege corridors |
| T12 | **Zero-Hour Front** | Barren + anomaly | Late-game temporal warfare, rift closure and world-law conflicts |

Not every theater is continuously controlled by one faction. Claim, garrison, civilian allegiance, de facto occupation and supply reach are separate state variables. Factions may establish colonies, evacuate, lose capitals, become scattered refugee remnants and later return. Content generation must support contested routes and reachability instead of placing 600 decorative map dots.

## Implemented first-frontier boundary (2026-10-15)

`app/src/main/jni/native/omega_front_frontier.h` and its host tests now implement the first named 16 strategic sectors and an *in-memory* three-operation `The Water We Own` battle sequence. This first 16-sector slice is now source-connected to a compact GLES2 strategic map and a validated full-frontier save. It remains **not** a complete 600-sector world, finished operation book, strategic AI, or 70 authored arcs; actual Galaxy S9 acceptance is still pending. In this implementation S009 Rust Orchard's first emergence is telegraphed for day four and becomes newly breathable Eden without instantly producing stocks. The three initial operations now run different win conditions: waterworks defense, contested White Orchard hold (3 pulses), and engineer sabotage of a defended field relay (2 finite components). They share the 24×24 tactical engine, but do **not** share identical victory rules; 69 later arcs remain to be authored. See [native tactical status](../native/OMEGA_FRONT_TACTICS.md).

## 4. Initial 16-sector Hearthfall March front

The first campaign opens in **T01**, on a readable 4×4 geographic neighborhood with **16 identified sectors** and 34 additional T01 campaign sectors still awaiting names and detailed layout. **Crucial timeline:** these primary-class labels show the world **after** the Hearthwell Emergence; several currently green sectors were previously wasteland and must carry a historical class-transition event in their persisted state. This is a first-playable window, not a deliberate reduction of the 600-sector target.

```text
                A                    B                    C                    D
     1    S001 Chalkwatch      S002 Ninth Camp      S003 Splinter Ridge  S004 Old Relay
     2    S005 Caravan Ford    S006 White Orchard   S007 Hearthwell      S008 Charter Gate
     3    S009 Rust Orchard    S010 Ashrail Yard    S011 Lantern Pump    S012 Toll Foundry
     4    S013 Spoor Nest      S014 Glass Cut       S015 Broken Viaduct  S016 Dry Basin
```

Full sector IDs use `T01-S001` ... `T01-S016`. Adjacent orthogonal coordinates are potential edges; not all potential edges are operational roads. Rivers, bridges, checkpoints and anomaly access have explicit separate travel requirements.

| Sector | Class / anomaly | Starting actor | Material and mission role |
|---|---|---|---|
| S001 Chalkwatch | Barren | None | Overlook, rock extraction, distant threats |
| S002 Ninth Camp | Wasteland | F06 Lost Ninth | Roman displaced camp, food demand, marching route |
| S003 Splinter Ridge | Wasteland | None | Ore ridge and high-ground scouting |
| S004 Old Relay | Wasteland | F03 reconnaissance (not occupation) | Damaged signals hub |
| S005 Caravan Ford | Eden | F16 passing refugees | Crossing and displaced civilian escorts |
| S006 White Orchard | Eden | F01 Hearthward | Farms, protected water and relief |
| S007 Hearthwell | Eden | **Player independent** | Home base, storage, shelter, repair |
| S008 Charter Gate | Eden | F02 Meridian | Toll checkpoint and legal claim |
| S009 Rust Orchard | Wasteland | None | Toxic orchard wreck and medical/salvage expedition |
| S010 Ashrail Yard | Wasteland | F04 Scrapline | Recoverable rail stock and metals |
| S011 Lantern Pump | Eden | Local unaffiliated engineers | Reservoir and waterworks defense |
| S012 Toll Foundry | Wasteland | F02 Meridian | Fuel processing, armor repair and blockade origin |
| S013 Spoor Nest | Wasteland | F12 Carrion Circuit | Recycler nest fed by battlefield wrecks |
| S014 Glass Cut | Wasteland + anomaly | None confirmed | First local rift research pocket |
| S015 Broken Viaduct | Wasteland | None | Bridge reclamation and supply artery |
| S016 Dry Basin | Barren | F14 prospectors (not occupation) | Salt and mineral extraction |

The table gives **designer-known initial actors**, not automatic player intel. Some initial faction contacts are only patrols or distant claims and must be staggered over narrative/strategic time; seventeen armies must not materialize at once. A traveling refugee party is not a full faction-held military town. Discovery depends on real reconnaissance and reports.

**Critical links and constraints:**
- S006↔S007 orchard supply road and S007↔S011 pump line provide the initial home's essentials.
- S007↔S008 checkpoint is physically passable but politically contested; treaty, smuggling or direct conflict changes its operational status.
- S011↔S015 bridge is severed until engineers rebuild it; the alternative is a costlier reroute.
- S005↔S006 crossing depends on water level, military control and built ford/bridge quality.
- S010↔S014 glass route carries an anomaly-risk gate, not an unavoidable plot event.
- F02 armor from S012 needs a serviceable S008 route and real fuel. F04's lighter salvage machines use degraded paths; F06 marches without diesel but not without food.

**Five initial operations with persistent consequences:**
1. **The Water We Own** — defend S007/S011 civilians and pump; win if settlement survives with water, lose if evacuation collapses. Destroyed pump remains destroyed after winning firefight.
2. **The Price of Metal** — negotiate, skirmish or collaborate with F04 around finite S010 wreckage; removed salvage actually leaves the sector, and uncollected vehicle debris feeds F12.
3. **Eagles at the Ford** — discover displaced F06 Roman military/camp followers and negotiate passage or fight over S005 access; logistics, language and farmland matter.
4. **The Charter Arrives** — F02 seeks S007 by title claim, toll, blockade or armed siege; player can fight, comply strategically, sabotage S012 fuel, or bargain.
5. **Teeth in the Scrapyard** — F12 threatens local machinery and villages from S013; destruction of its recyclable material supply and core arrests growth, while a sloppy previous battle increases the threat.

S014 should open once reconnaissance/countermeasures make it practical; it is available to explore early at real risk but **never a mandatory softlock**. Player settlement identity remains independent and unwritten in historical canon.

## 5. Tactical tiles, materials and cover

Terrain is composed of **base ground + elevation/slope + obstacle/cover + weather + hazard overlays + ownership**. Logical grids remain square; source and world state are independent of per-eye rendering.

| Tile feature | Movement/line of sight | Strategic consequence |
|---|---|---|
| Meadow | Most classes pass; low cover | Productive Eden agriculture vulnerable to burning |
| Orchard rows | Infantry/light vehicles; concealment | Harvest, damaged food and partial LOS interruption |
| Ruined concrete | Foot pass; wheeled slower; cover | Scrap and destructible buildings |
| Rubble heap | Limited wheels; strong directional cover | Salvage, blocked transports, fire shelter |
| Asphalt highway | Fast vehicles, exposed | Fuel-efficient convoy routes, ambush risk |
| Dirt lane | Moderate wheeled movement | Cheap to repair, vulnerable to rain/mud |
| River ford | Depends on depth and class | Bridge/interdiction/civilian crossing |
| Deep water | Boat/bridge only | Pumping, flooding, channel denial |
| Mud/slurry | Wheels slow or immobilize | Engineer mobility, vehicle rescue |
| Ridge/rock | Slope restrictions, sight advantage | Scouts, artillery spotters, remote defenses |
| Quarry pit | Limited entry, good walls | Ore, dangerous exposure and collapse |
| Trench | Foot cover | Entrenchment, anti-suppression |
| Standing wall | Blocks fire/movement until breach | Civilian shelter, fortified line, debris state |
| Burning wreck | Exposure risk, smoke cover | Fuel burn, wreck removal, reactor/supply loss |
| Toxic depression | Protection-required movement | Equipment and medical logistics |
| Salvage wreck | Cover until retrieved | Finite parts; machine swarm sustenance |
| Fracture scar | Explicit rift movement rules | Chronal effects and spatial access |
| Reflective tile | Visual decoys/anchor support | Anomaly reconnaissance and LOS traps |

**Cover is directional** and line-of-sight geometry shared across humans and AI. Destroyed structures create actual persisted rubble, salvage or contamination, not vanish into stat bonuses. High-tier combat still respects range, material resistance and ammunition rather than converting every level-100 projectile into universal penetration.

## 6. Hazard sources, disclosures, and counterplay

Every hazard has a discoverable source, explicit tile footprint, bounded duration, visible/tactile warning, reliable countermeasure and deterministic state. **Illustrative values below are prototype numbers, not balanced gameplay results.**

| Hazard | Proposed test behavior | Tactical response |
|---|---|---|
| Dust storm | LOS -2 tiles; exposed vehicle wear +1 each 10 pulses | Filters, shelter, safer routes, recon |
| Acid drizzle | Unsealed armor -1 condition each 20 exposed pulses | Sealed carriers, canopy, repair |
| Spore wind | +1 exposure per 5 pulses; debuff at 10 | Masks, sterile firebreaks, destroy nest |
| Heat bloom | Heat dissipates at half rate; hydration demand rises | Shade/coolants, shorter bursts, water |
| Mud season | Wheeled movement cost ×2, tracked ×1.5 | Road works, tracks, alternate routes |
| Chronal drag | +2 command pulses latency through boundary | Signal anchor, flank, disable source |
| Chronal rush | Shorter windup, +1 future initiative debt | Limited salvos, control engagement |
| Mirror false contact | Confuses low-grade reconnaissance | Probes, high-grade sensors, break anchor |
| Debris fall | Two-pulse forecast on impacted tiles | Evade, hard cover, reinforced shield |
| Gravity inversion | Displacement/fall vector changes in bounded zone | Avoid ledges, tether, disable anchor |

Environmental effects aren't arbitrary encounter RNG and must never run twice because two eyes draw the same VR frame. Forecasts vary with scouting, but world rules and hazard triggers are common to AI and player. Long-run dynamic hazards have finite propagation budgets so a fire spreading across 64×64 tiles never stalls frame pacing or becomes an unbounded infinite explosion loop.

**Environment-specific enemy interactions:** F12's Carrion Circuit consumes metal wreck mass; F13 Red Bloom feeds on actual biological substrate; F14 Quarry Directive physically removes geological tiles; F15 Vitreous Court needs anchored reflective geometry; F17 Hourless Regency needs charged temporal anchors. These are real consumable/interruptible systems rather than permanent free summons.

## 7. Ecosystem, civilian and infrastructure rules

At minimum, persistent sector state tracks: `region_id, owner, resident_allegiance, population, habitat_safety, water, food, power, manpower, raw_materials, salvage_mass, rift_pressure, contamination, roads, bridges, warehouses, hospitals, depots, defensive_works, recent_siege_scar, civilian_exodus`.

Water treatment facilities, pumping pipes and reservoirs are individually damaged/repairable. Farmers and civil labor are part of the production economy; losing a harvest damages months of simulated campaign production, but **not literal real-world waiting**. Evacuations require real transport capacity and safe route reservations, not a diplomacy button that teleports civilians.

**Supply conservation:** fuel, ammunition, repair components, food, water, salvage and construction material exist in finite depots/vehicles. Convoy transfer debits one source and credits exactly one destination. Destroyed transport scatters an actual finite recoverable amount; nothing is cloned by save/load, allied trades, salvage, prestige transitions or enemy harvest.

**Territorial control stages:** discovered → claimed → patrolled → garrisoned → administered. Different factions can contest title and resource access without combat occupying every cell. Defeating a garrison does not instantly restore local allegiance.

**Environmental recovery:** contamination cleanup, repair, salvage extraction, spore burning and replanting have material/crew costs and campaign-time duration. The land retains scars until an action actually repairs it. No passive real-calendar advancement when offline.

## 8. Thematic art atlas and VR design

The tabletop's charm comes from tiny soldiers on lethal terrain, not from huge animated shaders.

- **Eden:** green irrigation rows, brick houses, citrus-like groves, water tanks, farms next to bunker walls; water/health iconography embedded into the world.
- **Wasteland:** tilted highway slabs, derailed stock, ash earth, junk hills, vehicle carcasses and occasional opportunistic grass; burnt wrecks show salvageable piles.
- **Barren:** white salt crust, basalt, dry riverbanks, hard shadows and tiny sealed outposts; minimal cover and long lanes.
- **Anomaly:** doubled architecture, offset shadows, repeat seams, fractured mirrored floors, suspended projectile silhouettes; visual effects must correspond to authoritative anomalies.
- **Worldscar:** distant broken strata, cliff silhouettes, static hazy depth layers; no presumed intact planet under a glowing magic barrier.

**First tile/sprite budget:** prototype 16×16 and 24×24 terrain motifs, 32×32 unit silhouettes for magnified inspection, 4–8 facing directions as needed; validate actual headset readability rather than assuming a nominal pixel count proves it. Static scene data per environment family, changed-state overlays only, original assets and no ASCII stand-ins. Combat inputs remain usable via shell-normalized touchpad/head-gaze actions; text can scale to VR readability.

## 9. Generator, persistence and test gates

A sector is seeded from `campaign_seed + sector_id + ascension + world_generator_version`; a tactical battle derives its own reproducible instance and saved deltas. The 600 campaign sectors are distinct from the **4,096 proposed inspection-room nodes per ascension** in the existing mechanics charter. The latter live inside formations, items, bases and world layers, not as simultaneously active overworld maps.

**Deterministic generation pipeline:** theater graph → population/Eden placement → faction claims and conflict → resource network → roads and infrastructure → height/ground → ruins and cover → optional rift overlays → route/objective reachability audit → enemy placement → persistent sector descriptors.

**Hard acceptance tests once implemented:**
1. 600 unique IDs, 12 theaters with 50 each; 16 unique named T01 opening nodes.
2. Four environment concepts have genuinely different production, logistics and combat effects.
3. All faction/sector relationships distinguish patrol, population allegiance, claim and military control; exactly 17 primary faction IDs.
4. Civilians and convoys cannot be teleported; scarce inventories remain conserved across transfer, capture, loss and salvage.
5. At least 10,000 reproducible generated battle/map fixtures are audited for unwinnable start, inaccessible objective, missing extraction, impossible terrain and infinite hazards.
6. Each of the first five operations has victory/defeat persistence; diplomacy and repairs offer alternatives where authored.
7. Planetary fracture reads visually as an actual severing, while unexplained world physics remain explicitly unknown.
8. No starvation of command inputs, stereo state mismatch or over-budget VR rendering on Galaxy S9 sustained tests.
9. The complete tenfold world-prestige covenant remains testable without losing faction IDs, player history or terrain-save integrity.

**Release-status boundary:** this document defines intended environment content, not a produced atlas, battle map, simulation engine, implemented hazard, world generator, VR playable or APK.
