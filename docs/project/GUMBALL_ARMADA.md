# Gumball Armada — The Search for the Candy King

**Target:** ReverieVR, one phone-resident APK, Galaxy S9 / Daydream.  
**Status (2026-10-16):** Development-only S01/S02 combat, S03 starter weapons, S08/S09 objectives and collectible salvage, **partial S06 connected shipyard**, and a **partial source-only CAST-02/03 support-and-grenadier encounter slice**. The standalone shipyard component and tests were byte-verified against the commit and passed strict GCC/Clang and Clang ASan/UBSan. The integrated Android/NDK APK and physical S9 headset playthrough remain unverified; authored textures, deep voiced announcer and full campaigns remain unimplemented.

## Premise

The Candy King has vanished. With the royal confectionary fleet fractured, the Gubernatorial Gumball sends an inexperienced commander into the interplanetary lanes to retrieve the King. The Sour Armada has seized the routes. Four generals lead its armies: Sally Scrunch (siege and crushing formations), Captain Cringe (ambushes, traps), Pete Pucker (citric weapons and area denial), and Senator Sauer (political manipulation and late-game fleet command). The Regent's public optimism may conceal the King's actual fate.

## Design identity

A candy-painted flight-action RPG, with modular ships made from licorice trusses, gumdrop power couplings, nougat armor, hard-candy barrels, fizzy reactors and wafer sails. Deep voice combo callouts such as "SUGAR RUSH!", "SPECTACULAR!", "ABSOLUTELY CONFECTIONARY!" and "SOUR ANNIHILATION!" should feel ludicrously monumental. Those recordings, voice delivery and original score are future production work; CURRENT callouts are visual text only. Do not copy Kingdom Hearts or Candy Crush code, screens, audio recordings, brand assets, visual designs or copyrighted characters.

## Implemented native combat and weapons (source-only)

- 4096 deterministic forward-only transit addresses in four enemy regions, with three charted route choices per station. Still **not** a traversable bidirectional galaxy or authored campaign.
- Three-wave sorties with distinct Scout, Interceptor, Bomber, final Ace and fortress roles; different movement/firing/barrages, tracked player hits, unavoidable objective failure if mandatory elite escapes, and a three-escape limit.
- One-controller pad steering without holding click; held click fires. Hurt telegraph, limited hit grace, delayed shield recharge, visible mission objectives and after-action cash/bonus/escapes/combo tally. Current GLES2 visuals are **procedural placeholder silhouettes** and a moving starfield, not finished sprite/texture assets.
- **Three working purchased/equipable weapon modes:** starter Pea Repeater (rapid standard shots), Rock Cannon (slow high-damage slug) and Mint Spread (three diverging shots). Cash purchases, ownership and active selection save persistently, not just displayed labels. Weapons reuse provisional existing native shell gun SFX, not unique candy recordings.
- Three route-specific missions now affect actual combat: **Sour Sweep** (three escapes fail), **Courier Intercept** (strict zero escapes, a bounded timer and a distinct fast, evasive, unarmed priority courier ship) and **Salvage Recovery** (three collected gumdrop drops required). Enemies emit bounded physical salvage pickups that drift downscreen and award extra Gumball Cash only when the player steers into them. Missing the pickup quota after remaining drops expire fails recovery. UI reports live mission objective, time/collection status and accurate failure reason.
- **First working modular ship-construction slice (source):** in-headset 7×7 connected-component grid with a fixed central Candy Core, stock-constrained Frame/Armor/Wing/Thruster/Reactor/Battery/Shield/Gun Mount/Cargo types, reversible adjacency-safe placement and removal, power-draw and mass/thrust readiness gates. Parts carry real flight effects: armor/hull, shield capacity, speed/weight, wider salvage pickup radius, and actual muzzle origins; a second powered Gun Mount creates a second simultaneous firing point. Each assembled component is rendered on the flying ship rather than only stored as a menu label.
- Shipyard view is reachable from the existing chart using the normalized touchpad axes and click. Selecting a stocked item sets the edit brush, clicking an empty grid cell places it, clicking an installed non-core component removes it, and Return closes the editor. Illegal/disconnected parts reject with failure feedback; incomplete/overloaded ships cannot launch. The editor itself has a compact item catalog, stock counts, mass/power/thrust diagnostics, and red readiness warning.
- Shipyard layouts persist in an independent, fixed 72-byte `shipyard.v1` module-private save with version/magic, explicit checksum, structural/stock validation and transactional restore; missing/corrupt saves fall back to the default starter design. This is deliberately **not yet** a fully purchased/scavenged parts shop: the editor exposes a bounded starter crate with finite components, while cash and weapon upgrade ownership remain in the existing `campaign.v1` binary v2 slot. A later production stage will unify external acquisition and inventory economics without unsafe cross-slot partial purchases.
- Four earlier numerical upgrade tracks remain present alongside these physical parts; commander skill tree, named hull classes, localized part damage, tiered loot and full fleet-building are still unimplemented.
- Module save slot is still named `campaign.v1` for compatibility, but the **binary payload is now schema version 2** containing weapon ownership and active weapon. The module reads and validates older version-1 snapshots, migrates their existing cash/levels/position to v2 and atomically rewrites the slot. Runtime failures surface a save-error warning. The game does not yet resume mid-sortie.
- The source remains Development-only and governed by unchanged native ABI v1. No Android APK, Galaxy S9 launch, dual-eye comfort or complete campaign acceptance is claimed.
- Do **not** claim the Candy King has been rescued when defeating four prototype generals. Until a real finale is authored the UI says that the Sour Armada is defeated but the King remains missing.

## Delivery gates

| Stage | Output | Evidence required |
|---|---|---|
| GUM-01 | Native integration foundation | Exact-remote C++ test, native-doc sync, NDK build and APK exported-library verifier |
| GUM-02 | Controller/VR acceptance | Galaxy S9 headset: touchpad steers without click, click shoots, dual-eye geometry, shell menu recovery, thermal/session test |
| GUM-03 | Deep modular ship-building | Slots, weight/power, modular chassis, loot, salvage, steal and purchase equipment |
| GUM-04 | Actual semi-open galaxy | Revisits, settlements, secret routes, dynamic Sour territory, trading and reachable-map validation |
| GUM-05 | Four authored generals | Enemy doctrines, distinctive fleets, multi-phase capital ship bosses, story events and boss consequences |
| GUM-06 | Original art, sounds and deep narrator | Original asset inventory, recording provenance, real voice-trigger engine, captions and audio settings |
| GUM-07 | Complete Candy King campaign | Level curve, commander progression, main quests, fleet systems, save migrations and device-ready release |

**Test:** scripts/verify-gumball-armada.sh runs standalone C++17 regression code. No new GitHub Actions are required. The source module is not accepted as a tested handset game until Android/NDK and physical-device evidence exists.

## Recurring side characters and support cast

**Current partial code slice:** once a sortie has been completed, a deterministic guest selection can produce Pip's bounded escort shots, Lolly's bounded shield/salvage aid, Blort's slow interceptable double-bomb attacks, Nib's shootable minefield, or separate Biff/Baff offset bomb waves (or no guest). Those are actual simulation/render source paths with procedural silhouettes and a new unit-test runner; the full integrated native build and S9 VR pass remain pending. The other eight characters, complete shops/quests, bespoke sprites, portraits, voice and future Extra Lime continues remain planned. The final four-general results screen is no longer a permanent trap: it returns to the chart, while the Candy King remains missing.

The **14 named character concepts** (six shopkeepers/operators, two friendly mid-sortie support pilots, and six recurring Sour nuisance/rival pilots) have an authoritative *planned* cast bible: [GUMBALL_ARMADA_CAST_BIBLE.md](GUMBALL_ARMADA_CAST_BIBLE.md). It records original characterization, shop archetype assignments, sample dialogue, future encounter mechanics and visual briefs. **Portraits, voice lines and shop transactions are still planned; the six runtime-source characters are described above, without artwork or sound assets.** Character images are deliberately deferred for later production.

## Full production roadmap and release-content targets

The development roadmap is now decomposed into 19 gated production stages, S00 through S18, from source verification to one full arcade sortie, weapons/defenses, genuine player-built ships, salvage and shops, revisitable galaxy, four individual Sour general campaigns, finale, audio/art completion and signed APK acceptance. See [GUMBALL_ARMADA_STAGED_PLAN.md](GUMBALL_ARMADA_STAGED_PLAN.md). Quantified planned catalogs (functional blocks, enemies, authored missions, voices, music, effects, art provenance and integration gates) are in [GUMBALL_ARMADA_CONTENT_MATRIX.md](GUMBALL_ARMADA_CONTENT_MATRIX.md). All targets are **planned**, not claims that these assets or systems currently exist.
