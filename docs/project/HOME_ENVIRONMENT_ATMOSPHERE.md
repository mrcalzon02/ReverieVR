# VR Home — environmental art and audio

**Implementation status (15 October 2026): source committed, handset acceptance pending.**
Canonical files: `HomeAtmosphereGeometry.java`, `HomeEnvironmentRenderer.java`,
`HomeAmbientAudio.java`, `VrShellRenderer.java`, `VrActivity.java`.
The home scene remains native GLES2 with one APK and no network runtime.

## Current implemented slice

| Scene | Visual layers | Environmental playback |
|---|---|---|
| Birch Panel Living Room | Two actual rear-wall window apertures with recessed scenic sky/hills, mullions and sills; room furniture and original mirror untouched | Low-level steady original HVAC/room-hum PCM |
| Pastoral Forest Glade | Three-band four-sided sky panorama, cloud groups, warm sun, distant hills, rabbits, one deer, two independently moving bird silhouettes | Original synthesized leaves/wind and intermittent chirps |
| Windswept Dune Beach | Three-band coastal sky, cloud groups, sun, low headland on one side of horizon, crabs, two moving birds | Original synthesized surf/wind and gull-like calls |

Meshes are built once and reuse existing uniform-color GLES2 rendering. The sky curtain
fits the 40 m far plane, and the bird clock advances once per Cardboard frame, not once
per eye. World MVP is restored after the bird pass. Wildlife does **not** use heavy
skinning or AI. Interior windows have **genuine mesh apertures** with opaque low-poly
landscape backing behind them; they are not portals into another simulated
world or real-world passthrough. There is no external video feed, weather API, AI scene
analysis, or runtime texture generation.

The 22,050 Hz mono, 4-second PCM16 AudioTrack loop occupies 176,400 bytes for the
selected environment. It is generated only on a change of scenery and loops in
Android's audio path without per-frame synthesis. It starts only in Home or the
environment chooser, pauses elsewhere or when Android suspends VR, and releases
on scene change or Activity destroy. The intended low gain preserves the independently
controlled packaged music. Original synthesized sounds are not field recordings.

## Verified CC0 environmental sound sources — candidate intake only

The following are identifiable real recordings, *not* APK contents. CC0 status was
checked on the linked Freesound item pages. They are suitable candidates for a later
asset pass after downloading the exact bytes and reviewing their audible content:

| Target | Author and recording | Origin / stated license | Source dimensions |
|---|---|---|---|
| Forest bird bed | Snudio_Records — **Forest Bird Sounds** | https://freesound.org/people/Snudio_Records/sounds/267491/ — CC0 | OGG, 59.6 s, 3.4 MB stereo |
| Short forest bird cue | fleurescence — **birds chirping in a forest** | https://freesound.org/people/fleurescence/sounds/578858/ — CC0 | WAV, 14.4 s, 5.3 MB stereo |
| Beach, sea and gulls | onderwish — **Artificial Beach & Seagulls** | https://freesound.org/people/onderwish/sounds/465901/ — CC0 | MP3, 33.0 s, 1.3 MB stereo; synthesized source, not a field recording |
| Real shoreline alternate | Sadiquecat — **windy Canet sea ambience** | https://freesound.org/people/Sadiquecat/sounds/796011/ — CC0 | OGG, 137 s, 8.0 MB stereo |
| Indoor ventilation | sonically_sound — **Air Conditioner** | https://freesound.org/people/sonically_sound/sounds/625224/ — CC0 | WAV, 28.8 s, 5.3 MB stereo |

**Asset gate:** Do not claim a recording is included until its downloaded source
bytes, author, license, hash, trimmed/converted output hash, credit and file size
are recorded in the repository alongside the asset, and an installed APK proves
playback offline. Re-encode small low-gain mono OGG assets when appropriate and
limit layering to the Galaxy S9 thermal/audio budget. No new license gate is
added to runtime: this is straightforward provenance documentation.

## Regression and acceptance

The checked-in `HomeAtmosphereGeometryTest` covers finite triangle data,
40 m sky bounds, scenic window depth ordering, creature/bird mesh budgets,
deterministic birds, 4-second loop duration, exact join silence, and distinct
environment beds.

Still required before device acceptance:

1. Run `./gradlew :app:testDebugUnitTest` or the current working Gradle equivalent
   and build the signed Android APK; verify both ARM ABIs if applicable.
2. Enter and revisit all three environments on Samsung Galaxy S9 + Daydream View;
   check left/right eyes match, window occlusion, legibility, bird velocity/scale,
   distant terrain, pointer hit targets, recenter and room transition.
3. Listen in-headset with existing lobby music, Quick Menu, DOS, native games and
   Activity pause/resume; verify no ambient bleed or clicks and no UI-thread stall.
4. Record sustained render/frame and thermal profiles with the additional
   geometry. If the device budget degrades, remove excess geometry, not game logic.

This implementation does not claim rendered screenshots, synthesized audio quality,
signed APK packaging, or the physical headset test has been completed.
