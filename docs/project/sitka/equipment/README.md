# Sitka physical equipment — approved rough-model references

**Status:** Visual planning assets, not finished 3D models or a released game. These nine sheets were approved by the user on **2026-11-05** as the reference language for Sitka equipment, physical item pickups, shop information, and corresponding icons. Do not substitute flattened concept images for physical VR geometry.

The **nine original, full-resolution PNG concept sheets** are stored in the existing [Sitka PSX Material Drafts Google Drive folder](https://drive.google.com/drive/folders/1V5-OnZ8im7j7u9o1w9DQk3ZnHCyb8Loi) with verified filenames, lengths and SHA-256 recorded in [asset_catalog.json](asset_catalog.json). Source file indexing is Git-committed here; the binary PNGs are stored in Google Drive, **not** falsely represented as blobs committed in GitHub.

**Catalog coverage:** 85 illustrated tier concepts, **48 purchasable advancement levels** across the first five game-equipment categories, excluding their starter equipment. The other 37 pictures are proposed physical fittings and boat/navigation/storage variants; their presence here does *not* imply those tracks already exist in the game.

| Equipment family | Pictured variants (starter included) | Verified image | Current game-code relation |
|---|---:|---|---|
| Fishing rods | 13 | [Source PNG](https://drive.google.com/file/d/1kxc9g2lLsJVNmlENzzQlurIDFaqZlV8F/view?usp=drivesdk) | Journal.gear; levels 0–12 (12 purchased upgrades) |
| Reels | 11 | [Source PNG](https://drive.google.com/file/d/1kaVrQ8KQCj-j1pFos3s31QXAt7AiH5kd/view?usp=drivesdk) | Journal.reel_level; levels 0–10 (10 purchased upgrades) |
| Fishing line | 11 | [Source PNG](https://drive.google.com/file/d/1lKgwTTx7-eXOKNsnNdfc9UqXEs_3ZQId/view?usp=drivesdk) | Journal.line_level; levels 0–10 (10 purchased upgrades) |
| Lures and tackle | 9 | [Source PNG](https://drive.google.com/file/d/1QovmG4qm8iUS1fBir7yqc0ndTM3baHWv/view?usp=drivesdk) | Journal.lure_level; levels 0–8 (8 purchased upgrades) |
| Beach discovery kits | 9 | [Source PNG](https://drive.google.com/file/d/14oqAZmYcWA0UCIBzC4LGvp6Tql-kCC_p/view?usp=drivesdk) | Journal.shore_kit_level; levels 0–8 (8 purchased upgrades) |
| Skiff hulls | 6 | [Source PNG](https://drive.google.com/file/d/1dnwedmFGE4gJakr5E4TM8Id1L0qM4A4j/view?usp=drivesdk) | Journal.hull_tier currently levels 0–3; six pictured concepts require separate design/balance integration |
| Helm and outboards | 9 | [Source PNG](https://drive.google.com/file/d/1kXvJvppuGGDu2N2t7AZ8uhlLZiQeodRM/view?usp=drivesdk) | Journal.helm_upgrades is a fitting bitmask, NOT 9 linear upgrades; concepts need slot mapping |
| Catch storage | 9 | [Source PNG](https://drive.google.com/file/d/1jum3DRzh6lc8dbtx_qRoIb6Q8pBMn6gz/view?usp=drivesdk) | concept-only track; no implemented persistent storage-tier purchase |
| Fish finders and navigation | 8 | [Source PNG](https://drive.google.com/file/d/1p1QDW6jHq6Xz6PIJmVqR43558XhLeDHS/view?usp=drivesdk) | concept-only upgrade sequence; existing sonar and sourced bathymetry code must be reused |

## Production requirements

Read [MODELING_AND_ICON_CONTRACT.md](MODELING_AND_ICON_CONTRACT.md) before sculpting; start from `asset_catalog.json`. Stable `sitka.<group>.<nn>` IDs, individual physical cues, editor asset source refs, and the difference between `current` and `planned` functionality are there. Every separate purchased tier must be physically distinguishable without reading text; player-held rods/reels/line/lures, worn beach kits, boat-mounted fittings, bins, sonar and GPS have different world placements and attachment pivots. Icons are later renders **of those modeled objects**, not a crop of the full reference sheet.

**Data integrity:** These are fictional visual design references; the apparent Sitka logos, specifications, depths, fish shapes, labels and boat dimensions in generated sheets are not verified real-world measurements, tide forecasts, gear products, marine safety requirements or source-provenance records. The user's demand for **geographically accurate Sitka** still applies to shore, roads, buildings and terrain, independently of these stylized movable gear props.

**Modeling backlog:** start with starter rod + reel + spool + lure pouch + beach pouch and one held/equipped/inventory/icon pass; then add shared modular geometry and drive all 48 purchased variants; then shore pickups, skiff/helm variants, catch storage and fishfinder navigation. Use actual geometry and correct world scale; source .blend meshes and generated GLES2-compatible runtime assets should be committed separately when created and tested.

Run `python3 scripts/sitka/verify_equipment_reference.py` to check catalog completeness, tier numbering, source references and current fishing-upgrade counts.

