# Tier 2 and Tier 3 physical vessel and crab-pot development

**Development status: real low-poly OBJ/MTL rough blockouts and test-only
crabbing rules; NOT yet an installed/playable Sitka trawler.**

Tier 1 already has six user-approved skiff hull concepts. This package
expands **only** the missing Tier 2 (center-console/cabin fishing vessels)
and Tier 3 (commercial working trawlers), with SIX additional 3D
configurations per class and six standalone crab-rig model variants.
The source is deterministic Python; no 3D model is faked by placing a
poster on a plane. Models include walkable deck polygons, bridge/cabin,
mounted racks/equipment, and tier-distinct hull geometry.

- [Tier 2 six-boat review sheet](https://drive.google.com/file/d/1cMWUqeeX_DqoXJd14zhbdDsKV9Vg7pHC/view?usp=drivesdk)
- [Tier 3 six-boat review sheet](https://drive.google.com/file/d/1CK2w86lg6Ett69H128PNXu-Cjtn1Vuyj/view?usp=drivesdk)
- [All 12 OBJ/MTLs, six crab-rig OBJ/MTLs, previews, generator and manifests](https://drive.google.com/file/d/1LajaryRWQPkeVTDdNoRHGa1YbJn4_lJf/view?usp=drivesdk)
- [Original wheel/helm/boat design document](https://docs.google.com/document/d/1mA5x2NZhW-6ZyaGySphw0t2aP0F3ioyYhMBTPV2dhLQ/edit)

The original binary OBJ/MTL pack is archived on Google Drive; GitHub
stores reproducible source, two JSON manifests, and tests.
Run python3 scripts/sitka/build_boat_rough_models.py to regenerate,
then python3 scripts/sitka/test_build_boat_rough_models.py to validate.
The runtime currently holds four hull STATES (dinghy, skiff, pilothouse,
trawler), not six installed upgrade levels per major class. These six
rough configurations per class are planning steps pending game integration.

## Tier 3 crab-pot economy: source-only simulator

The large trawler remains capable of hand-rod fishing, but the main
endgame income is baited commercial-style crab pots. One pot is set,
soaks while the player sails or tends others, then its buoy is found
and its catch hauled onto the physical deck. Crab income is paid once,
only after returning to a designated port. Each set costs either 100
game credits in purchased fish bait, or one fish previously retained
for bait; lifetime fish-sale totals do not automatically create bait.
All money, catch counts, timings and fuel numbers are game-balance
prototypes, NOT actual Alaska fisheries data.

| Rough crab rig | Set time | Haul time | Pot limit | Physical model |
| --- | --- | --- | --- | --- |
| Handline (painfully slow) | 24 min | 30 min | 1 | rope, buoy, pot, bait |
| Manual crank | 19 min | 24 min | 2 | crank and line guide |
| Deck line-puller | 14 min | 18 min | 3 | improved mount |
| Powered pot hauler | 10 min | 12 min | 4 | motor and reel |
| Hydraulic gantry | 7 min | 8 min | 5 | gantry and handling deck |
| Expedition crane | 5 min | 5 min | 5 | crane and bait station |

A one-hour game soak starts AFTER each pot is fully placed. The upper
rig target is 20 minutes outbound, three 5-minute sets, wait for the
first pot's 60-minute soak, three 5-minute hauls, then 20 minutes home.
Without further between-site travel this totals approximately 120
minutes (20+15+50+15+20), not 60–90 minutes. Shorter sessions would
require an explicit game-clock setting or shorter designed soak time.
Individual catches vary deterministically. The endgame five-trips
per fuel tank target is represented in the simulator as approximately
20% of a tank per full round trip, not as a physically tested engine.
Actual water depths, gear rules, legal seasons/species/limits and
catch probabilities still need sourced study.

The five-stage character Deckwork skill from the native V4 journal
is actually **eight levels** and can reduce the slow early handling
durations by up to 20% in the isolated crab work-cycle prototype.
The top crane preserves the intended 5-minute handling floor.

The crab simulator is CPU-only and has GCC/Clang tests. It is NOT
hooked to the running boat Update loop, runtime gear slots, real pot
buoy props, save transactions, boat fuel, lawful harvest checks,
realistic bathymetry eligibility, marketplace UI, or physical loading
animations. Do not claim crab hunting is playable in the APK.
