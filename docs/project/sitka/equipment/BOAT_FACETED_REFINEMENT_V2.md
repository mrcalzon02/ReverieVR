# Sitka boat faceting and geometry revision (v2)

**These are ACTUAL generated triangular OBJ meshes with MTL materials,
not 2D artwork, side-view sprites or in-game screenshots.**

The earlier baseline of eighteen three-tier six-stage designs remains
in the [original GitHub review gallery](../../../../gallery/sitka/boats/2026-11-05/README.md).

This pass changes all boats' hull sections to 11 class-specific stations
with trawler fullness, skiff shallow chines, and coastal boat bow flare;
keeps the existing continuous hull/deck/bulwark rules; replaces exposed
box-like wheelhouse and storage structures with octagonal chamfered shells;
generates raked forward dual windshields, side and aft glazing following
real faces; adds crowned roof geometry; reshapes the final trawler's raised
bridge; and replaces cubic motor caps with eight-sided cowls.

New source regression checks explicitly reject square eight-vertex
wheelhouses/deck cabinets, invalid model face indices, out-of-bulwark
deck supports, non-monotonic hull stations, degeneracy and malformed
shore-clearance geometry. The generated physical meshes total 29,236
triangles across 18 boats, preserving the under-5,500 triangles-per-boat
design budget. Separately, six physical crab gear OBJ parts remain.

Regenerate: `python3 scripts/sitka/build_boat_rough_models.py OUT`.
Test: `python3 scripts/sitka/test_build_boat_rough_models.py`.
VTK visual rendering: `python3 scripts/sitka/render_model_360.py`,
optionally `--tier 3 --only tier3_05`.
The 18-GIF offline test: `python3 scripts/sitka/test_model_360.py`
after generating both models and all 24-frame previews.

**Open:** Integration into the native boat renderer, player walkable
collision decks, buoyancy, interactive equipment and device acceptance.
Model preview water is illustrative and the 18 visual variants are
not eighteen native navigation profiles.
