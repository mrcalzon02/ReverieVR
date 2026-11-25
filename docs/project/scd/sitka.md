# Sitka — completion guide

**Project ID:** `sitka` • game • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
Registered 17th native module; survey over NOAA/USGS data and separate fictional practice bay support shore discoveries and gear. Actual walkable harbor/boating route not yet proven.

## Authority and entry points
- `docs/project/SITKA.md`
- `docs/project/SITKA_CITY_WORLD_DESIGN_SPEC.md`
- `docs/project/SITKA_GIS_TILING.md`
- `docs/project/SITKA_BATHYMETRY_AND_BOAT_SPEC.md`
- `docs/project/SITKA_WATERFRONT_EXPORT.md`
- Production anchor: `app/src/main/jni/native/sitka_terrain_module.cpp`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/sitka/verify_gameplay.py`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/sitka/stage_native_land_assets.py`

## Ordered gates
### G01 — Establish a real geographic playable berth and walking route

**Implementation:** Reconcile source-derived NOAA chart land, USGS terrain, actual shoreline, bathymetry and vertical datum. Stream bounded tiles; construct authentic harbor/office access, walk collision and safe spawn. Never label fictional practice geometry as actual Sitka.

**Does it work?** Walk from surveyed harbor berth to office and shore fishing location on actual source-derived mesh, with consistent coordinates and no shore seam, falling through ground or land/water alias. Missing datum/bathymetry must be explicitly blocked rather than fabricated.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Connect Tier 0 harbor economy to actual terrain

**Implementation:** Place NPC harbor master, gear inventory, catch/sell/credit and upgrade actions into accessible real locations. Keep actual fishing and inventory state isolated from survey diagnostics.

**Does it work?** From fresh save, acquire starter gear, fish, sell, purchase upgrade, identify and register a find, save/reopen; all credits and items reconcile once.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Integrate the three boat classes and all intended upgrade tiers

**Implementation:** Use the accepted 18 model variants and actual hull/geometry contracts. Implement boarding, steering, throttle, buoyancy/collision, docking, storage, equipment and crab-pot operations. Correct contiguous hull, decking penetration and waterline; use approved assets.

**Does it work?** Each boat can leave berth, steer, stop, return, dock and exit; cargo and fuel if specified persist; upgrade has measurable intended effect. Gear interaction and crab-pot cycle work. Show genuine 360-degree production-asset captures and runtime waterline.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G04 — Expand connected city and harbor activities

**Implementation:** Implement multiple harbor offices, route navigation, irregular buildings, walkways, regional shore specimens and economy progression using approved actual geography and assets.

**Does it work?** Traverse at least two distinct real harbors and their offices; complete fishing/shore/crabbing and upgrade routes from valid spawns; no fictional map presented as verified geography.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G05 — Verify whole-game playability and scale

**Implementation:** Measure tile resident-set, uploads per frame, frame times and memory in a sustained trip; integrate one APK and preserve saves.

**Does it work?** Finish a legitimate starter-to-upgraded boat route through installed game and reference hardware; stable control, navigation, save/recovery and bounded streaming.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
