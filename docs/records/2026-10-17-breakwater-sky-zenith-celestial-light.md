# Breakwater — zenith closure and celestial-aligned illumination

**Date:** 2026-10-17  
**Authority:** `mrcalzon02/ReverieVR` / main.

The earlier 360-degree sky wrapped the horizon but had an open cone directly above the player. An added cap comprises one triangle per existing 16-sector sky wedge from the highest sky ring to a shared overhead vertex (0, 40, 0). The cap's vertex color is the location-tinted top band of the existing day/night sky gradient. It is part of the same static XYZRGB scene batch, with no new shader, dynamic light, bitmap, API, texture, collision or simulation state.

A second correction makes authored world-space surface lighting agree with the actual sun/moon disc position. `SkyCelestialAngle` and `SkyCelestialHeight` define both visual discs, and `KeyLightFor` exposes two fixed near-unit directions based on those positions. Daylight comes from the front-right sky and moonlight from the front-left sky rather than the landward side. Numeric light-vector differences against precise normalized sky directions are <0.003.

The C++17 `breakwater_sky_test.cpp` now checks vertex finiteness, zenith triangle counts, sky placement, day/night contrast, 3 location-dependent plume totals, and invalid location cases. The existing host visual script lists both sky and environment tests, while the environment test also checks sky-disc/light-direction agreement.

**Execution:** all source changes committed to GitHub `main` through non-force optimistic ref updates; source existence and content read back from the remote. Static source checks and an independent JavaScript numeric comparison of sun/moon bearing passed. **Not claimed:** executed C++ native test binaries for this pass, Android build, Galaxy S9 stereoscopic field of view, visual inspection, or sustained FPS. The head-tracked camera, controllers, Home UI, `Mars des Souvenirs`, and save layout were not changed.
