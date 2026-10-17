# Breakwater: rear landscape grounding and directional day/night lighting

**Date:** 2026-10-17  
**Authority:** mrcalzon02/ReverieVR, main, existing native C++17/GLES2 renderer.

## Executed
- Added `RearGroundHeight(const EnvironmentProfile&,x,z)` to the game-private landscape header. It reproduces the renderer's 12 ground strips and their split-triangle diagonal, and roots Shingle Bay trunks/track, Iron Quay sheds/chimneys, and Blackcap basalt bases in that surface. No collisions, unit paths, saves or gameplay logic changed.
- Added `IlluminateFacet` to compute cheap sun-versus-moon diffuse reflection from actual world triangle edges and face normals. The renderer now shades landscape, beach, shore cover, gun hardware, vessels and vehicles in its existing shared scene mesh. Two separate material modes prevent already-night-colored landscape palettes being multiplied by the actor night tint again. Sky, ocean, muzzle hits and UI do not run through this surface-light path.
- Strengthened `breakwater_environment_test.cpp` to check both barycentric regions of the ground strip, slopes across each campaign location, exact geometry budget, scene-prop bases, world-color tint, and day/night directional response. Existing `scripts/verify-breakwater-visuals.sh` already runs that test on machines with the complete repo and compilers.

## Evidence and limits
- Changes were written through GitHub Git-data objects directly onto `main` with non-force, expected-parent updates; remote readback supplies final acceptance.
- Executed isolated GCC and Clang C++17 `-Wall -Wextra -Wpedantic -Werror` host programs for manually **mirrored formulas**, plus Clang ASan/UBSan for the light formula. They are not evidence of compiling the precise GitHub header or fully linked module.
- Full repository test runner, native Android build, APK, headset 360 look/comfort, per-eye visibility, and Galaxy S9 GPU/thermal performance remain unverified.
- Neither the global Home UI nor `Mars des Souvenirs` assignment nor GitHub Actions were changed.

**Next gate:** run the checked-in visual script and Breakwater campaign suite from a full local checkout, then perform visual and frame-time testing on the actual Daydream Galaxy S9.
