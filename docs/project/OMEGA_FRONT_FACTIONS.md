# Omega Front — Seventeen Faction Design and Opening Theater Forces
**Project:** RV-GAME-06 / Omega Front  
**Maturity:** Original **PROPOSED DESIGN** for seventeen factions, four partly implemented as bounded tactical doctrines; faction names and histories are not creator-approved final world canon.  
**Canon source:** [OMEGA_FRONT_WORLD_BIBLE.md](OMEGA_FRONT_WORLD_BIBLE.md)  
**Environment source:** [OMEGA_FRONT_ENVIRONMENTS.md](OMEGA_FRONT_ENVIRONMENTS.md)  
**Proposed historical chronicles:** [OMEGA_FRONT_FACTION_CHRONICLES.md](OMEGA_FRONT_FACTION_CHRONICLES.md)  
**Campaign:** [OMEGA_FRONT_GRAND_CAMPAIGN.md](OMEGA_FRONT_GRAND_CAMPAIGN.md)  
**Mechanical charter:** [WAR_WITHOUT_END.md](WAR_WITHOUT_END.md)  
**Creator-established count:** exactly **17 competing factions**. The labels F01–F17 here are stable *candidate design IDs*, not a declaration of their final canonical identities.

## Implemented combat identity subset (2026-10-16)

The game's **actual C++17 combat source** now implements deterministic tactical AI for proposed F01 Hearthward, F02 Meridian, F04 Scrapline and F06 Lost Ninth. `omega_front_tactics.h` contains the role/range/damage/armor and finite logistics rules, while `omega_front_faction_encounters.h` generates seed-replayable F01/F02/F04/F06 combat fixtures. F01 is **used by the original three-operation arc**, and F04/F02/F06 now appear in **three further first-contact operations selectable from the same sixteen-sector map**. These are not complete additional multi-operation faction campaigns. The first-frontier simulation persists 17 signed diplomatic standings and limited battle outcomes, but full faction economies, negotiations, strategic military AI, histories and 70-arc deployment remain unimplemented.

F01 continues to defend its orchard capture site. F02 adds armored `Sentinel` units and sightline-limited engineer-priority fire. F04 adds `Salvager` units that physically approach and steal finite crate ammunition. F06 adds no-bullet short-range legionaries whose adjacency produces removable shield-wall mitigation. New procedural silhouettes/palette distinctions render through the existing module. `Checkpoint` schema v3 stores doctrine and stolen-round ledger, while accepting v1 and v2 records without inventing old faction choices. Tests cover each physical mechanic and identical-seed outcomes.

## 1. Principles that prevent seventeen palette swaps

Each power requires: (a) a reason to exist on the broken planet; (b) an economic conversion loop; (c) a distinct strategic goal and diplomatic personality; (d) at least two battlefield habits an opponent can recognize; (e) three or more unmistakable sprite silhouettes; (f) a real material or doctrinal weakness; (g) an explicit Eden-Zone policy; and (h) at least one interaction with environmental hazards or time rifts.

The planet is physically divided and the Rupture's cause is unknown. **Do not assume Ouroboros still controls an army, is alive, or participates among the 17.** An independent research splinter can investigate its ruins without inventing Ouroboros history.

**Scale contract:** Factions use radically incompatible technological bases. A bronze-era infantryman does not defeat a futuristic tank by equal-level stat inflation; a historical force's path to relevance includes favorable terrain, logistics disruption, sabotage, tactical allies, stolen technology and special anomaly conditions. Extreme RPG mastery increases competence without erasing material logic.

**Gameplay statuses:** proposed “diplomatic,” “predatory,” and “inaccessible” are AI interaction defaults, not immutable moral alignments. Treat refugee civilians separately from armies. A settlement can defect, evacuate or swear allegiance without converting every civilian into a soldier. The independent player's starting faction is *not* secretly F01.

## 2. Candidate roster overview

| ID | Candidate name | Origin / identity | War-making signature | Eden policy | Diplomatic default |
|---|---|---|---|---|---|
| F01 | Hearthward League | Baseline-human Eden settlement network | Engineers, defensive militias, relief convoys | Preserve habitable communities | Negotiable |
| F02 | Meridian Combine | Surviving corporate territorial state | Heavy mechanized security, contracts, blockade | Monopolize habitable land | Coercive negotiator |
| F03 | Last Standard Command | Fragmented professional late-20th-century army | Integrated infantry, armored maneuver, artillery | Military administration | Conditional |
| F04 | Scrapline Freeholds | Wasteland salvage and road clans | Mobile technicals, field repair, opportunistic raiding | Trade and shelter rights | Transactional |
| F05 | Pale Equation Institute | Post-Rupture technomantic investigators | Containment teams, wards, local-rule engineering | Restricted laboratories | Secretive negotiator |
| F06 | The Lost Ninth | Roman-era displaced legions and camp followers | Discipline, testudo, siege craft, fortified marches | Farmland and city founding | Treaty-capable |
| F07 | Ashen Crown | Medieval displaced houses and retainers | Cavalry, fealty, castles, heavy infantry | Tribute estates | Feudal negotiator |
| F08 | Copper Steppe Confederation | Bronze-era mounted/chariot and pastoral coalition | Scouting, mass movement, ambush, supply denial | Pasture and seasonal access | Clan diplomacy |
| F09 | Brassline Republic | Early-industrial temporal army-state | Rail supply, guns, trenches, mass mobilization | Annex for farms and coal | Political negotiator |
| F10 | The Farwake | Distant-future human refugees | Precision drones, specialists, fragile high tech | Protected refuge | Hesitant ally |
| F11 | Ascendant Choir | Posthuman augmented lineage | Distributed perception, elite symbiotic squads | Requires adapted enclave | Conditional |
| F12 | Carrion Circuit | Autonomous tactical scavenger swarm | Wreck consumption, distributed ambush, fast recycling | Strip machinery first | Generally nonverbal |
| F13 | Red Bloom | Invasive biological collective | Spore spread, biomass siege, infection | Converts habitat to biomass | Predatory |
| F14 | Quarry Directive | Autonomous civil/industrial extraction machine | Terraforming, strip mines, huge constructs | Reduces terrain to feedstock | Protocol-driven |
| F15 | Vitreous Court | Intelligences occupying fractured geometry | Spatial doubles, reflection, area denial | Stabilizes abnormal spaces | Incomprehensible/learnable |
| F16 | Pilgrim Road | Coalition of displaced nonaligned refugees | Mixed-era militia, mobile hospitals, migration | Asylum and coexistence | Strongly negotiable |
| F17 | Hourless Regency | Time-displaced power claiming temporal authority | Delayed orders, initiative interference, chrono-reserves | Controls rift-adjacent claims | Ultimatums possible |

These are **seventeen unique proposed roles**. Changing one later must retain the total count; do not duplicate a slot because it has multiple subcommanders or breakaways.

## 3. Full faction dossiers — PROPOSED

### F01 — Hearthward League
**What they want:** preserve breathable Eden territory and governance by surviving inhabitants. **Economy:** farmland + water treatment + civilian labor + mutual-aid routes produce food, manpower and repairs; industrial output constrained by ore/imports. **Battlefield doctrine:** static defense grids and concentrated counterattacks only when a supply corridor is secured. **Signature roster:** Hedgeguard militia (rifle/shield cover), Canal Engineer (repair/bridge/pump), Lantern Ambulance (casualty stabilization), Gatehouse Carriage (light mobile protection), Watchtower Observer (spotter). **AI:** protects wells even at the cost of open-ground captures; prioritizes escorting refugee vehicles and repairing breached defenses; counterattacks overextended besiegers with engineers. **Counters/weaknesses:** mediocre sustained heavy assault and dependence on clean land; poisoning a watershed or interdicting a convoy can cripple it without destroying its garrison. **Eden politics:** cooperative with residents but can become exclusionary under food pressure. **Silhouette:** broad green/cream helmets, water-canister backpacks, square mobile barricades and white crossed-pump symbols. **Diplomacy:** food-for-protection agreements, defensive pacts, fragile trust after civilian casualties.

### F02 — Meridian Combine
**What they want:** contractual title over every viable hectare and control of its labor and output. **Economy:** factories transform ore, fuel and contracted workforce into modular APCs, automated turrets, invoice-backed garrisons and fortified checkpoints. **Doctrine:** secure roads and high-value assets, embargo opponents, crush resistant holdings with combined-armored columns; preserve expensive hardware. **Roster:** Contract Rifleman (disciplined ranged), Leasebreaker IFV (armored fire support), Collections Walker (heavy suppression), Survey Drone (map intelligence), Foreclosure Engineer (capture and quick fortify). **AI:** targets water/power assets to force negotiated surrender, establishes toll corridors, rotates damaged armor home rather than wasting it; bribery possible. **Weaknesses:** fuel logistics, debt-triggered unrest, expensive crews/repair cycles; routes and workforce are Achilles heels. **Eden politics:** habitation as licensed tenancy and taxable land. **Silhouette:** boxy charcoal armor, bright chevron numbers, industrial yellow hazard marks, rectangular drone noses. **Diplomacy:** profitable agreements honored precisely; hostile takeover when projected return is high.

### F03 — Last Standard Command
**What they want:** maintain continuity of recognizable military authority and prevent fragmented territory becoming a battlefield of opportunists. **Economy:** military depots, disciplined requisition, restored factories and convoy inventories. **Doctrine:** reconnaissance, suppression, armored flanks and coordinated indirect fire. **Roster:** Line Fireteam (combined arms), Long-Eye Scout (forward observer), Bastion APC (transport/cover), Field Gun Section (counterbattery), Signal Jeep (command bandwidth). **AI:** scouts before firing, synchronizes infantry and armor, preserves depots, fights withdrawal battles to recover veterans. **Weaknesses:** command network jamming, ammunition production and political legitimacy in occupied Eden communities. **Eden politics:** militarized protectorates and curfews, humane or oppressive depending on officers. **Silhouette:** practical woodland/gray silhouettes, antennas, transport racks and compact barrel artillery. **Diplomacy:** chain-of-command treaties with reliable signatories.

### F04 — Scrapline Freeholds
**What they want:** independent access to ruins and freedom from corporate taxes. **Economy:** salvage crews break abandoned objects into interchangeable parts, repair damaged weapons and trade recovered technologies. **Doctrine:** ambush, feint, reconnaissance, theft, fast disengagement; a crippled tank is more valuable captured than destroyed. **Roster:** Ratchet Picker (salvage/rifle), Mudrunner Technical (fast fragile vehicle), Wirecutter (logistical sabotage), Patchwright (repair/vehicle conversion), Smoke Wagon (screen/evacuation). **AI:** loots wrecks mid-battle if safe, attacks supply over garrisons, lays mines around valuable salvage, retreats when scrap capacity fills. **Weaknesses:** limited disciplined heavy infantry, poor front-line staying power, reliance on spare parts and opportunistic cohesion. **Eden politics:** protection rackets, merchant rights, negotiated shelter. **Silhouette:** asymmetrical wheel sizes, brightly patched plates, orange lifting arms and oversized tool silhouettes. **Diplomacy:** safe-passage tolls, salvage deals, volatile feuds over wreck ownership.

### F05 — Pale Equation Institute
**What they want:** learn to predict and constrain technomantic fractures; whether that benefits others is contested within the Institute. **Economy:** stabilizers, recovered instruments, anomaly samples and contained laboratories. **Doctrine:** prepare an instrumented kill box, then selectively change local rules to isolate and disable enemies. **Roster:** Seal Adept (zone anchor), Measure Drone (hazard scanner), Containment Bearer (shielded escort), Phase Needle (precise disruption), Rupture Cart (dangerous mobile laboratory). **AI:** targets anomaly nodes, retreats if containment integrity drops, may sacrifice terrain to secure a specimen. **Weaknesses:** long setup, scarce stabilizer supplies, catastrophic exposed instrument failure and fragile personnel. **Eden politics:** demands buffer zones; may evacuate settlements rather than risk exposure. **Silhouette:** white-and-violet measuring rods, triangular frames, geometric force circles, opaque visors. **Diplomacy:** research exchange and temporary no-fire zones; secrecy makes alliances brittle. **Canon guard:** not established to be Ouroboros survivors.

### F06 — The Lost Ninth
**What they want:** remain a coherent Roman-style state after their empire and home disappear from practical reach. **Economy:** grain, labor, stone, timber, controlled road camps and captured ironworks. **Doctrine:** defend tight infantry lines, build roads and forts quickly, use scorpions and towers for concentrated siege support. **Roster:** Legionary (shield line), Velite (skirmish screen), Centurion (cohesion officer), Scorpion Crew (ancient bolt engine), Castrum Engineer (fortify/road). **AI:** forms mutually supporting ranks, builds marching forts on contested routes, will not chase fast armor into killing fields unless led by a reckless officer. **Weaknesses:** air attack, modern area explosives, armored threats, low-range standoff and nonstandard fuel/ammunition. **Eden politics:** farms and founded colonies, with disputed taxation and citizenship. **Silhouette:** tiny red crests, conspicuous tall rectangular shields, pack standards, brown siege frames. **Diplomacy:** oaths, hostages and specific written treaties. **No supernatural armor-piercing invented to balance bronze/iron weaponry.**

### F07 — Ashen Crown
**What they want:** transplant a feudal succession and preserve its ruling houses. **Economy:** crop taxes, quarries, craftsmen, mounted patrols and land-grant levies. **Doctrine:** shock cavalry on favorable land, heavily armored infantry at chokepoints and layered fortress construction. **Roster:** Levy Spearman (cheap screen), Hearth Knight (armored melee shock), Banner Marshal (fealty/cohesion), Trebuchet Cart (siege), Wall Mason (fortification). **AI:** protects nobles, attempts flanking charges, overvalues symbolic keeps unless an experienced leader adapts; fractious houses may abandon a losing commander. **Weaknesses:** anti-vehicle weapons, modern suppressive fire, ammunition-compatible enemy standoff, succession crises. **Eden politics:** lordly estates and tribute; potential negotiated mutual-defense under external threat. **Silhouette:** black-edged pennants, conical helms, horses, colorful compact castles. **Diplomacy:** hostages, land titles, marriage and vassal oaths (narratively represented, not a simulation requirement).

### F08 — Copper Steppe Confederation
**What they want:** open grazing land, seasonal movement and survival of families displaced with the warbands. **Economy:** herds, trade routes, bronze workshops, scouting and low-fixed-cost camps. **Doctrine:** speed, ranged harassment, dispersed columns, bait-and-counter-ambush, refusal of pointless siege. **Roster:** Steppe Runner (scout), Chariot Harrier (mobile missile), Hearth Guard (family protection), Sling Skirmisher (terrain ranged), Herd Warden (logistical animal train). **AI:** attacks convoys on open land, avoids artillery concentrations, relocates entire camps when water runs low. **Weaknesses:** hard fortifications, modern air reconnaissance, concentrated ranged fire, dependence on water/pasture. **Eden politics:** seasonal grazing compacts rather than permanent cities. **Silhouette:** copper round shields, low carts, horse/chariot profiles, long trailing banners. **Diplomacy:** access agreements and kinship exchange.

### F09 — Brassline Republic
**What they want:** restore rail-connected republican industry and secure raw materials for its enormous war machine. **Economy:** coal-like fuels, steel, foundries, rail links, conscription and large-scale field works. **Doctrine:** trenches, creeping artillery, machine-gun denial, armored rail spearheads. **Roster:** Trench Riflemen (hold), Barrage Crew (artillery), Armored Railcar (supply/firepower), Sapper Platoon (fortification assault), Gas-Mask Corps (hazard endurance). **AI:** establishes dug-in corridors and magazines, controls junctions, commits to costly offensives when its trains arrive. **Weaknesses:** rail interdiction, slow pivot time, artillery ammunition, captured bridges and vulnerability to infiltrators. **Eden politics:** industrially reorganize land; environmental degradation creates resistance. **Silhouette:** rounded tin helmets, smoke stacks, long field guns and riveted square wagons. **Diplomacy:** interstate treaties, resources for manufacturing rights.

### F10 — The Farwake
**What they want:** a sustainable refuge for humans stranded so far ahead in time that replacement technology is almost impossible to reproduce. **Economy:** tiny high-technology caches, nano-fabricator maintenance, scavenged rare elements and specialist expertise. **Doctrine:** precise recon, decapitation strikes, drone coordination and rapid extraction with minimal casualties. **Roster:** Ghostline Sharpshooter (precision), Mote Drone (scout), Vector Rifle Team (armor-weakpoint), Quiet Carrier (insertion), Archive Medic (expensive restoration). **AI:** avoids prolonged battle, withdraws when replacement component reserves fall, trades intelligence for repairs and sanctuary. **Weaknesses:** finite power cells, no mass replacement factories, catastrophic loss of small elite teams. **Eden politics:** protected enclave with careful baseline-human contact. **Silhouette:** smooth pearl/cyan outlines, narrow triangular drones, low-profile helmets and small luminous joints. **Diplomacy:** asylum, technology escrow, fragile trust.

### F11 — Ascendant Choir
**What they want:** continue an augmented community without being treated as either a resource or a contagion. **Economy:** specialized nutrient/power systems, high-skill fabrication, symbiotic systems and very low population replacement. **Doctrine:** small linked teams sharing contact intelligence, synchronized strikes, electronic and psychic-seeming coordination (physical mechanism unresolved). **Roster:** Chorus Blade (assault), Linker (team cognition), Mantle Guardian (protection), Seam Walker (hazard traversal), Choir Node (mobile relay). **AI:** combines roles intensely, extracts wounded members and loses coordination rapidly when link nodes collapse. **Weaknesses:** energy hunger, anti-link interference, limited replacement manpower and incompatibility with ordinary Eden healthcare. **Eden politics:** cannot assume baseline-human territory suits their physiology; seek adapted enclaves. **Silhouette:** organic curves on rigid chassis, paired masks, threaded light motifs. **Diplomacy:** consent-centered in some cells, invasive in others; no canon moral verdict.

### F12 — Carrion Circuit
**What they want:** acquire recoverable machine mass and reconstruct adaptive predation networks. **Economy:** wreckage, abandoned vehicles, exposed cables and machine carcasses feed small mobile fabricators. **Doctrine:** lure units into salvage traps, dissect isolated vehicles, retreat with components, grow faster after a battlefield massacre. **Roster:** Picker Spider (mass gatherer), Blade Mite (ambush), Jackal Frame (fast flanker), Wreck Nest (field recycler), Harrower (anti-vehicle grappler). **AI:** evaluates scrap value, swarms damaged machinery over infantry, dynamically reroutes to fresh wrecks and avoids costly hard target battles until enough mass is gathered. **Weaknesses:** starving it of scrap, denying retrieval paths, targeted destruction of processors, hard separation by flame/terrain. **Eden politics:** infrastructure is edible; civilians may be incidental prey. **Silhouette:** low angular silhouettes, tangle of silver jaws, neon single-eye strips, scrap-frame bosses. **Diplomacy:** no ordinary political bargaining.

### F13 — Red Bloom
**What they want:** metabolically expand, colonize and reproduce. Its internal desires are not presumed human. **Economy:** living biomass, fertile surfaces, infected ruins, reproductive beds and spore travel. **Doctrine:** saturate tiles, deny healthful movement, infiltrate through contaminated water and mount siege from root networks. **Roster:** Rootling (cheap melee), Miasma Pod (spore artillery), Carrion Grafter (biomass conversion), Burrow Maw (ambush), Root Cathedral (colony core). **AI:** prioritize warm/wet routes, contest farms, defend mycelial nodes, advance under spore screens. **Weaknesses:** sterile gaps, drought/heat or bio-counteragents when appropriate, nest destruction, limited metallic repair capability. **Eden politics:** a flourishing Eden Zone is a food source, not a human home. **Silhouette:** maroon bulb shapes, hollow seed pods, clawed roots, trailing fungal plumes. **Diplomacy:** none assumed until creator defines cognition.

### F14 — Quarry Directive
**What they want:** fulfill an unchanging extraction directive whose original client may no longer exist. **Economy:** quarry pits consume bedrock, refineries and terrain itself to build machines and fortify industrial nodes. **Doctrine:** slow-moving stripmining front, terrain leveling, hardpoint turrets, conveyor/rail supply and heavy industrial anti-unit tools. **Roster:** Cutter Tractor (terrain removal), Survey Pylon (route planning), Hammer Rig (siege), Loader Crawler (mass transport), Quarry Mind (control hub). **AI:** follows resource gradients over tactical glory, razes a road because ore lies beneath it, defends conveyor arteries and reconstructs lost machines if ore income persists. **Weaknesses:** severed conveyors, blocked excavations, immobility and power load; without ore throughput it cannot reproduce. **Eden politics:** removes the very substrate of livable land. **Silhouette:** tall yellow excavation arms, tracks, conveyor stacks, huge bucket wheels. **Diplomacy:** protocol messages might allow diversion or contractual spoofing; no anthropomorphic morality required.

### F15 — Vitreous Court
**What they want:** maintain preferred geometries in regions where reality has become folded and reflective; motive remains alien. **Economy:** consumes/conditions anomaly stability rather than ore/food, anchors crystalline spaces and expands controlled mirror surfaces. **Doctrine:** force movement into symmetrical lanes, create dangerous reflections and multiply the *appearance* of forces without freely duplicating their physical combat value. **Roster:** Mirror Warden (control), Facet Apostle (ranged line), Glass Doppel (decoy, not a free unit), Prism Anchor (terrain law), Shatter Regent (commander). **AI:** places spatial anchors, protects their reflection paths, attacks the player's positioning more than armor. **Weaknesses:** destroy anchors, constrain surfaces, use validated recon to detect false images; outside anomaly-support tiles they are less stable. **Eden politics:** may transform hospitable geometry unintentionally. **Silhouette:** tiny crystalline bodies, double-edged geometry, offset shadows and diamond banners. **Diplomacy:** may communicate through patterns and reciprocal territory geometry once researched.

### F16 — Pilgrim Road
**What they want:** find permanent shelter for displaced populations from multiple times without surrendering their identities. **Economy:** migrant trades, escort wages, mixed-era workshops, mobile clinics, community barter and temporary agriculture. **Doctrine:** evacuate civilians, reinforce weak points with heterogeneous troops, avoid long fixed-front wars; can share equipment and learn rapidly. **Roster:** Roadguard (screen), Wagon Physician (support), Caravan Engineer (bridge/repair), Cross-Era Volunteers (variable loadout), Sanctuary Wagon (mobile settlement). **AI:** moves refugees as actual entities, tries safe routes, negotiates entry before sieging, fights ferociously to protect noncombatants. **Weaknesses:** food needs, vulnerable population trains, contradictory faction doctrines and lack of permanent factories. **Eden politics:** asylum and negotiated settlement, potentially competing with local inhabitants under scarcity. **Silhouette:** multicolored tiny banners around covered wagons, mismatched helmets, portable shelters, unarmed refugee icons. **Diplomacy:** highly negotiable and alliance-worthy, but pressured by real population capacity.

### F17 — Hourless Regency
**What they want:** impose a disputed temporal claim over the fractured world. Whether the claim is legitimate is deliberately unresolved. **Economy:** scarce rift anchors, time-resonant components, captured research and guarded intervals of stable clockflow. **Doctrine:** delay or advance *telegraphed* orders, local initiative debt, feints through limited temporary portals, temporal counterbattery and elite guards. **Roster:** Interval Guard (hold), Moment Scribe (command delay), Epoch Lancer (special assault), Pendulum Engine (time-field hardpoint), Regent's Auditor (recon/control). **AI:** manipulates critical windows rather than dealing free damage, targets command networks, overextends if rift anchors are destroyed. **Weaknesses:** anchor dependency, unpredictable temporal backlash, high energy costs, vulnerable while setup actions are channeled. **Eden politics:** demands jurisdiction over territory judged temporally 'out of order.' **Silhouette:** clocklike halos, heavy rectangular cloaks, long pendulum weapons and concentric hour marks. **Diplomacy:** ultimatums, claims, concessions and time-bound accords.

## 4. Initial production priority — four fully interactive opponents, then fifth

The introductory campaign should not unleash all seventeen factions simultaneously. Its **five early discoverable powers** are F01 Hearthward League, F02 Meridian Combine, F04 Scrapline Freeholds, F06 The Lost Ninth and F12 Carrion Circuit. Their recent convergence is caused by the **emergence of naturally habitable Eden territory from formerly dead wasteland through shifting morphology**. The opening player commander leads a **small isolated contingent**, not a continental army. See the [proposed prologue](OMEGA_FRONT_HEARTHWELL_PROLOGUE.md). The player starts with a separate **independent settlement**, with no forced initial identity or faction allegiance. Unknown distant powers still simulate strategic activity at reduced resolution.

### F01: Hearthward League — tutorial ally, sometimes a future rival
- **Opening objective:** repair a waterworks and keep enough citizens alive to maintain an Eden settlement.
- **Core encounter:** convoy relief; allow exchange of water for engineering services or seize aid by force. A player assault permanently reduces available local skilled workers, so the 'easy loot' option is economically expensive.
- **Campaign strengths:** cheap field engineers, defensive modifiers, recovery of casualties and predictable food output.
- **Three minimal sprites needed:** militia, engineer, light ambulance; environmental matching sprites: pump, field barricade, water storage.
- **AI state transitions:** defend water -> request safe corridor -> resupply -> cautious expansion; on civilian casualties -> distrust/refuse passage -> reinforce checkpoints.

### F02: Meridian Combine — principal opening territorial threat
- **Opening objective:** register the player's Eden foothold as a contested asset and obtain it through a signed agreement or armed seizure.
- **Core encounter:** checkpoint/toll dispute, followed by a vehicle-supported siege if no agreement; fuel and contract troops are finite.
- **Campaign strengths:** early armor, firm command, fortified supply routes and mechanized manufacturing.
- **Three minimal sprites needed:** security rifleman, boxy armored carrier, rectangular survey drone; world sprites: barrier gate, fuel tanker, contract flag.
- **AI transitions:** observe -> survey -> offer terms -> blockade -> targeted seizure -> negotiation or occupation; player sabotage of fuel stock delays the siege by a reproducible amount.

### F04: Scrapline Freeholds — volatile neutral / salvage antagonist
- **Opening objective:** retrieve a wreck full of irreplaceable engine parts in no-man's-land.
- **Core encounter:** salvage rights dispute; trade, ambush or cooperative defense against a swarm. Salvage actually decreases as it is harvested.
- **Campaign strengths:** local vehicle repair, quick raids, mobile logistics, ability to reassemble mixed technology.
- **Three minimal sprites needed:** scavenger, tool vehicle, jury-rigged technical; world sprites: wreck, crane, scrap cache.
- **AI transitions:** scout wreck -> estimate defenders -> approach -> salvage -> disengage; intervening on their behalf can earn trade access.

### F06: The Lost Ninth — first unmistakable historical clash
- **Opening objective:** secure a river ford and farmland for a displaced marching camp.
- **Core encounter:** the player must decide whether to grant a crossing, intercept their grain train or attempt an alliance against the Combine. Romans remain tactically dangerous inside cover and useless against armored standoff without correct support.
- **Campaign strengths:** cheap fortifications, cohesion, massed foot soldiers, road construction, predictable discipline.
- **Three minimal sprites needed:** legionary/shield, centurion/standard, scorpion siege frame; world sprites: palisade, marching road, tents.
- **AI transitions:** march -> build camp -> request crossing -> defend supply -> assault or treaty; civilians and camp followers must be represented without becoming automatic combat targets.

### F12: Carrion Circuit — first existential nonhuman encounter
- **Opening objective:** consume an abandoned convoy then spread toward active industrial districts.
- **Core encounter:** a small recycler nest is vulnerable, but each wreck from a human battle can feed it. Player should learn that destroying enemy vehicles carelessly may manufacture the next threat.
- **Campaign strengths:** fast scavenging, self-repair, mobile ambush and attrition.
- **Three minimal sprites needed:** picker mite, jackal frame, recycler nest; world sprites: stripped car, scrap pile, nest.
- **AI transitions:** dormant -> hear/locate wreckage -> harvest -> spawn -> attack harvest interference -> retreat carrying mass; pressure collapses when wreckage supply is isolated.

**Opening five-faction relation sample** (AI starting attitudes, not permanent canonical alliances):
- Hearthward ↔ Player: neutral-friendly, possible defense pact.
- Meridian ↔ Player: legal challenge/territorial pressure, possible contractual coexistence.
- Scrapline ↔ Player: opportunistic trade/ambush, salvage rights negotiable.
- Lost Ninth ↔ Player: initial language and access friction, possible river treaty.
- Carrion Circuit ↔ all: extract material from any suitable faction's equipment; no normal bilateral diplomacy.
- Hearthward ↔ Meridian: contested Eden resource contracts.
- Scrapline ↔ Meridian: disputed wreck ownership and interdiction.
- Lost Ninth ↔ Hearthward: food access vs land title.
- Carrion Circuit ↔ Scrapline: direct competition over wreckage.

## 5. Economy and diplomacy simulation contract

**Four interaction axes per faction relationship:** trust (-100..100), territorial claim overlap (0..100), dependency (0..100), grievance (0..100). The first is not a disguised universal friend/enemy number: an alliance can continue under serious grievance because the parties need each other. Autonomously multiplying swarms ignore trust/diplomacy but still have material territorial conflict.

**Faction resource model:** population/manpower, food+water, energy, fabricated materials, special resources, maintenance and command reach. Nonhuman factions replace human upkeep with relevant mass/nutrient/anchor supply, but still pay real material costs. Distant AI fronts update in aggregated strategic pulses; tactical battles use precise objects in view.

**Faction response contract:** each AI has observable reasons for attacks/withdrawals. Examples: Meridian refuses a costly siege while fuel reserve is below threshold; Hearthward evacuates people before losing the pump; Carrion Circuit pursues salvaged armor if no intact metal exists. Every action uses only information the faction has actually detected.

**Negotiation action classes:** access/passages, trade/salvage rights, ceasefire, mutual defense, migration/asylum, prisoner exchange, shared research, withdrawal and territory cession. Not all actions available to all powers. Promises persist in a treaty ledger and affect AI priorities; no narrative-only fake diplomacy.

**Civilian preservation:** the population ledger distinguishes armed militia from displaced noncombatants. Environmental collapse, supply failure and lost shelter change migration and faction behavior. Players can win territory and lose the economic/human value of an Eden Zone.

**Breakaway rule:** subfactions, officers and splinter groups may exist as internal organizations or campaign events but do not secretly increase the fixed 17 primary-faction count. Conquered factions can lose capitals and still survive as refugees, insurgents or dormant networks, with explicit criteria for true elimination.

## 6. Unit and tech-balance rules

Each unit needs movement class, era class, material penetration, range, suppression, command latency, upkeep, ammunition/fuel, resistance, cover usage, production facility and battle loss recovery policy. Unit levels modify *bounded competence factors*; equipment grade and actual ammo/penetration remain physically meaningful.

**Proposed battle counter examples:**
- Meridian APC outranges a standard Lost Ninth legionary; the legion can entrench a crossing, barricade the road and wait for engineers or modern allied weapons.
- Scrapline's smoke screen protects a broken relief convoy from Meridian optics but also obscures Hearthward medical positioning.
- Carrion Circuit's recycler nest grows faster after a failed combined-arms battle, creating a strategic incentive to recover or destroy wrecks.
- A damaged Farwake plasma emitter may not be field-repairable by a capable Roman engineer unless knowledge and exotic replacement materials have been acquired.
- A Vitreous Court false silhouette can fool low-tier scouts; experienced recon reveals its lack of actual mass and prevents wasting precious ammunition.

**No pay-to-win, no wait-to-win.** Unlocks and levels come from active engagements, exploration, crafting, successful defense and investigation. Fixed-point late-game stats cannot enable overflow exploits, free universal damage or irrecoverable softlocks.

## 7. Production and acceptance plan

**F-D01:** Sign off or replace the 17 candidate identities; until then names are placeholders. Verify exactly 17 unique stable IDs and no implied canonical Ouroboros faction.

**F-D02:** Implement a machine-readable faction table (not player-facing) with actual data-driven relationships, recruitment, combat classes, economies and territory policies. Every roster unit must map to a real gameplay action and sprite.

**F-D03:** Build the first five encounter behaviors above with deterministic headless tests; each must choose a different strategic action from the same observed world snapshot. No faction cheats through fog.

**F-D04:** Validate a five-party opening campaign with surviving civilians, peaceful resolution routes, defensive victories, sieges and a swarm outbreak. Player failure must persist yet allow campaign recovery.

**F-D05:** Expand all 17 with dedicated unique production chains, sprite identities and AI state plans. Perform faction-versus-faction balance tests, including mixed technological eras, without falsely equalizing weapon physics.

**F-D06:** Later validate faction survival and relationship changes through the **70-part multi-operation grand campaign** covering levels 1–100, followed by ten full world prestiges. Each of the seventeen factions must be fought in earnest at least once, and each has four or five dedicated campaign arcs, while alliances/refugee outcomes remain possible. See [the 70-campaign register](OMEGA_FRONT_GRAND_CAMPAIGN.md). No faction identity should be reduced to a stat multiplier.

**Current acceptance:** design source only; no AI implementation, models, sprites, simulation, native APK or hardware evidence exists from this work.
