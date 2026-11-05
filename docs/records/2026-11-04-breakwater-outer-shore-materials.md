# Breakwater — exterior shore and battery repeat-UV material implementation

Date: 2026-11-04
Source base: `f41a37070fa4d69602d2ce230f4b546b6d4d6bd7`
Scope: game-private visual geometry only; reuse the existing 16-asset shared v1 coastal material namespace.

## Applied
- Iron Quay: the existing two 16m harbor piers and matching seawalls gain repeated 2m DockPlanks and MossySeawall cladding panels, respectively. Fourteen bollards per shore pair (seven per side, each 2-part) and six mooring posts acquire weathered Steel surfaces. The underlying boxes/collision/depth remain unchanged.
- Shingle Bay: the existing eighteen distant side stones gain PebbleGround faces, two driftwood beams acquire CratePlanks, and six rear-track segments gain DockPlanks.
- Blackcap Island: the existing eighteen side stones and eight decorative outcrops receive Basalt.
- Iron Quay rear area: two supply sheds receive ReinforcedConcrete, two overhangs CorrugatedRoof and two chimney structures RivetedBulkhead.
- Gun emplacements: the unchanged solid apron is now dressed in eight ~1m wide DiamondPlate sections, not a stretched single tile; mounted gun cradle gains DiamondPlate and OlivePaint cladding.
- The game retains no new PNG asset copies or GPU texture handles. All UV faces append to the existing textured sprite mesh; the renderer still draws the original opaque XYZRGB batch followed by one textured batch in the same stereo-eye draw. Game collision, aim, troops, economy and save data are untouched.

## Geometry and validation
`EmitCoastalTerrain` and `EmitSurroundingLandscape` have additive cladding-callback overloads. Their older flat-color callback signatures are still valid and are used in existing campaign/environment tests.

New `breakwater_shore_cladding_test.cpp` validates expected material distribution and finite UV geometry across all three coasts, both day and night, rejects invalid locations and checks that day/night geometry is identical. The existing `breakwater_emplacement_test.cpp` requires eight repeated apron panels. Both are included in the existing Breakwater host-visual runner and Android acquisition workflow's source-compilation stage. The Android installed-module test also checks all sixteen 64×64 PNG signatures and dimensions in the actual APK and CI expects the existing module to log `Authored Breakwater materials loaded 16/16` after its native create path. These are required *runtime read proofs*; they do not by themselves establish headset appearance, thermals, or frame pacing.

## Remaining acceptance
The source and blob integrations are deterministic. Verify the full Android build, actual native sprite output/screenshot (not concept art), GPU frame budget, depth precision near panel joints, near-object occlusion, both eyes and Galaxy S9 thermal behavior before marking physical acceptance. Procedural fallback remains for absent/corrupt tiles. Avoid adding another per-game texture atlas or network download dependency.
