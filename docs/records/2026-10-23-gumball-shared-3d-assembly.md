# Gumball Armada — Shared 3D assembly and scrolling combat slice
**Date:** 2026-10-23
**Repository:** `mrcalzon02/ReverieVR`, `main`
**Scope:** Gumball Armada native module only; game-specific support headers, native regressions, and project records. No new native host API, Home menu, release workflow, or unrelated games.

## Authority and goal
Replace the existing flat 7x7 edit-and-display arrangement with a functioning depth-layer editor and one 3D component system that produces both the player vessel and Sour enemies. Retain combat, currency, controller steering, and save continuity rather than disguising a different game behind new art.

## Source implementation
- `gumball_ship_3d.h`: 7x7x5 module volume; fixed central core; six-direction connectivity; inventory-conserving atomic placements/swaps/removals; real mass, hull, shield, mount, power, thrust calculations; launch readiness; checked 268-byte `Ship3DSave`; exact migration from the validated 72-byte `ShipyardSave` by placing prior X/Y cells in Z=2.
- `ShipAssembly::enemy(kind,salt)`: deterministic connected hull patterns for 13 actual enemy types, built from the SAME part vocabulary and geometry as the player. Heavy blueprints receive required attached thrust/power modules to meet the same readiness rules.
- `AppendAssemblyMesh`: actual six-faced 3D colored voxel geometry with hidden internal faces removed, yaw rotation, authored part palettes and directional shading. The module uses it for both player and live Sour ships, not flat sprites or separate enemy billboard geometry.
- `gumball_space_3d.h`: perspective-positioned scenery and real Z-scrolling lane markers/debris; projectile solids in the established six-float position/color vertex format.
- Live native module: Shipyard offers Z slice down/up, full 3D preview, rotate; pad rotates preview and changes zoom, with primary exiting preview. Existing pointer hit geometry and cursor focus covers all 14 palette targets. Player edits update the real combat profile, save slot, and the flight model immediately.
- `shipyard.v2` preserves older `shipyard.v1` on migration; failure/corruption is reported and falls back to a safe default, never silently rolls a corrupt V2 back to an old build. Campaign V3 saves remain unchanged.

## Controls
In **Shipyard**, select a part from the right shelf and click a cell in the currently selected depth slice; `SLICE - / SLICE +` edits Z. `3D PREVIEW` shows the full assembled ship. While previewing, left/right rotates and up/down zooms; primary exits to editing. `ROTATE 45` opens the preview in a new orientation. App/Minus still switches between grid and palette outside preview.

In **flight**, the Daydream tracked-controller ray retains direct target steering and touchpad click retains firing. The native render displays the built player ship and deterministic enemy constructions in perspective along a forward-moving 3D candy lane. Enemy AI, projectile hit logic, mission economy, escorts, and saves continue through the existing real simulation; collision/gameplay coordinates remain two-dimensional for this slice.

## Verification contract / open gates
- Source files committed on remote `main`; connector readback required after each change.
- `gumball_ship_3d_test.cpp` covers migration, three-dimensional connection, inventory conservation, v2 checksum and corruption, deterministic enemy builds and generated mesh, and moving perspective lane geometry.
- Existing `verify-gumball-armada.sh` includes the new test in GCC, Clang and Clang sanitizers. **This record is not evidence that the script was executed**.
- **Open:** host test execution against full exact checkout, GLES2/NDK link, Gradle asset/APK packaging, actual stereoscopic render and model visibility, Galaxy S9/Daydream controller/menu ergonomics, framerate/thermal acceptance.
- **Architectural limitations:** an early 3D world presentation is committed, not final 3D flight dynamics, collision volumes, full component rotation/wedge library, freely orbiting camera rig, or a complete traversable galaxy. The color-mesh renderer uses back-to-front draw ordering instead of an independently depth-tested 3D geometry pass; examine occlusion and performance on a real headset before promotion.
- No GitHub Actions requested or run in this slice.
