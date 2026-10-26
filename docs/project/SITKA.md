# Sitka

**Official title:** Sitka (no subtitle).  
**Proposed project ID:** RV-GAME-SITKA; tentative module ID `sitka`.  
**Category:** ReverieVR > Native Apps / Projects, once functional.  
**Stage:** Proposal/design only — not implemented or downloadable.  
**Platforms:** Android Daydream VR (reference Samsung Galaxy S9, Daydream View/controller, 30 FPS sustained); native Windows, Linux and macOS desktop; provisional PC VR through OpenXR/SteamVR on supported Windows/Linux configurations.  
**Reference:** Owner-supplied overview image of Sitka Sound, 2026-10-26.

## Identity

A gentle, relaxing VR fishing and beachcombing game along the real coast of Sitka, Alaska. Visit recognizable islands, beaches, harbors and coves. Fish, explore tidal pools, spot crabs and collect rare shells. Upgrade rods, reels, bait, tackle and boats to access new places. There is no combat, countdown, punitive failure, forced chores or grinding requirement. Quiet exploration must itself be enjoyable.

**Loop:** choose coastal destination on nautical-style chart -> travel by shore or boat -> explore from comfortable stationary/teleport viewpoints -> cast and reel, beachcomb, observe marine wildlife -> document discoveries in a field journal -> return to dock -> upgrade tackle or vessel -> explore farther.

The landscape is geographically grounded but stylized in a beautiful low-polygon PSX-inspired aesthetic using techniques learned from The Beckenburg Project and The Malefic Veil. Those projects' desktop renderers are **not** assumed compatible with ReverieVR GLES2; transfer suitable mesh, occlusion, material, LOD and caching techniques deliberately.

## Cross-platform delivery and play modes

**Product requirement (design target, not an implemented feature):** Sitka is one game with the same coastline, fishing, beachcombing, discoveries, gear progression, boat upgrades and versioned save data across supported platforms. Android Daydream remains the reference/lowest-budget renderer; desktop play is first-class, not a headset simulator or an Android emulator.

| Target | Required experience | XR status |
| --- | --- | --- |
| Android APK | Existing seated Daydream 3DoF path hosted by ReverieVR, including shell Quick Menu/recenter/recovery | Reference VR target |
| Windows x64 | Native windowed/full-screen desktop game, keyboard/mouse and gamepad | Provisional PC VR using OpenXR with SteamVR as a runtime where available |
| Linux x86_64 | Native windowed/full-screen desktop game, keyboard/mouse and gamepad | Provisional OpenXR PC VR using supported SteamVR or Monado runtimes |
| macOS (Apple Silicon priority; Intel evaluated separately) | Native windowed/full-screen desktop game, keyboard/mouse and gamepad | No SteamVR requirement; optional future XR only after viable runtime/device validation |

SteamVR ended official macOS support in 2020. Do not advertise SteamVR on macOS, assume an OpenXR runtime exists, or make an XR headset mandatory for a desktop build.

### Engineering boundary

- Keep fishing rules, world streaming/LOD decisions, wildlife, weather/tide simulation, upgrades, journal, progression and serialization in a portable deterministic native game core. Adapt the existing ReverieVR native-game module/ABI as required by an explicit design review; do **not** assume that an Android .so or the Android host application can be copied unchanged to Windows, Linux or macOS.
- Provide thin platform hosts: Android/ReverieVR Daydream; Windows desktop; Linux desktop; macOS desktop; and an optional PC OpenXR adapter. Isolate OS window creation, graphics context, input transport, audio devices, filesystem, update delivery, headset tracking, frame submission and controller discovery from game rules.
- Preserve a common asset source and legal/source provenance records, with platform-specific shader compilation, texture formats, meshes/LOD budgets and packaging. Reuse Beckenburg/Malefic Veil's editor, water, terrain and weather *techniques* where compatible; their Java renderer and tools are not assumed to run unmodified inside Android or desktop Sitka.
- Support flat-screen orbit/look camera, walking and optional comfortable viewpoint teleport; make dock/boat travel, shore interactions, cast/reel/tension, inventory, journal and menus fully operable without head tracking. Desktop UI must use a real readable font, native window resizing, aspect-ratio adaptation, and cursor/gamepad focus handling.
- Map **actions**, not physical buttons: desktop WASD/arrow movement, mouse-look, primary cast/interact, secondary/cancel, wheel/key/gamepad reel and menu actions; configurable bindings and left-handed/accessibility alternatives. VR uses the host-normalized controller/pointer and supports supported OpenXR controller profiles without assuming a specific 6DoF device.
- PC VR uses OpenXR (SteamVR is a selectable runtime, not a game-engine dependency). Detect loader/runtime/session/controller availability, avoid phantom VR options, and recover cleanly to a standard desktop window when XR is absent or initialization fails. XR mode must meet headset-native timing and comfort requirements before calling it supported.
- Store portable, versioned saves in normal platform application-data directories. Share schema/content identity and migration checks, but **do not** promise automatic cloud synchronization or silent cross-device transfer. Separate settings for desktop FOV/graphics, Android stereo/thermal and PC headset/runtime preferences.
- Deliver independently packaged platform artifacts (Android APK, Windows package, Linux package and macOS app bundle) and identify platform/architecture in the existing independent-module version/update metadata. A Steam store release or Steamworks integration is not authorized or required by provisional SteamVR compatibility.

### Modes and performance

- **Android Daydream:** seated 3DoF, dual-eye rendering, conservative PSX-style shader/terrain budget, stable reference 30 FPS and sustained S9 thermals.
- **Desktop flat-screen:** one camera, adjustable resolution/quality, windowed/fullscreen, uncapped or configurable frame pacing; initial acceptance target is stable 60 FPS on an explicitly recorded test machine, not a universal hardware guarantee.
- **PC VR (provisional):** render stereo via OpenXR swapchains, pass headset pose/controller actions from the XR host, update simulation once per frame, and target the active device's refresh rate with bounded motion-to-photon latency. No headset-camera bobbing or forced roll.
- The lower-spec Android path must not regress because desktop offers better water, weather, viewing distance or geometry. The same authored shoreline and simulation must remain recognizable across quality settings.

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

**G — Desktop foundation (separately gated):** extract/verify portable simulation and serialization; desktop host window, input/action mapping, graphics/audio/file adapters; source-accurate one-cove scene and fishing loop on Windows, Linux and macOS. Validate native builds and actual interactive launch on each OS, including mouse/keyboard/gamepad and full game-state save/load.

**H — Provisional PC VR (separately gated):** wire OpenXR runtime discovery, optional VR entry/exit and controller actions, stereo submission and runtime-failure fallback for Windows/Linux; measure frame pacing and comfort on actual supported headsets. Do not mark SteamVR compatible merely because an OpenXR library compiles.

**Android minimum honest playable acceptance:** launch Sitka from ReverieVR; visit two recognizable Sitka shoreline destinations; cast and land a fish; inspect a crab or rare shell; upgrade tackle; depart and return; save/exit/relaunch to intact progress; hear environmental audio; no loss of shell recovery. If a feature cannot perform, it remains a design entry rather than an inert button.

**Desktop acceptance additionally requires:** start from the native OS executable with **no** headset or Android host; complete the same fishing/beachcombing/upgrade/save loop with keyboard/mouse or gamepad; verify install/start/exit and restored saves separately on Windows, Linux and macOS. **PC VR acceptance additionally requires** a real supported OpenXR headset/runtime session, stable stereo output, mapped controls and recoverable exit; it is provisional until physically verified.

## Project separation and scope

Sitka is an **independent native application/game concept** beside Breakwater Battery, Gumball Armada, Red Ledger, Omega Front and other native modules—not a Breakwater mode, a Classic Game or a ROM. It will be hosted as a ReverieVR Android native module **and** offered as independently packaged native desktop applications on Windows, Linux and macOS; optional PC VR is an additional mode of the Windows/Linux desktop game, not a separate fork. It should ultimately have its own module ID, state, assets, packaging and update manifest as the host's independent native-module delivery is validated. This is a Sitka-specific scope change; it does **not** by itself declare the whole ReverieVR Android shell desktop-compatible. The project name remains **Sitka**.

This document **only indexes and specifies a proposal**. It does not claim a new GitHub repository, authored meshes, installed game, executable module, source integration, published APK or headset validation, and does not authorize starting those tasks without further direction.
