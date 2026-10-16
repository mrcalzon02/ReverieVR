# Gumball Armada — Dependency-Gated Production Plan

**Type:** authoritative *planning* document, not proof of implementation.  
**Target:** ReverieVR single Android APK; Galaxy S9, Daydream View and original one-handed 3DoF controller.  
**Baseline:** remote main 914c84bcfdade356ad45cc571f4cbab889393e06 (2026-10-15).  
**Present reality:** Development-only forward-chart / wave combat / simple upgrades / low-cost GLES2 **source draft**; NDK build, APK and reference-device playthrough unverified.

## 1. Production contract

The current 4096-address chart is not a revisitable open-world galaxy. Four numerical upgrades are not a ship block assembler. Visual combo slogans are not recorded voice. A native library entry is not a device-tested game. Stage completion requires actual behavior, actual packaged assets, the associated evidence and honest acceptance state.

State ladder: **planned → source implemented → statically verified → packaged verified → device accepted**. Content has extra states: authored → versioned/rights recorded → packaged → bound and observed in real gameplay. Do not conflate these states.

Every microstep follows Intent → Execute → Observe → Verify → Claim; small single-purpose commits to main, remote readback, bounded connector retries, no force push and no newly introduced GitHub Actions. Preserve Shell Menu/Home/recenter/gaze recovery, offline/phone-first behavior, user settings and current Android update path. No dead buttons, disconnected art assets, fabricated audio or sidecar desktop dependencies.

### Definition of a playable game increment

Launch from the actual ReverieVR native game shelf → begin mission → control the ship without an unnatural held-click movement requirement → encounter real danger → win OR lose under honest rules → receive real result/reward → modify persistent player state → safely return and replay. All content advertised for the increment is used, visible or audible. Input, headset stereo, save and performance gates apply to every runtime slice.

### Milestones and stage ordering

| Milestone | Production stages | User-playable scope | Gate |
|---|---|---|---|
| M0 Foundation verified | S00 | Buildable registered source with deterministic tests | Local/NDK/APK/native exports, errors recorded |
| M1 First arcade run | S01–S02 | Full sortie with combat, reward, retry and distinct waves | Physical S9, controller, stereo and shell recovery |
| M2 Combat + first player-built ship | S03–S07 | Weapons, defenses, actives, themed audio/visuals, actual block assembly | Functional component/ship saves and device proof |
| M3 Regional sandbox | S08–S11 | Earn/buy/sell/steal parts, missions, visit/revisit navigable nodes | Persistent economy, reachability and replay tests |
| M4 First complete regional campaign | S12 | Sally Scrunch, unique fleet and multiphase flagship | From-new-game chapter playthrough |
| M5 Four-general campaign | S13–S15 | Cringe, Pucker, Sauer, escalating doctrine and story | Boss-specific tests and campaign progression |
| M6 Full finished title | S16–S18 | King reveal, ending, complete assets/VO/music, tuning and release | Full Galaxy S9 cold-start-to-credits test and signed APK |

Asset production and test harnesses start at M0/M1 and grow at *every* milestone; S17 is a final completion pass, not permission to postpone all artwork and audio.

## 2. Staged implementation with independently completable microsteps

### S00 — Establish trustworthy native baseline (dependency: current main)

- S00.1 Reconcile authoritative source, existing Android.mk/host/module/Java controller admission paths, current tests, project docs, head SHA, supported ABIs and actual build toolchain.
- S00.2 Exercise C++17 game regression under Clang/GCC and ASan/UBSan; include seeded replay, boss escape, non-fire steering, invalid enum, save corruption, large delta, enemy escaping, cash limits and stale-shield edge cases.
- S00.3 Gradle/NDK build, native shared-object export/ABI verifier, packaged game asset/library manifest, doc-sync check and test-install candidate. Do not silently pass a missing build.
- S00.4 Inspect game launch, GL create/draw/release, both eyes, Android pause/resume, controlled exit and module-private campaign.v1 save read/write.
- S00.5 Record bugs, crashes, frame warnings and hardware-blocked claims in acceptance ledger. Fix authoritative source rather than disabling checks.
- **Exit:** exact tree compiles and packages with observable native module; physical tests remain explicitly pending until done.

### S01 — One genuinely complete arcade sortie (dependency: S00)

- S01.1 Define seated stereo cockpit/tunnel: player looks naturally around, Daydream pad steers ship up/down/left/right independently, click held fires; head movement is NOT forced ship movement.
- S01.2 Make 3–5-minute loop: brief → launch → warm-up attacks → dangerous formations → miniboss → victory/failure → tally → earn Gumball Cash → chart/garage → replay.
- S01.3 Add hull/shields, readable hitboxes, telegraphed bullets, hit invulnerability feedback, damage impact, compact HUD, objective tracking, optional aim assistance, clear death/recovery.
- S01.4 Bind actual original assets: 1 hero craft, 3 enemies, 1 miniboss, readable candy corridor/skybox, six VFX, basic UI icons plus working launch/shoot/hit/explosion/win/lose sounds.
- S01.5 Protect shell Home/Menu/recenter and controller disconnect; prove pad moves without clicking, while click-fire does not disable pad steering; keep gaze fallback for shell recovery, not fictitious gaze-flight analog control.
- **Exit:** on S9/Daydream, cold start → complete one victory and one loss, cash/save persists and a second run visibly differs; both eye views are correct.

### S02 — Arcade combat depth and encounter director (S01)

- S02.1 Separate deterministic world update (once per frame) from per-eye GLES drawing. Define enemy/projectile fixed pools, collision, wave events, difficulty classes, failure/contribution rules.
- S02.2 Add roles: skirmisher, interceptor, shield escort, bomber, mine layer, turret barge, heavy gunship and elite; each has distinctive behavior, silhouette, drops, attack tell and counter.
- S02.3 Wave grammar: column, fan, pincer, cross-fire, spiral, rear pressure, siege formation and escort group. Define 8 tested authored patterns and 2 hazards, not copied waves with higher HP.
- S02.4 Improve combo system: kill-window, streak loss, multikill reward, boss bonus, no cash duplication; visual rank labels and appropriately throttled announcer event requests.
- S02.5 Telemetry for damage taken/dealt, battle duration, enemy escape, draw/active entity counts; desktop seeded combat stress tests before handset retuning.
- **Exit:** 6+ actually distinct enemy archetypes and 8 patterns with repeatable input tests; arcade quality does not depend on stat inflation.

### S03 — Weapon catalogue, geometry and firing systems (S02)

- S03.1 Build data schema for weapon parts: mounting footprint, arc, tier, mass, price, power, heat, ammo, fire mode, projectile ID, geometry/atlas ID, audio cue, restrictions and quality.
- S03.2 First verified loadouts: sugar-pea repeater, rock-candy cannon and syrup beam; expand to licorice lance, popping burst mortar, gummy torpedoes, peppermint scattergun, fizz flak, caramel railgun, prism shield-stripper and elite variants.
- S03.3 Implement mechanical differences: rapid stream, splash, lob, pierce, sustained beam, seek, spread, flak intercept, charge, shield stripping; actual targeting and collision, not alternate text labels.
- S03.4 Add fire arcs, reload/capacitor behavior, overheat, ammo/magazine, directional mounts and muzzle origin from a visible component socket.
- S03.5 Test combat damage class versus moving targets, point-defense, armor, shield, beam ticks, chain reward, simultaneous mounts and invalid configs.
- **Exit:** at least three equally viable, visibly distinct, tested combat builds with genuinely distinct timing/damage and actual guns/sounds/VFX.

### S04 — Defenses, utilities and special abilities (S03)

- S04.1 Equipable nougat plating, waffle ablative armor, bubble shields, sourproof coating, crystal reflectors, intercept turrets and armored nacelles, each with meaningful damage/recharge mechanics.
- S04.2 Working abilities: gummy dash, decoy candy, bubble burst, taffy tractor, sugar-overdrive, mint EMP, temporary barrier and emergency icing repair. Introduce 3 then grow.
- S04.3 System cost/counters: cooldown, energy/heat consumption, equip slots, hazard vulnerabilities, boss immunities with telegraphed counterplay and failure feedback.
- S04.4 One-handed activation UX: tested time-slow radial ability selector or remappable supported action; original Menu/Home remain shell owned, no fictional Daydream extra buttons, volume remains volume by default.
- S04.5 Damage/repair connects to real components (broken turret/engine/shield node) rather than cosmetic damage indicators.
- **Exit:** three real actives and at least three defenses, each change outcome on same seeded combat scenario; no inaccessible VR actions.

### S05 — First full asset + audio integration pipeline (parallel S03/S04, converge before M2)

- S05.1 Approve style guide for Royal gummy structures, angular Sour hulls, candy material readability, cockpit scale, boss size, atlas look, HUD typography, palette and VFX saturation.
- S05.2 Asset manifest with stable ID, authored source, generated/runtime file, dimensions, SHA, provenance/license, atlas cell, shader/material usage and proof screenshot. Originals only; no ripped Kingdom Hearts/Candy Crush source assets.
- S05.3 Package actual hero, enemy, weapons, defense, item icons, 2 lane-space scenes, 5 impact/explosion sheets and accessible UI with real atlas/binding tests. No release-path ASCII art or temporary anonymous colored boxes for content promised complete.
- S05.4 Audio architecture: native-game events → platform mixer/packaged voice/SFX bank with pause/resume, priorities, gain, cooldown and captions. Existing generic host cue set cannot magically play arbitrary announcer recordings.
- S05.5 Produce initial original deep-voiced bank, approximately 8–12 lines, actual runtime triggers (Sugar Rush, Spectacular, Sour Annihilation, victory etc.), proper ducking/captioning; verify listener hears each event on phone.
- **Exit:** one hero combat loop visibly uses the authored atlases and audibly uses SFX/VO; no manifest misses and sound not just phantom events.

### S06 — Editable modular candy shipyard (S03–S05)

- S06.1 Define bounded 2D/2.5D snapped ship build-grid: frame/hull/core, footprints, sockets, rotatable parts, structural connectivity, mount arcs. A low-poly 3D ship is assembled from that bounded grid.
- S06.2 Starter block library: candy cockpit, licorice frame, wafer wing, nougat plate, marshmallow buoyancy node, fizz thruster, battery, shield node, cannon mount and cargo pod. Each is an actual working item with own graphics/data.
- S06.3 Accessible garage: scrollable category inventory, preview/pick/rotate/attach/remove/undo, clear legal/illegal placement feedback, weight/power summary and no dead edit actions.
- S06.4 Save named blueprints, installed-instance ownership, variant paints and active ship, restoring accurately across relaunch. Migrate early four numerical upgrades non-destructively.
- S06.5 Reject disconnected core, impossible reactor load, overlapping blocks, blocked mount arcs and blocked engines. Test 1000s of generated configs for save/placement invariants.
- **Exit:** two player-built visibly different legal ships fly and behave differently in identical combat; restart reloads the exact ship, including inventory.

### S07 — Ship integrity, power, heat, thrust and damage (S06)

- S07.1 Structural graph propagation, localized component hits, disabled/destroyed subassemblies and bounded deterministic detachable debris.
- S07.2 Power priority/reserves: engines, shields, weapons and utility draw from finite supply; an overloaded ship visibly loses functions according to declared rule.
- S07.3 Mass/engine layout/drag modifies steering response; engine damage affects performance but NEVER produces forced uncomfortable head-camera roll.
- S07.4 Heat generation/cooling adds repeatable fire tradeoffs, warning, shutdown and recovery. Ensure ship is not softlocked after component destruction.
- S07.5 Hull classes: skiff, interceptor, salvage tug, escort and gunship; exact balancing numbers determined after measured playtests, not arbitrary upgrades.
- **Exit:** same parts installed in alternate legal configurations measurably change steering, damage, range, cooling and survival; save/restore roundtrip proven.

### S08 — Cash, inventory, salvage, repairs, vendors and theft (S06/S07)

- S08.1 Economy ledger: Gumball Cash sources from runs, combo, contracts, salvage; sinks from repairs, components, consumables, research, ship licenses and insurance. Explicit no-free-credits and recovery policies.
- S08.2 Persistent component instances have unique IDs, owner, quality, durability, rarity, stack constraints, installed slot, lawful/stolen status, source and price; prevent dupes/negative quantities.
- S08.3 Recover floating salvaged parts with tractor/cargo mechanics and post-run salvage claim, damage/repair/breakdown. Theft only through risky actual encounter/boarding and faction consequence.
- S08.4 Shop classes: Royal certified, neutral scrapyard, Sour black market, mobile trader and faction specialist; deterministic stock/price rules and no exploit reset by reentering shop.
- S08.5 Bankruptcy recovery, starter-safety mission and loaner ship: never block critical progression behind unbuyable parts or force full save restart after bad trade.
- **Exit:** run mission → salvage weapon → install → sell/trade → repair → buy defensive block → launch; all balances and inventory survive every reload and no dupes.

### S09 — Commander skills, research, objective variety (S08)

- S09.1 Separate rank/XP from Gumball Cash and component power; commander talents for precision, evasion, trade, salvage, defense, leadership, combos.
- S09.2 Research/unlock grid ties meaningful technology to faction story, gameplay discovery, rare drops and contracts; never all unlocked by arbitrary cash-only clicks.
- S09.3 Objective types: lane clear, escort, courier intercept, station defend, fortress siege, salvage recovery, disable/capture, survive ambush, hunt elite and destroy artillery.
- S09.4 Mission fail states, bonus thresholds, time, damage and reputation consequences; ship categories must have reasons to exist.
- S09.5 Balance enemy threat and reward without level-scaling that invalidates upgraded ships; independent seed test against early and late loadouts.
- **Exit:** minimum 6 genuinely different objective types, two viable commander builds and a coherent currency/XP progression without grind softlocks.

### S10 — True semi-open star-lane graph, not 4096 sequential runs (S08/S09)

- S10.1 Evolve forward address counter into node/edge graph with safe return travel, region transitions, unlocked shortcuts, warp gates, fuel/cost rules and story-critical path.
- S10.2 Discoverability: fog of war, revisitable stations, route bookmarks, chart filtering, hidden passages and mission markers; compact VR readable navigation.
- S10.3 Deterministic seed selects hazard/encounter/drops, while explicit persistent state records visited, cleared, reclaimed, corrupted, liberated and commercially changed nodes.
- S10.4 Classes: neutral ports, scavenger bazaars, syrup storms, asteroid confections, shattered bakeries, crystalline nebulae, citric seas, Sour forts and secret convoy vaults.
- S10.5 4096-address scale must be traversable logically; generate **small bounded scenes on demand** instead of rendering thousands of levels at once.
- S10.6 Whole-graph audit for reachability, unique addresses, two-way access where declared, seeded replay, valid exits, no story lockout and non-destructive save migration.
- **Exit:** player visits port A, explores branch B, returns to A, buys new part, reaches different branch C, and finds a persistent consequence in the world.

### S11 — Region ecology, procedural encounters and navigation assets (S10)

- S11.1 Four coherent enemy-controlled region environments with unique motifs, celestial skies, routes, hazards, ship silhouette language and audible ambience.
- S11.2 Encounter composer mixes authored tactical grammar, location, enemy doctrine, risk, loot and faction flags; dozens of variations must **behave** differently, not just recolor.
- S11.3 Dynamic content: rescue beacon, wreck graveyard, smugglers, patrol blockade, construction yard, pirate trap, Royal relief convoy, secret stash and repair station.
- S11.4 Real fleet roles, visible gun points and weak points: drone, skiff, fighter, escort, cutter, heavy gunship, cruiser, carrier and fortress/dreadnought.
- S11.5 Bound active entities, sky geometry, atlas memory, streaming and audio concurrent voices to phone capabilities; run seeded worst-case encounter stress.
- **Exit:** three replays show distinct objectives, region visuals and loot; revisited authored locations remain logically stable and fit handset budget.

### S12 — Sally Scrunch: first complete authored regional campaign (M4)

- S12.1 Full narrative sequence from Gubernatorial Gumball commission to first evidence of the King's last route; area introduction, Royal allies and siege preparation.
- S12.2 Sally's Crushing March doctrine: armored rams, blockade ships, heavy plated escorts, crushing walls, fortified lanes and siege artillery.
- S12.3 Counter-tools: armor piercing, shield cracking, intercepting rams and shooting destructible cover; shops/side missions supply accessible loadouts.
- S12.4 **Unique three-phase flagship battle**: defensive siege perimeter → crashing ram assault → exposed candy-core with fleet reinforcements. Boss art, breakable weapons, phase transitions, sound and VO are real.
- S12.5 Defeating Sally changes territory/vendors, unlocks her named component and gives a King clue that leads to Cringe's lanes.
- **Exit:** from fresh save, complete the whole chapter and boss, see territorial/narrative consequences, reload and observe them persistent.

### S13 — Captain Cringe: trap navigation and ambush campaign (S12)

- S13.1 Unique Twisted Passage region with false distress calls, stealth ambush, mines, trap beacons and moving decoys.
- S13.2 Build functional recon scanners, flares, decoy detectors, ECM and anti-ambush modules. These *change* combat decisions and encounter outcome.
- S13.3 Several authored chase, intelligence and rescue missions exposing contradictory accounts of the King's disappearance.
- S13.4 Distinct three-phase Cringe flagship battle: masked clones → disabled guidance/ambush field → exposed command bridge, with counters from equipped tools.
- S13.5 Story unlocks secret route, navigation block family, new vendors and usable intel for Sauer.
- **Exit:** player defeats real route deception using installed scanner, completes boss and retains unlocked graph path and clue on reload.

### S14 — Pete Pucker: corrosion and hazard campaign (S13)

- S14.1 Citric Front environment: acid nebula, corrosive wakes, heavy sour artillery, refinery fields and dissolving hull debris.
- S14.2 Real per-component corrosion and protections, shield interactions, coating consumption, repair windows and hazard-navigation choices.
- S14.3 Refineries/supply line missions recover Royal engineers and telemetry about the King's craft. Ship construction/heat/salvage decisions now matter.
- S14.4 Unique three-phase Pucker boss: artillery saturation → corrosive wake/pursuit → catalytic core vulnerability.
- S14.5 No mandatory ultra-rare counter: accessible purchasable/quest-granted sourproof path and balanced fallback loadout.
- **Exit:** distinct enemy roster and boss with counters; story chapter finishes across alternative builds without economic softlock.

### S15 — Senator Sauer: political capital and combined fleet campaign (S14)

- S15.1 Bitter Parliament region: bribed ports, forced tariffs, captured repair depots, propaganda transmissions and manipulated trade lanes.
- S15.2 Mixed-fleet command AI: coordinated shield escorts, capital batteries, jamming, interceptors and elite guards reacting to player positioning.
- S15.3 Missions use earlier allies, saved intel, faction reputation and ship configuration choices; a previous liberation changes tangible support.
- S15.4 Multi-mission assault on relays, logistics, personal flagship and command parliament. Defeating Sauer is earned via weak points and combined mechanics, not just huge HP.
- S15.5 Final King coordinates unlocked only by authoritative required story state. Previous three sector flags remain correct under save/load/replay.
- **Exit:** all four generals have unique working boss fights, doctrine, dialogue/music/art and persistent chapter consequences.

### S16 — King reveal, finale and endgame (S15)

- S16.1 Written and implemented resolution for why the Candy King went missing; clues from each general build toward reveal, with no missing mandatory information.
- S16.2 Dramatic final transit run + authored capital encounter and/or story choice, accessible without optional secrets or arbitrary rarity gating.
- S16.3 Actual ending/credits/epilogue with runtime-visible art, music, voices, restored galaxy state and saveable postgame.
- S16.4 Final-tier chassis/exotic parts and optional postgame fleet battles/challenge lanes; optional endgame should not replace the ending.
- S16.5 Complete save migration and mainline progression guards: new game, interrupted boss, replay side quests and credits all work.
- **Exit:** fresh save completes introduction → all generals → finale → credits, and reload has valid postgame state.

### S17 — Art, sound, announcer, story scenes and comfort finalization (continuous, closeout after S16)

- S17.1 Enumerate every gameplay/item/environment/story ID. Every required asset has source, license/provenance, atlas/bank reference, package check and captured usage proof; missing file/reference is a release blocker.
- S17.2 Finalize ship blocks, enemy variant silhouettes, all weapon muzzle/impact FX, defense visuals, animated boss stages, cosmic tunnel/skybox layers, menu/garage/chart views and readable character portraits.
- S17.3 Announcer bank: combos, multikill, no-damage, elite, new part, special active, shield gone, boss, game over, general defeated, kingdom rescued. Include cooldown, overlap policy, ducking, user volume and captions. Source recordings must be original or rights-cleared.
- S17.4 Original audio: menu, garage, exploration, base combat, each regional score, individual general boss themes, finale and credits; event-specific SFX for all guns, impacts, shields, engines, salvage, interfaces and pickups.
- S17.5 Accessibility: comfort vignette/low motion, camera stability, head-look independent from movement, text scale, color-independent tells, effects reduction, one-handed controls, recovery and controller-loss behavior.
- **Exit:** every mandatory visual/sound is actually seen/heard on the shipped phone, and all assets pass mapping/provenance checks.

### S18 — Production balance, compatibility, packaging and release (S16/S17)

- S18.1 Full mechanic coverage: 1000+ generated legal/illegal ship tests, corrupt save/upgrade version tests, economy replay/dupe checks, all boss phase transitions and deterministic graph reachability of all critical nodes.
- S18.2 Playability: multiple real builds for early/mid/late stages, damage/time/reward telemetry, no forced grind traps, documented optional challenges, skill curve, complete narrative playthrough.
- S18.3 Run Gradle/NDK compilation, native export/ABI/content manifest checks, runtime stereo and GL state, signed single APK, update signer/versionCode provenance and offline launch.
- S18.4 On reference Galaxy S9: cold start, controller and click-free touchpad travel, simultaneous steer/fire, visuals in both eyes, Home/Menu/recenter, controller drop, pause/resume, failure and save restoration.
- S18.5 Sustained combat and worst-case boss test with live measured frame-time distribution, thermal behavior, memory, draw calls and audio underruns; target stable device-native refresh. 60 Hz would imply 16.67 ms frame budget but must first confirm actual device/runtime refresh.
- S18.6 Final audit: copyright provenance, licenses, voice permission, credits, screenshots, release notes, traceable build SHA, verified release asset and APK. No claim of released unless remote published build exists.
- **Exit:** cold-start-to-credits S9 run and migration/restore run, no release-blocking defects, complete manifest and authoritative signed APK evidence.

## 3. Shared acceptance rules

**Performance:** treat Galaxy S9 budget as measured, not fictional. Favor low-poly/flat-shaded 3D cockpit, layered parallax candy-skyboxes, camera-facing enemy/VFX sprites, atlases and limited active actors. Draw one bounded scene at a time. Optimize after observing GPU/CPU thermals; never erase promised combat mechanics merely to mask an inefficient draw path.

**Input:** original Daydream has touchpad (touch and click), App/Menu, Home and volume rocker; not a fictional trigger/second joystick. Preserve platform-reserved controls and use tested remappable gestures or VR overlay for specials. Controller loss must leave reachable shell recovery.

**Assets:** one enemy is not complete until it has art + AI + spawn + damage + death + drop + sounds + tests. One ship block is not complete until inventory + build-grid + power/heat + combat + damage + atlas + save + cost + tests work. One general is not complete until story + regional ships + boss phases + original audio/art + unlock + persistent defeat state work.

**Campaign:** thousands of generated transit addresses are *space to explore*, not 4096 manually authored missions. Build authored chapter spines in four territories, surrounded by procedural variety and real persistent consequences.

**Per-slice evidence:** baseline and ending main SHA; changed paths; exact deterministic tests/outputs; actual APK verification; device/screenshots/audio evidence if applicable; asset-usage manifest; observed versus deferred requirements; next smallest blocked or ready task. No GitHub Actions by default.

**Related:** GUMBALL_ARMADA.md (brief/current status); GUMBALL_ARMADA_CONTENT_MATRIX.md (quantified production catalog); GUMBALL_ARMADA_CAST_BIBLE.md (planned merchants, two support pilots, Sour nuisance/rival characters; portraits deferred); INCREMENTAL_DEVELOPMENT_PLAN.md (platform ordering); ACCEPTANCE_LEDGER.md (verified states).
