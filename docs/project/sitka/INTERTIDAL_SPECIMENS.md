# Sitka intertidal specimen model catalog (source-only)

25 tiny low-poly physical shoreline specimens are defined in
`app/src/main/jni/native/sitka_intertidal_models.h`. These models are
geometry for world-space objects, NOT inventory icons or historical source
photographs. Actual placement will continue to require a reviewed
`sitka/beach/discoveries.txt` source anchor, never random real-world
geographic claims. The standard five-foot / 1.524 m reveal cutoff
applies equally to all specimen types, including artifacts, with a
centimeter-scale pixel-like highlight. **Current GLES2 layer is RGB vertex
colors, not a true emissive texture or bloom shader.**

Species and habitat references:
- Sitka NPS overview: https://www.nps.gov/im/sean/sitk.htm
- ADF&G Alaska tidepools: https://www.adfg.alaska.gov/index.cfm?adfg=viewing.tidepooling
- ADF&G native shore crabs: https://www.adfg.alaska.gov/index.cfm?adfg=wildlifenews.view_article&articles_id=1048
- Cultural finds: https://www.nps.gov/lacl/learn/historyculture/sites-artifacts-and-collecting.htm
- NOAA tidepool care: https://oceanservice.noaa.gov/facts/tide-pool.html

**Ecological examples, not guaranteed finds:** a listed species can
exist in the Southeast Alaska intertidal ecosystem, but its presence
at a particular shoreline point must not be inferred from this catalog.
Some common names (e.g. generic sea star) are deliberately non-specific.
No sea creature is modeled as a harvestable collectible. Cultural
finds are deliberately prefixed "Possible" and require explicit
source-reviewed placement; they must always remain in the world after
inspection. Never imply historical finds can be removed or sold.

| Token | Name | Action | Habitat requirement | Size (radius m) | Rarity |
| --- | --- | --- | --- | --- | --- |
| `empty_clam_shell` | Empty clam shell | Collect | Gravel, Wrack | 0.065 | common |
| `crab_molt` | Discarded crab molt | Collect | Rocky, Wrack | 0.092 | uncommon |
| `beach_pebble` | Interesting beach stone | Collect | Gravel, Wrack | 0.052 | common |
| `sea_glass` | Sea glass shard | Collect | Gravel, Wrack | 0.035 | uncommon |
| `driftwood_chip` | Driftwood fragment | Collect | Wrack, Gravel | 0.077 | common |
| `purple_shore_crab` | Purple shore crab | Observe | Rocky, Pool | 0.047 | common |
| `green_shore_crab` | Green shore crab | Observe | Rocky, Pool | 0.041 | common |
| `hermit_crab` | Hermit crab | Observe | Pool, Rocky | 0.060 | uncommon |
| `small_sea_star` | Small sea star | Observe | Rocky, Pool | 0.095 | common |
| `limpet` | Limpet | Observe | Rocky | 0.035 | common |
| `chiton` | Chiton | Observe | Rocky, Pool | 0.074 | common |
| `barnacle_patch` | Barnacle patch | Observe | Rocky | 0.055 | common |
| `mussel_cluster` | Mussel cluster | Observe | Rocky, Pool | 0.063 | common |
| `littleneck_clam` | Littleneck-style clam | Observe | Gravel, Pool | 0.065 | uncommon |
| `sea_anemone` | Tidepool anemone | Observe | Pool, Rocky | 0.057 | uncommon |
| `nudibranch` | Nudibranch | Observe | Pool | 0.061 | rare |
| `sea_cucumber` | Sea cucumber | Observe | Pool | 0.084 | rare |
| `sea_urchin` | Sea urchin | Observe | Pool, Rocky | 0.073 | uncommon |
| `sea_snail` | Sea snail | Observe | Rocky, Pool | 0.033 | common |
| `kelp_float` | Washed kelp float | Collect | Wrack | 0.073 | common |
| `possible_worked_stone` | Possible worked-stone fragment | Document | Cultural | 0.053 | very_rare |
| `possible_net_weight` | Possible historic fishing weight | Document | Cultural | 0.062 | very_rare |
| `possible_shell_midden` | Possible shell midden trace | Document | Cultural | 0.100 | very_rare |
| `possible_carved_wood` | Possible carved wooden fragment | Document | Cultural | 0.076 | very_rare |
| `small_mottled_sea_star` | Mottled sea star | Observe | Rocky, Pool | 0.080 | uncommon |

## Deterministic population from real reviewed beach zones

`scripts/sitka/build_beach_discoveries.py --areas reviewed.json
--output sitka/beach/discoveries.txt` accepts **reviewed licensed
shoreline polygons in local EPSG:32608 metres** and produces stable
gameplay specimens inside those approved habitats. The input format is
`SITKA-BEACH-AREAS-1`: provenance (source ID, SHA256, source HTTPS
URL, license, reviewed flag and CRS), origin [east,north], and an
`areas` list with unique ID, habitat, 3–32-vertex polygon,
`first_site_id` and `count` (1–12). Author-assigned ID ranges
are immutable to protect the native save. Up to 64 sites are possible.

**Nothing here fabricates Sitka terrain.** These generated spots are
fictional creature representations inside genuine reviewed habitat,
not observed animal locations. There is intentionally **no**
protected-cultural area in this auto-populator; archaeological
finds require independent cultural/archaeological source review and
manual placement. No real Sitka coastline polygons are currently
staged, so the exporter is ready but produces no actual Sitka
discovery file until that prerequisite is met.
