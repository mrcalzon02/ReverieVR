# Lantern Desk permanent terrain library — LTIL v1

**Canonical once-baked terrain identities: exactly 1,000.** These are the
complete fixed library. Do **not** generate 1,000 new tiles each war. Do
**not** regenerate one of these heightfields when a reconnaissance
photograph is taken, a photo is enlarged, or weather changes.

The 10 binary files under `app/src/main/assets/native/lantern/terrain/`
are part of the one unified ReverieVR APK. Each has 100 permanently
numbered physical places. The verified manifest beside them gives
source byte count, FNV-32 pack checksum and immutable Git blob SHA.
A future design revision may adjust a particular place deliberately;
never silently reroll the entire catalog or change a place's ID.

## Per-record absolute byte layout (2,880 bytes)

| Offset | Bytes | Meaning |
|---|---:|---|
| 0 | 2 | permanent tile ID (little-endian) |
| 2 | 4 | original baking seed |
| 6 | 1 | theater family 0–9 |
| 7 | 1 | biome index 0–9 |
| 8 | 1 | elevation profile 0–5 |
| 9 | 1 | static river/village/rail flags |
| 10 | 2 | terrain base altitude (decimeters) |
| 12 | 2 | terrain and road material indices |
| 14 | 2 | reserved zero |
| 16 | 2,178 | baked 33×33 plasma topographic elevation samples, decimeters |
| 2,194 | 256 | baked 16×16 walkability/navigation costs; zero = impassable |
| 2,450 | 18 | nine ordered road centerline 2D waypoints |
| 2,468 | 48 | six building footprints: x,y,width,depth,height,kind,rotation,exists |
| 2,516 | 288 | 48 canopy impostors: x,y,height,radius,species,age; zero=absent |
| 2,804 | 72 | 12 typed navigation and scene placement markers |
| 2,876 | 4 | per-tile FNV-32 checksum |

The 32-byte pack header contains `LTIL`, version 1, exactly 100
records, stride 2,880, map dimensions, the master bake seed, pack
FNV-32 checksum, and permanent group start tile ID.

The terrain is represented by **baked source samples**. Height
interpolation, solar/slope lighting, billboards and local scene
assembly are *views of that saved terrain*, not new terrain generation.

## World-state boundary

`AssignWarfront(warSeed)` can distribute all 1,000 existing tile
identities to persistent 1,000 location slots when a campaign begins.
The permutation seed/mapping must be kept in war state. From that point
forward, the tile's roads, orchard trees, houses, rivers and slopes
remain the same through the campaign. Unit/vehicle orders, tents,
temporary fieldworks, campfires, damage, visibility and all capture
conditions belong to **timestamped snapshots** of that physical place.
The player's developed photograph exposes no automatic World Truth
grading or arbitrary regenerated geometry.

`NavPath` uses committed passability and terrain anchors. It refuses
diagonal corner cuts; `ResolveAt` derives movement and unit
concealment from the same saved tile and time.

The current three-sector Lantern Desk module now **reads tile records
0237/0379/0240 from these exact sealed assets**, and places authored
tutorial moving units with live world-height foliage/structure
occlusion and capture-time solar/moon/cloud lighting. The entire
1,000-location warfront simulation, persisted world mapping,
arbitrary movement orders and historical campaign state are not yet
wired. Nothing may replace terrain ID 0240 with another procedural
river on a revisit. Do not misrepresent hosted tests as proof of
all 1,000 populated campaign sites or physical headset acceptance.
