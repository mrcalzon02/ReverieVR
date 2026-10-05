# ADR-0001 — Performance-First Retro VR Budget

**Status:** Accepted  
**Date:** 2026-10-04

## Context

Phone-based VR renders separate eye views while also handling tracking, input, application logic, audio/video, and display presentation inside an enclosed headset. Chasing high graphical fidelity on older mobile hardware can consume thermal and power headroom, cause throttling, destabilize frame pacing, and worsen comfort.

The project explicitly prefers useful sustained VR over graphical spectacle.

## Decision

ReverieVR adopts a **performance-first, retro-first** rendering philosophy.

The platform and bundled experiences should begin from deliberately simple scene complexity comparable in spirit to Doom/early-3D-era presentation and add visual cost only when measured reference-device headroom justifies it.

The project prioritizes:

1. stable frame pacing;
2. low tracking/input latency;
3. thermal sustainability;
4. legibility and comfort;
5. battery/runtime practicality;
6. graphical fidelity after the above are protected.

Default techniques include simple geometry, conservative materials, baked/static lighting, limited transparency, culling, LOD where useful, restrained texture sizes, minimal post-processing, and adjustable/dynamic render scale.

## Consequences

- Art direction should make low complexity intentional rather than apologetic.
- Performance instrumentation is a foundation feature.
- Every module receives a performance budget.
- Visually impressive features can be rejected if sustained device behavior is poor.
- Reference-device sustained tests are required before claiming performance acceptance.
- Exact numeric budgets are not guessed in advance; RV-0301 will derive thresholds from measured Galaxy S9 behavior.
