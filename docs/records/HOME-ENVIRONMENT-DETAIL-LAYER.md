# ReverieVR — Second home-environment detail layer

Date: 2026-10-12

## Preserved environments

The three persistent home presets are **Birch Panel Living Room** (`white_room`),
**Pastoral Forest Glade** (`forest_glade`), and **Windswept Dune Beach**
(`dune_beach`). Existing room geometry, environmental preference values,
controller models, phone panel, mirror, and in-scene home-menu hierarchy remain
unchanged. This is a detail pass, **not** a scene replacement.

## Visual additions

- Birch: ceiling coves, picture-frame mouldings, waist-high rails, inset wall
  art and small books/lamp props on the existing side consoles.
- Glade: layered foliage on the existing tree ring, faceted mossy stones, and
  small meadow flower heads outside the center interaction lane.
- Beach: shoreline-adjacent wavelets, subtle sand ripples, clustered dune grass,
  and light shell/pebble accents, keeping the water and horizon legible.

## Third pass — PS1 silhouette and seeded primitive recipes

The next authored layer adds two motif compositions inside the existing birch
picture frames, four coarse fallen logs at the outer forest tree ring, and
six three-face tide rocks at the sides of the beach. Those shapes use compact
source recipes and fixed integer seeds, not baked assets. Generated vertex
positions snap to an intentionally coarse 1/128-metre grid **once** during
construction, and accent colors are selected from a tiny RGB555 palette
initialized once, making the entire new layer visibly faceted without modern
smooth-material effects. This is intentionally an approximation of the
PlayStation-era look; no claim of actual PS1 hardware emulation.

The material choice remains texture-free: no extra PNGs, texture uploads,
normal maps, fragment-noise passes or per-eye procedural regeneration.
One additional draw call **per eye in the selected preset** (three small new batches across all presets)
carry the new accent groups. Each preset gains at most 24 extra triangles,
and the existing scene-specific caps (95/230/240) remain in force with
determinism, finite-bounds and interaction-corridor tests. Both eyes use
the same immutable generated buffers. Reference S9 startup cost, battery,
thermal and perceived visual quality still require physical measurement.

## Rendering, comfort, and acceptance

All new meshes are generated deterministically **once** during renderer
construction as flat-shaded GLES2 triangles. No shaders, textures, lighting
passes, particles, streaming, or per-frame mesh allocations were introduced.
Ten extra merged draw calls per eye are distributed only across their own
scene: three in birch, three in forest, four at the beach. Extra triangle caps
per visible scene: birch <=95, forest <=230, beach <=240 (JVM-tested). A
geometry contract checks finite coordinates, world bounds, and exclusion of
the central forward-facing menu and desk interaction corridor.

Sustained Galaxy S9/Daydream View stereo performance, 60fps frame pacing,
thermal limits, furniture readability, and real headset visual appearance
remain **device acceptance gates**. A GitHub APK build is source/package
validation, not a substitute for physical comfort testing.

## Mesh-contact targeting (2026-10-11)

The shell targeting beam now tests the actual static generated triangle meshes
of the selected Birch Living Room, Forest Glade, or Dune Beach once per Home
input sample, **not once per eye**. It uses a lightweight bounded AABB rejection
and exact double-sided ray/triangle intersections, caches mesh CPU geometry at
construction, and picks the nearest hit within 12 metres. Walls, floors, dune
features, tree/foliage primitives, furnishing silhouettes and the keyboard
keycaps are included. Interaction geometry for menu buttons and keyboard
keeps priority within a 35mm tolerance (to accommodate caps above the input
plane), avoiding phantom obstruction by tiny decorative geometry. Obstructed
UI targets no longer hover as if physically reachable.

When a ray reaches an environmental triangle, the beam stops at the surface
and a texture-free cross is formed in its normal plane, offset 12mm toward
the viewer to prevent z-fighting; scene depth testing remains active. The
original non-occluded marker still applies to actual modal/UI targets.
Both stereo eyes use the same ray sample and collision result. No GPU
readbacks, new textures, physics engine, per-eye triangulation, world
deformation, or per-frame mesh synthesis is involved. Native games retain
their own game-world raycast ownership; this implementation covers the
three ReverieVR Home backgrounds only. Host JVM tests cover AABB near-miss,
triangle-only collision, nearest surface, winding, normal polarity, distance
limit, nonfinite input and wall/floor marker orientation. Physical S9
stereo/depth/readability and performance acceptance remain pending.

## Fourth home detail pass (2026-10-12)

The existing immutable mesh batches now contain a few more deliberately
low-resolution features without additional materials, draws or application
subsystems. Both birch-wall art panels gained hard-edged geometric sunrise
crests and fine roofline rays; the Forest Glade flower mesh now also contains
eight simple faceted mushrooms dispersed beyond the center desk/menu
corridor; and the Dune Beach wavelet mesh contains eight small shallow tidal
pools along the flanks.

These additions extend the original `birchGraphicMotifs`,
`forestWildflowers` and `beachWavelets` geometries rather than creating
three more drawable mesh objects. The affected detail batches contain
18, 96 and 50 triangles respectively. Coarse 1/128-metre vertex snapping,
existing collision/raycast ownership, original scene-specific triangle caps,
all three home preference identifiers, and the single-APK low-power contract
are unchanged. Regression assertions cover the new triangle counts and
the preexisting scene-bounds, interaction corridor and determinism contracts.
Android JVM tests, stereoscopic rendering and Galaxy S9 thermal/runtime
acceptance remain separate gates and are not claimed here.
