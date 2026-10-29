# ReverieVR native desktop Home — early playable prototype

This is a separate **offline Java 17 Swing desktop application**, not an Android mirror, browser page, Electron shell, VR runtime, release installer, or replacement for the existing scene editor. It uses the real `HomeEnvironment` and `GameModuleRegistry` classes from the canonical ReverieVR source tree; the registry remains authoritative. No new dependencies. The existing Daydream APK is unchanged.

## Start and test

From a checkout containing `tools/desktop/player`:

- Linux/macOS: `bash tools/desktop/player/run.sh`
- Windows: `tools\\desktop\\player\\run.cmd`
- Headless state, rendering and arcade tests: `bash tools/desktop/player/test.sh`

Requires a local JDK 17+. Workspace settings are stored atomically in `build/desktop-workspace/desktop-home.properties`; use a path argument to choose a separate workspace. Offline and no headset necessary.

## Implemented

- A persistent perspective Home room showing three canonical environment identities, wall panels and a separate visible desk keyboard. `WASD`/arrows move; `Q`/`E` turn; Shift+mouse-drag rotates; mouse selects physically projected wall panels **and the individual labeled desk keys**. Opening the Native Games panel **does not leave Home**, reposition the viewer, or remove keyboard rendering/typing. Global quick options use `F1` (or Esc on unobstructed Home); Back/Esc closes in-room panels before leaving anything.
- Native Games uses real canonical game IDs but **only exposes genuinely playable desktop ports** as activation cards. Current ports: Pong (paddles, AI, scoring), Snake (food, growth, walls and self-collision), Breakout (bricks, paddle, lives). Each has playable input, `R` restart and `Esc` return to the preserved Home position and game panel. The other 12 native/classic modules **have not been ported**, so they are not fake launch buttons.
- `Search Games` or `/` activates physical keyboard text search, which can be dismissed with Enter/Esc while leaving the room. The 42 visible desk keys are individually mouse-selectable at their actual projected positions: clicking a letter/number opens the Games search and types, `BKSP` deletes, `SPACE` inserts a space, `ENTER` ends search focus, and `SEARCH` opens/focuses the Games catalog. Other panels and active games reject desk typing. Desk keys follow camera movement/turning; the wall gallery and physical keyboard continue working. Home has functional room selection, color-based avatar wardrobe, recenter, settings, Quick Menu and exit. Settings persist offline. This is not yet Android wardrobe-model fidelity.
- Code-only state/arcade tests verify all 42 projected desk hit targets at two camera poses, misses, search focus/text/backspace/Enter, non-Games/game input isolation, Home locomotion, game return pose, unavailable game refusal, three real games rendering/updating and save round-trip.

## Limitations / next work

The Home geometry uses an original CPU-rendered low-poly perspective preview, not the Android GLES mesh/material renderer. The desk keyboard uses per-key perspective hit targets and mouse clicks in this flat desktop prototype; tracked VR controller/gaze-ray interaction and device validation are not implemented. The three Java2D classic implementations are desktop prototypes, **not** the Android native `libreverie_module_*.so` binaries or ABI-equivalent gameplay ports. There are no fake DOS/media, OpenXR/SteamVR, phone panel or updater buttons. Gamepad and audio are not implemented. This is a runnable D1/D3 slice, **not full desktop parity, an installable `.exe`/AppImage, or a released package**. Full platform target and gates remain in `docs/project/DESKTOP_PLATFORM_EXPANSION.md`.
