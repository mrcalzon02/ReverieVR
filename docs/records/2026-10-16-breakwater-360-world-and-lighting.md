# Breakwater: 360-degree campaigns, physical rear landscape and day/night lighting

**Date:** 2026-10-16  
**Repository:** `mrcalzon02/ReverieVR`, `main` only  
**Execution state:** committed on GitHub; native Android/GLES2/Galaxy S9 runtime acceptance outstanding.

## What changed

The live source already contained a 360-degree sky shell, sun/moon and cloud panorama, and shallow forward beach strips. Its missing component was a place to look at behind the battery: ground, hills, mountains, and recognizable campaign identities. The new game-private `breakwater_environment.h` adds two continuous 48-sector ring layers inside the sky radius (17 and 22 world units) and a joined 12-strip rear-ground slope. The forward ridges remain low to protect ship lines of sight. The near and far rings close at 360 degrees with deterministic contours and no runtime random allocation.

The landscape distinguishes **Shingle Bay** (green hills, sparse pines, rear service track), **Iron Quay** (concrete supply buildings, slate hills, angular dock shoulders), and **Blackcap Island** (tall volcanic ridges, basalt fingers and rough offshore shoulders). `EmitCoastalTerrain` retains all gameplay hitboxes and troop lanes while giving every coastline its own raised outer shoulders and stronger color distinction.

The original sky gradient was adjusted from muted day tones to bright blue/daylight colors, and from near-grey dusk to deep navy at night. Sea water derives its RGB from the new per-location profile. `BuildScene` now emits the full environment before game targets and applies `IlluminateWorld` to real combat-world vertex colors, while preserving readable HUD text, UI panels, hit pulses, sound/muzzle flashes and stereo consistency. No dynamic per-pixel lights were introduced; this is true time-of-period world-color illumination at scene-build time, appropriate for the GLES2 reference device.

## Acceptance evidence and limits

- **Remote source and tests:** GitHub main commits accepted via optimistic non-force ref updates and subsequent remote readback. Includes `breakwater_environment_test.cpp` in the existing visual runner; no GitHub Actions.
- **Executed isolated native geometry checks:** GCC and Clang `-std=c++17 -O1 -Wall -Wextra -Wpedantic -Werror` passed. Clang `-fsanitize=address,undefined` also passed. The header checked matched Git blob `559fbaafd19c6fc3c77ad824b6dc459a2521a1c0` byte-for-byte, and a minimal stand-in was used for Campaign/sky type declarations. Checks covered all three profiles, both periods, finite bounded emitter geometry, wrap-around ridge seam, front/rear rise, invalid locations and world-color attenuation.
- **Still not run:** Full original project visual runner against a complete checkout, complete `breakwater_campaign_test`, Android NDK/GLES2 build, APK installation, 360 head look in a Galaxy S9, frame/thermal tests. No assertions of those outcomes are made.
- **Preserved:** 80-byte save envelope, cannon-origin targeting, existing mounts, game progression and wave system, `Mars des Souvenirs`, global ReverieVR Home menu.

## Next device check

On Galaxy S9, turn fully behind the battery in each location both day and night. Confirm no gap between sky/hills, no mountains occlude enemy approach, no doubled geometry or eyes disagreeing, bright day versus deep blue night in gun and world geometry, and no regression in stereo frame time.

**Seam closure correction:** Rear ground strip length is 1.45 world units (12 strips from z=-0.50 to z=16.90), meeting the near hill ring at radius 17. The original 1.15-unit length stopped at z=13.30 and risked a visible band of empty scenery when looking behind. The geometry regression now verifies the rear ground reaches the hill ring within 0.20 units. Isolated GCC, Clang and Clang ASan/UBSan tests passed after the correction; headset proof remains outstanding.
