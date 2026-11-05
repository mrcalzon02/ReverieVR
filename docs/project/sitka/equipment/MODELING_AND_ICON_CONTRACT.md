# Sitka physical model and icon production contract

## Original intent

The player walks freely through the Sitka city and shore in VR, physically holds the rod, reel, tackle and discovered beach objects, and equips or installs purchased objects. **No image-only gear:** the model must exist in the same 3D world as the player's hands, the boat and shoreline. The nine approved visual sheets in [asset_catalog.json](asset_catalog.json) establish concept shape and relative progression, **not** final game-ready geometry.

## Modeling workflow (per item)

1. Locate the exact `asset_id`, source sheet, and `physical_model_cue` in `asset_catalog.json`.
2. Block out from primitives at real-world dimensions. Invent only *equipment design*, never new Sitka geographic structures, named businesses or surveyed objects.
3. Split reusable components into independent parts (rod blank, rings, reel, line spool, pouch, hooks, case lids, boat seats, propeller, transducer). A reel purchase must change the actual attached reel, not just rename the same rod.
4. Put pivots at physically usable locations. Suggested standard local axes: +Y up, +Z forward, +X right. Consistent key slots: `hold_grip`, `reel_seat`, `line_exit`, `lure_eye`, `tool_attach`, `boat_mount`, `helm_mount`, `lid_hinge`, `transducer_mount`. Only bind slots the physical item supports.
5. Build PSX-era low-poly geometry, shaded flat/limited palettes, small deliberately UV-mapped textures and realistic simple silhouettes; do not paste full sheet artwork onto models. Prefer ~6–8 sides for tubes/reels and flat beveled cases initially, then inspect up close in a 3D editor.
6. Produce separate game mesh, coarse collision/pickup shape, minimal material/texture derivative, and inventory/shop icon render. Test silhouette legibility at hand, table, and store shelf distances. Never show the whole sheet in place of item icon.
7. Check level-to-level variation visually and functionally. The generated posters sometimes include a reel attached to a rod; **do not merge distinct purchased rod and reel tracks**. Keep modular equip slots, avoid duplicated/replaced line spools and equipped tackle.
8. Publish model/source files to version control *only after* the physical geometry and downstream generated assets exist, then link actual mesh/texture/icon paths to the corresponding stable asset IDs; do not claim shipped models based on this catalog.

## Model families and behavior

**Rod (13 variants):** tapered shaft, grip, rings/guides, optional reel seat. Held in right/left hand as configured by gameplay; casts and reel animation share the same object and `line_exit` pivot. Rod 00 is the starter, 01–12 are purchasable.

**Reel (11):** attach at rod reel seat; spun crank, spool and bail/drag are modeled components. Select model from the reel equipment level (00–10).

**Line (11):** a real shelf/spool object, plus visually differentiated line material/strand when equipped; not a static card. Spool or case exists as a physical pickup/store object (00–10).

**Lures (9):** individual hooks/spoons/flies/weights arranged into pouch or tackle box. Boxes have hinges and compartments; player-facing previews do not imply all tiny hook physics are enabled. Equip lure at `lure_eye` (00–08).

**Beach kits (9):** bag/net/bucket/jar/lens tools as physically present. Crab sightings are *observations*, not permission or mechanics to harvest live crabs. Site positions are anchored to geographic shoreline; specimen placement may be game-generated and is not asserted factual (00–08).

**Skiff hulls (6 pictured):** hull has deck, transom, benches, seating/storage mounts, railings, propeller/helm fittings. Existing runtime save represents four hull tiers, not six; adding two tiers requires separate code/save migration, balance and ship-waterline testing.

**Helm and outboard (9 pictured):** one concept board covers several *slots* (manual tiller, motor, console, sonar, advanced navigation). Do NOT treat the nine pictures as nine linear `helm_upgrades`: current field is a fittings bitmask. A future slot manifest should map each model to real `HelmProgress` fittings and optional new slots.

**Catch storage (9 pictured):** world-facing stringer, creel, crate, cooler, ice/workstation. Storage capacity and durability game systems are *not implemented yet*. The illustrated variants are design references until capacity/inventory rules are approved and built.

**Fishfinder/navigation (8 pictured):** the sonar display must consume the *existing* Sitka bathymetry/sounder validity and tagged procedural infill. Conceptual screenshots in source sheets are not actual readings, chart data or safe navigation guidance. The equipment upgrade track and installation UI remain design work.

## Icon and information contract

Every `asset_id` maps to exactly one shop/inventory information record when implemented: display name, upgrade level, physical description, price, beach discovery prerequisites, gameplay effect, model revision, icon revision and availability. Source current price and unlock checks from `NextEquipment`; never hardcode an independent store formula. For base equipment, show its starter state explicitly. If a category is not implemented, label it **Concept / unavailable**, rather than exposing purchasable buttons that do nothing.

For icons: render the **actual approved model** in an orthographic camera on a transparent background at 64×64 or 96×96 and review pixel legibility, silhouette and contrast. Separate icon artwork from 3D source; no inventing objects that the equipment mesh lacks.

## Integration and validation

Initial rough targets (tunable in benchmarks, **not hard limits**): rods 100–300 triangles, reels 100–300, spools/lures 40–200, kits 150–500, skiff hull and fitted assembly 400–1600, consoles/electronics 150–600. Reuse primitive instances and texture atlases when it actually reduces GL batches. Check stereo headset scale, pickup/hand orientation, Android GLES2 compatibility, animated joints, actual store purchase flows, and Galaxy S9 performance. Observe current release-blocking policy: build/runtime failures block, cosmetic/LOD/detail diagnostics are reported separately.

Do not derive actual Sitka coastline, docks, buildings, crab populations or fishing regulations from AI imagery.
