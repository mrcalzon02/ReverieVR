# Sitka

**Official title:** Sitka (no subtitle).  
**Proposed project ID:** RV-GAME-SITKA; tentative module ID `sitka`.  
**Category:** ReverieVR > Native Apps / Projects, once functional.  
**Stage:** Source-only native terrain rendering proof integrated; no real Sitka geographic content or playable VR game.  
**Platforms:** Inherits the full ReverieVR host targets (Android Daydream, native Windows/Linux/macOS desktop, and provisional OpenXR on Windows/Linux); Daydream Galaxy S9 remains the 30 FPS reference.  
**Reference:** Owner-supplied overview image of Sitka Sound, 2026-10-26.

## Current authoring milestone — 2026-10-26

**Source integrated on ReverieVR `main`, not a playable game:** A separate desktop [Sitka editor and terrain pipeline](SITKA_EDITOR_PIPELINE.md) has been added. The editor uses the genuine Beckenburg Java 17 editor palette/water controls and the same independent weather module consumed by Malefic Veil. A deterministic offline DEM+shoreline-to-low-poly terrain baker requires independently supplied, appropriately licensed geographic datasets; the Python synthetic-fixture regression passed. A game-private C++17 water/weather data adapter and host regression passed GCC/Clang sanitizer checks. Real Sitka geographic tiles, desktop GUI compile/preview, GLES2 terrain/water rendering, native game registration, APK and S9 headset execution remain **unverified and unimplemented**. Do not equate authored profiles with visual rendering, or expose an inert app icon.

## Terrain rendering source proof — 2026-10-27

The original `.sitkam` binary contract now has a strict native C++17 reader and
GLES2-safe 16-bit triangle batching. The game-private terrain renderer uses the
existing native shader/matrix/GL-state helpers, immutable indexed VBO/EBO uploads
once per GL context, and the same buffers for both eyes. A source-only
`reverie_module_sitka` NDK target consumes a bounded host `read_asset` mesh
and rejects missing/corrupt content. **It is deliberately not registered in
the host's playable module catalog and has no launcher control**: no verified
licensed Sitka mesh has been staged, and no fishing/shore gameplay exists.

The four-vertex synthetic `.sitkam` fixture and >65,535-vertex chunk test
passed GCC C++17 and Clang ASan/UBSan host execution; a fake-GLES2
lifecycle test exercised static uploads, two eye draws and context rebuild.
Those are CPU/mock checks, **not** Android NDK, real GL driver, APK, S9,
geographic or playable-game acceptance. See [terrain pipeline](SITKA_EDITOR_PIPELINE.md).

## Identity

A gentle, relaxing VR fishing and beachcombing game along the real coast of Sitka, Alaska. Visit recognizable islands, beaches, harbors and coves. Fish, explore tidal pools, spot crabs and collect rare shells. Upgrade rods, reels, bait, tackle and boats to access new places. There is no combat, countdown, punitive failure, forced chores or grinding requirement. Quiet exploration must itself be enjoyable.

**Loop:** choose coastal destination on nautical-style chart -> travel by shore or boat -> explore from comfortable stationary/teleport viewpoints -> cast and reel, beachcomb, observe marine wildlife -> document discoveries in a field journal -> return to dock -> upgrade tackle or vessel -> explore farther.

The landscape is geographically grounded but stylized in a beautiful low-polygon PSX-inspired aesthetic using techniques learned from The Beckenburg Project and The Malefic Veil. Those projects' desktop renderers are **not** assumed compatible with ReverieVR GLES2; transfer suitable mesh, occlusion, material, LOD and caching techniques deliberately.

## Host platform inheritance

Sitka is a native game hosted **inside the complete ReverieVR platform**. Its Windows, Linux, macOS, Android Daydream and optional OpenXR target modes derive from the **whole-project** [desktop expansion](DESKTOP_PLATFORM_EXPANSION.md), not from a separately planned Sitka-only desktop application. Game-specific environmental and fishing rules remain here; hub/renderer/OS delivery responsibilities belong to the platform plan.

## Coastal-world mapping

Use real Sitka Sound geography, with high priority on coastline contours, relative island and headland positions, harbor silhouettes, mountain profiles, coves and forested slopes. Author varied shoreline pockets in the appropriate positions on a **regional chart**, rather than a fabricated island map. The player's supplied map is a geographic brief, not redistributable game terrain.

Potential initial destinations to survey: Crescent Bay, Starrigavan Bay, Halibut Point and the Indian River outlet, all identified as Sitka-area shoreline fishing locations by Alaska Department of Fish and Game. Evaluate geographic accuracy, public access, local data resolution and game suitability before selecting a first playable pair.

### Mapping source requirement and rights

Owner requested Google Earth accuracy. **Do not scrape, trace, reconstruct, extract or cache Google Earth/Maps imagery, meshes, terrain or geodata to create offline game assets**. Google prohibits using Earth output to rebuild 3D models or similar content, and Google's Map Tiles API prohibits geodata extraction and offline caching beyond its terms. Any optional live Google Maps Platform integration must be independently licensed, network-aware and attributed, and cannot be an offline low-poly data pipeline. Reference: https://about.google/brand-resource-center/products-and-services/geo-guidelines/ and https://developers.google.com/maps/documentation/tile/policies .

Build accurate offline game terrain using properly sourced downloadable data instead:

1. **USGS 5 m Alaska IfSAR/3DEP** elevation for ridgelines and headland shapes: https://www.usgs.gov/ngp-user-engagement-office/alaska-mapping-initiative
2. **NOAA coastal/shoreline and bathymetry products** for coastline, seabed and shore gradients where available, respecting source coverage and disclaimers: https://www.ncei.noaa.gov/products/bathymetry
3. **OpenStreetMap** geographic features where useful under ODbL attribution/database obligations: https://www.openstreetmap.org/copyright

For every ingested source record download source, version/date, attribution/license, tile coverage, CRS/vertical datum, checksum, generated recipe and derived outputs. Transform to bounded local coordinates, triangulate and decimate terrain offline, build independently licensed texture/mesh atlases, blend seam boundaries, and package local data for offline play. Validate coastline-to-elevation alignment and tide/shoreline approximation. This is artistic geographic reconstruction, **not** marine navigation.

## Activities

### Fishing
- 3DoF-friendly controller ray aims and click casts; reeling and drag/tension use a simple trackpad gesture or held action with accessibility alternatives. No tracked arm translation required.
- Realistic species candidates: salmon (king/coho/pink), Dolly Varden, halibut, lingcod, rockfish and flounder. Season, water depth, habitats, tide, bait/lure and weather mildly affect odds without harsh fail states. The ADF&G confirms such Sitka fish communities: https://www.adfg.alaska.gov/index.cfm?adfg=ByAreaSoutheastSitka.fishingOpps&fishery=Bottomfish
- Cast -> subtle nibble cue -> engage -> reel within forgiving tension -> land and identify -> decide to retain/release where appropriate -> journal entry, sale or upgrade reward.
- Deterministic seeded fishing simulation, limited active fish, rare encounters with meaningful discoverability; catalog includes species, size records, locations and favorite lure.
- Do not treat virtual harvest rules as actual regulatory advice. If featuring deepwater rockfish release, respect descending-device conservation guidance.

### Shores
- Walk/teleport between clearly accessible beach pockets; find unusual shells, beach stones, fossils, driftwood and discoveries in tide pools.
- See and optionally gently inspect crabs, kelp, starfish and shoreline animals. Ambient gulls, eagles and seals enliven the environment without chase objectives.
- Rare discoveries use regional biome and tide context, not arbitrary glitter collectibles. Shoreline collections and field-journal photos can be displayed at the home dock.

### Upgrades and boat
- Rods, reels, line, hooks, baits, lures, tackle storage and convenience gear have meaningful differences.
- Vessel candidates: starter small skiff or rowboat -> modest motor skiff -> compact cabin boat. Boats extend reachable chart destinations and provide safe/comfortable storage and calmer operation, not mandatory speedrunning.
- Dock has functional boat berth, chart, tackle rack, equipment store, aquarium/collection display (where appropriate), journal, departure and return controls.
- No progress loss for an unproductive outing.

## Comfort and presentation

- Start in seated comfort mode. Prefer destination selection, shore-point teleport and optional gentle boat transit. Do not impose head-bobbing, deck-to-camera bob, forced roll/yaw, surprise acceleration or weather-related nausea.
- Smooth sailing is optional, with snap turns, adjustable comfort vignette, motion intensity and instant return-to-dock.
- Audio is a major feature: swash/surf, light wind through conifers, birds, distant rain, rod and reel, footsteps over gravel, water droplets and skiff motor; support quiet mode and independently adjustable audio.
- Season, time of day, mist, rain and wind should change color, behavior and ambience; initial slice may use handcrafted presets rather than live weather.
- One simulation update and two eye renders, shell-owned input/recenter/Quick Menu/exit/HUD, GL context lifecycle, isolated versioned saves. Do not let a live controller occasionally activate headset gaze-select beams. Controller loss preserves recovery without accidental casts.
- Low-poly GPU terrain with immutable indexed static buffers, offline decimated terrain and LOD meshes, baked light/vertex color, limited alpha, atmospheric fog, sky and far-shore impostors, tiny animated surf strips, seeded compact wildlife, cached shader programs. No per-eye geometry rebuild, no per-frame terrain upload, no expensive water reflections or dynamic shadow cascades. Profile 30 FPS and thermal performance on the actual S9.
- All in-world signage, species names and menus must use real readable font assets/glyphs; no substitute pseudo-letter geometry.

## Phased delivery (future work, not authorized by this proposal)

**A — Geographic proof:** select one small cove; document legal source data, coordinate conversion, accurate coarse terrain/shoreline meshes, reproducible chunk/LOD generator.

**B — Environment proof:** generate two related exploration pockets; shoreline, foliage, atmospheric audio, sea surface, stable comfortable headset view and measured stereo/thermal baseline.

**C — Fishing:** one real rod, two or three species, cast/bite/reel/land, identification and persistence. No decorative fake fishing mechanics.

**D — Beach discoveries:** shells/rare finds, moving crab with pickup or examine, field journal and persistent collection.

**E — Boat and gear:** launch/return, two destinations, an actual upgrade purchase affecting play, persistence/save compatibility.

**F — Integration and acceptance:** Android module descriptor and versioned asset package, Native Apps / Projects registration **only when functioning**, native C++/NDK/APK checks, S9 physical launch, 30 FPS thermal/frame pacing, audio, shell/exit/reconnect, save/load and update-delivery behavior.

**Minimum honest playable acceptance (at each actually supported platform):** launch Sitka from ReverieVR; visit two recognizable Sitka shoreline destinations; cast and land a fish; inspect a crab or rare shell; upgrade tackle; depart and return; save/exit/relaunch to intact progress; hear environmental audio; no loss of shell recovery. If a feature cannot perform, it remains a design entry rather than an inert button.

## Project separation and scope

Sitka is an **independent native game concept** beside Breakwater Battery, Gumball Armada, Red Ledger, Omega Front and the other ReverieVR native modules—not a Breakwater mode, Classic Game or ROM. It inherits platform runtime/render/input/delivery and optional PC XR capabilities from the **ReverieVR hub as a whole**; it is not a separately scoped desktop product or SteamVR integration. Sitka requires its own module ID, state, assets, packaging and update manifest when operational. The project name remains **Sitka**.

This record specifies the Sitka game and documents its **source-integrated authoring pipeline only**. It does not claim an independent GitHub repository, a surveyed Sitka mesh, a working GLES2 renderer, an executable Sitka module, installed game, published APK or headset validation. Continue implementation only within the user-authorized environment milestone and explicit next work.
