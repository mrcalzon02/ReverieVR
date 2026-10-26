# ReverieVR — launcher concepts archive and truthful adoption roadmap

**Status:** approved *future design references*, **not in-application screenshot evidence**.
**Owner instruction:** 2026-10-25. Keep the five AI-rendered mockups as inspiration, but obtain ACTUAL Android/native/DOS game frames when presenting implementation. Never use the concept renders as test screenshots or falsely advertise games and features.

## Durable artwork archive

Original PNGs retained in existing owner Reverie VR Drive collection, subfolder [Design References — AI Concept Mockups (Not Runtime Captures)](https://drive.google.com/drive/folders/1_sJNrNs-yOWP8PYX9WM86X8bWK_OeAzj). The upload was checked via Drive folder readback; filenames explicitly identify them as concepts.

| Reference | Persistent owner asset | Interpretation |
| --- | --- | --- |
| 01 Native Games launcher | [PNG](https://drive.google.com/file/d/1jxH4xjWC5N_TMxItEI4EnrjlLWGHe-8J/view) | Compact featured/detail panel, launchable game cards, status/signer affordances |
| 02 Classic Games — Ken's Labyrinth concept | [PNG](https://drive.google.com/file/d/1gLqKM_Eulw8t0wWT4DQpw-zJAugH8vsB/view) | Distinct classic-game selection and focus |
| 03 DOOM Shareware concept | [PNG](https://drive.google.com/file/d/1noMgKb5tfuvpsQC53gnlTKKoX7sEKU7s/view) | Retro preview, launch/resume/control information |
| 04 Classic library — DOOM concept | [PNG](https://drive.google.com/file/d/1s4LFRvWL5InoDyvx29TYfksbNqsM7AYq/view) | Dense scrollable library and a larger selected-game focus area |
| 05 Classic Games — Jetpack concept | [PNG](https://drive.google.com/file/d/1zcEcXO70DjNJmOSdDafzwviW0_ppRaEK/view) | Retro platformer presentation and game detail affordances |

These images were generated from prompts, NOT captured by `adb screencap`, `glReadPixels`, the DOS engine, or the Galaxy S9. They may depict nonexistent menus, genre identities, features, UI buttons, arbitrary download sizes/battery readings, and unlicensed art. Do not ship the mockups as game screenshots or put them in an automatic APK asset-reassignment pipeline.

## Correct implementation baseline

The real in-headset UI is `VrShellRenderer`, using world-space Canvas/menu geometry: Home separates **DOS Library** (a five-row scrollable list) from **Native Apps** and **Classic Games**, which currently use a **three-entry paginated native-module list**. The public module registry contains 15 native game identities, including eight `classic-*` games, development-only prototypes and ordinary visible Omega Front; development admission is intentional. The DOS game library is distinct from native `classic-*` titles, and `GameModuleRegistry` must remain the authority for actual availability. None of the concept's buttons or catalog rows prove code/download/playability.

**Incorrect concept content to quarantine:** Breakwater in the live app is a coastal-defense combat experience, not a tranquil fishing/exploration game; the concept's "Explore • Fish • Unwind" is a possible **separate future game idea**, not a silent scope change. The mockups' Commander Keen, Wolfenstein 3D, Prince of Persia, additional DOOM/Ken/Jetpack screenshots, version, game progress, install/download sizes and exact 2D layout are illustrative only. Do not default-install copyrighted titles from an AI mockup. Check exact software/shareware rights and runtime packaging before listing them as launchable.

## Adopt visual structure without regressing Daydream controls

1. **Correct inventory and truth labels first.** Separate: installed native titles, native classic board/arcade games, DOS classic game archives, native development-only previews, and available-for-download modules. Every Launch/Update/Resume/Details control must dispatch to a real capability or be omitted. No ghost install buttons, fabricated download sizes or game state.
2. **Actual capture before visual migration.** Obtain per-game captures from the installed Android codepath, without reinterpreting concept images as observed UI. For native titles, instantiate the existing `NativeModuleRuntime` and capture its first screen via GLES2 `glReadPixels` into an archived PNG. For DOS titles, launch the actual DOS engine/archived executable on the emulator and capture the real framebuffer/screen only if an integration test proves the module started. Retain per-title capture manifest with exact Git SHA, Android API, native module ID, file digest, and result.
3. **Compact world-space library.** Translate the strongest elements—dense readable list/card hybrids, consistent blue focus/borders, a selected title summary, short genuine status labels and controller battery readout positioned left of its bar—into the existing in-headset Canvas UI. Preserve existing hitboxes and bindings until tested replacements are ready. Prefer continuous list scrolling to additional page tabs, following existing DOS library scroll behavior. Maintain old focus/selection when moving, and never assign mismatched promotional art to a game.
4. **VR-safe controls.** Daydream tracked ray, clickable touchpad, Minus/App Back, O/Home Quick Menu, and intentional headset-gaze fallback must remain operable on the Galaxy S9; controller connection must never sparkle temporary gaze projections. Keep safe menu spacing, high contrast, focus hover/active/rejection sounds, head-look readable text scale, and exact UI geometry matches. 2D flat concept aspect ratios are reference only; project to correct world-space curved or flat panel for binocular comfort.
5. **Artwork and UX implementation.** Game art must be original, locally licensed, and semantically matched to each game. Introduce thumbnails only when authored art is approved and staged with hashes/license notes. Native games can show preview frames captured from their OWN renderer; do not block launch on missing artwork. Add compact featured panel only where it does not displace search/scroll/recovery.
6. **Acceptance:** scripts capture all actually packaged native modules and each genuinely installed DOS title, in real GLES/DOS runtime, with images and log provenance. Compare real screenshots to designs; rerun Android emulator and high-contrast/input tests. Mark S9 headset optical/physical controller confirmation separately. Only then update live UI and claim visual adoption.

## Future game/design idea bank from the incorrect renders

- Peaceful open-sea fishing, sunset lighthouse, exploration mode/world distinct from Breakwater Battery's coastal defense gameplay.
- Retro computer-game library with curated title-specific card art, historical metadata/controls, compact scrollable roster and an obvious installed/not-installed distinction.
- A small side-scrolling jetpack/cavern arcade homage concept, subject to original art and naming/rights review.
- A curated retro FPS hallway/maze interface template useful for DOOM/Ken integration without suggesting replacement of their authentic engines/screens.

**Do not automatically implement any idea** merely because image generation illustrated it. Treat this page as design backlog, not an asset staging manifest or permission to redistribute third-party IP.
