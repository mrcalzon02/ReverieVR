# Gumball Armada — The Search for the Candy King

**Target:** ReverieVR, one phone-resident APK, Galaxy S9 / Daydream.  
**Status (2026-10-15):** Development-only S01/S02 combat and partial S03 weapon source implementation; local equivalent GCC/Clang C++17 native tests pass, but Android/NDK packaging and physical headset playthrough are NOT verified. Full campaigns, authored sprite atlases and deep voice audio remain unimplemented.

## Premise

The Candy King has vanished. With the royal confectionary fleet fractured, the Gubernatorial Gumball sends an inexperienced commander into the interplanetary lanes to retrieve the King. The Sour Armada has seized the routes. Four generals lead its armies: Sally Scrunch (siege and crushing formations), Captain Cringe (ambushes, traps), Pete Pucker (citric weapons and area denial), and Senator Sauer (political manipulation and late-game fleet command). The Regent's public optimism may conceal the King's actual fate.

## Design identity

A candy-painted flight-action RPG, with modular ships made from licorice trusses, gumdrop power couplings, nougat armor, hard-candy barrels, fizzy reactors and wafer sails. Deep voice combo callouts such as "SUGAR RUSH!", "SPECTACULAR!", "ABSOLUTELY CONFECTIONARY!" and "SOUR ANNIHILATION!" should feel ludicrously monumental. Those recordings, voice delivery and original score are future production work; CURRENT callouts are visual text only. Do not copy Kingdom Hearts or Candy Crush code, screens, audio recordings, brand assets, visual designs or copyrighted characters.

## Implemented native combat and weapons (source-only)

- 4096 deterministic forward-only transit addresses in four enemy regions, with three charted route choices per station. Still **not** a traversable bidirectional galaxy or authored campaign.
- Three-wave sorties with distinct Scout, Interceptor, Bomber, final Ace and fortress roles; different movement/firing/barrages, tracked player hits, unavoidable objective failure if mandatory elite escapes, and a three-escape limit.
- One-controller pad steering without holding click; held click fires. Hurt telegraph, limited hit grace, delayed shield recharge, visible mission objectives and after-action cash/bonus/escapes/combo tally. Current GLES2 visuals are **procedural placeholder silhouettes** and a moving starfield, not finished sprite/texture assets.
- **Three working purchased/equipable weapon modes:** starter Pea Repeater (rapid standard shots), Rock Cannon (slow high-damage slug) and Mint Spread (three diverging shots). Cash purchases, ownership and active selection save persistently, not just displayed labels. Weapons reuse provisional existing native shell gun SFX, not unique candy recordings.
- Four numerical upgrade tracks and commander rank; these are **not** a modular block shipyard or full commander skill tree.
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

## Full production roadmap and release-content targets

The development roadmap is now decomposed into 19 gated production stages, S00 through S18, from source verification to one full arcade sortie, weapons/defenses, genuine player-built ships, salvage and shops, revisitable galaxy, four individual Sour general campaigns, finale, audio/art completion and signed APK acceptance. See [GUMBALL_ARMADA_STAGED_PLAN.md](GUMBALL_ARMADA_STAGED_PLAN.md). Quantified planned catalogs (functional blocks, enemies, authored missions, voices, music, effects, art provenance and integration gates) are in [GUMBALL_ARMADA_CONTENT_MATRIX.md](GUMBALL_ARMADA_CONTENT_MATRIX.md). All targets are **planned**, not claims that these assets or systems currently exist.
