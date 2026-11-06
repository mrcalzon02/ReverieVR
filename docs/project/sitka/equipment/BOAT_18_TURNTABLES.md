# Sitka 18-vessel physical turntables and navigation review

**2026-11-05 design study.** All 18 ships were rendered from
the exact deterministic, low-poly OBJ geometry (six visual upgrades
per class), with a static illustrated ocean plane. Every one of the
18 looping GIFs contains 24 real 3D angles at 15° increments:
360° across all frames; **no AI-painted substitute** or implied
playable Sitka footage.

- [Complete source OBJ/MTL and all 18 rotating GIFs, 3 six-vessel
  image galleries, deterministic generator, renderer and tests
  (Google Drive ZIP)](https://drive.google.com/file/d/1CSXxI58t1009dl-52nx1tSizWPFwDWx-/view?usp=drivesdk)
- [Prior 12 higher-tier shaped hull package](https://drive.google.com/file/d/11JI2iNGoDPz0k1mh-kDqEet_TYn0AuKp/view?usp=drivesdk)
- [User-approved Tier 1 original skiff concept](https://drive.google.com/drive/folders/1V5-OnZ8im7j7u9o1w9DQk3ZnHCyb8Loi)

## Actual geometry

Tier 1 (Dinghy / Skiff) **six new OBJ designs** are physical
prototypes derived from the approved six-step skiff concept:
Shore Dinghy 2.4 m, Utility Skiff 3.6 m, Reinforced Harbor Skiff
4.2 m, Coastal Fishing Skiff 5.2 m, Deepwater Aluminum Skiff
6.1 m, Master Angler Skiff 6.8 m. Their beam and progressive
benches, motor, rail, helm, sonar, and T-top follow the reference.
Tier 2 (Coastal Cabin Boat) uses the previous six refined
hulls 6.2–8.0 m. Tier 3 (Crabbing / Commercial Trawler) uses
the previous six 10.8–14.4 m working vessels.
All share the nine-section chine/keel hull, continuous gunwales
and contour-inset deck rather than a rectangular penetrative slab.
Every model has mesh source geometry, not just an icon.

The raw OBJ/MTL and the 18 GIF binaries are in Drive; their source
generator, image renderer, validation code, and exact-model manifests
are Git-committed here. This avoids embedding animated GIF files
into the APK or falsely representing those preview models as
installed game meshes.

## Reproduction / checks

Run from the repository (requires Python 3 only):
`python3 scripts/sitka/test_build_boat_rough_models.py`.
To regenerate models:
`python3 scripts/sitka/build_boat_rough_models.py /tmp/sitka-meshes`.
For the optional preview renderer (VTK, NumPy, Pillow), run in an
isolated working directory containing the generation source:
`python render_360.py --frames 24`.
Validate the exported Gallery ZIP using the packaged
`test_turntables.py`. The GitHub generator regression is pure Python
stdlib, so it does not require VTK or an Android GPU.

## Navigation versus turntables

The actual native source-only navigation in
`app/src/main/jni/native/sitka_boat_navigation.h` has only **four
purchased hull profiles** (dinghy, skiff, pilothouse, trawler), not
18 runtime purchase tiers. Its new regression exercises all four
over 20 simulated minutes per boat, checks 30 Hz determinism,
depth-dependent draft admission, turn-rate limits and a five-point
hull footprint against shoreline masks and bottom depth.
The test basin is synthetic; do not interpret it as Sitka
bathymetry, maritime safety guidance or measured boat performance.

None of these previews certifies water buoyancy, dynamic rigid-body
collisions, berthing physics, user/device VR handling, or playable
module registration. Those remain distinct future integration steps.
