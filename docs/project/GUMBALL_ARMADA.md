# Gumball Armada — The Search for the Candy King

**Target:** ReverieVR, one phone-resident APK, Galaxy S9 / Daydream.  
**Status (2026-10-15):** Development-only native source integration; Android/NDK packaging and physical headset playthrough NOT verified.

## Premise

The Candy King has vanished. With the royal confectionary fleet fractured, the Gubernatorial Gumball sends an inexperienced commander into the interplanetary lanes to retrieve the King. The Sour Armada has seized the routes. Four generals lead its armies: Sally Scrunch (siege and crushing formations), Captain Cringe (ambushes, traps), Pete Pucker (citric weapons and area denial), and Senator Sauer (political manipulation and late-game fleet command). The Regent's public optimism may conceal the King's actual fate.

## Design identity

A candy-painted flight-action RPG, with modular ships made from licorice trusses, gumdrop power couplings, nougat armor, hard-candy barrels, fizzy reactors and wafer sails. Deep voice combo callouts such as "SUGAR RUSH!", "SPECTACULAR!", "ABSOLUTELY CONFECTIONARY!" and "SOUR ANNIHILATION!" should feel ludicrously monumental. Those recordings, voice delivery and original score are future production work; CURRENT callouts are visual text only. Do not copy Kingdom Hearts or Candy Crush code, screens, audio recordings, brand assets, visual designs or copyrighted characters.

## Implemented first slice (source)

- A seeded, deterministic chart with 4096 addressable transit nodes in four 1024-node enemy regions. Each chart position offers three forward routes and hazard/reward hints. It is NOT yet a genuine bidirectional open world with backtracking or political territory simulation.
- Real-time waves, finite enemy/projectile arrays, Daydream pad flight steering, click-held shooting, fixed shot cooldown, hostiles that return fire, shield/hull damage and heavy damage when enemies escape. Fortress bosses must actually be destroyed.
- Gumball cash payouts, kill streak/combo multiplier, commander rank, four purchasable tracks (blaster/hull/thruster/shield), four bosses and a campaign-end state.
- Module-private campaign.v1 save slot on upgrade purchase and sortie resolution; schema-version/checksum and bounds verification at load. In-progress flight is not persisted as a resumable sortie.
- Original low-cost GLES2 stereoscopic guest scene: selectable three-route chart, four-component shop, colored ship silhouette and enemies, projectile meshes, status HUD and combo text. It uses unchanged native module ABI v1 and is admitted only in Development logging mode.
- Daydream pad X/Y uses the already-implemented touchpad-aware native-classics profile for this seated arcade (NO shell locomotion); pressing click is NOT required for steering. Click buys/launches and is held to fire; Menu and Home remain shell-owned. This is source routing, NOT a physical-input acceptance claim.
- The host's existing weapon machine cue supplies provisional shot feedback, not distinctive candy audio. The requested recorded deep-voice announcer does not exist yet.

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
