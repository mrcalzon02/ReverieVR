# Classic Native Games — implemented gameplay notes

**Status:** Source-implemented; Galaxy S9 + Daydream controller acceptance pending. These original games use the native ABI v1 module host and the common `classic_games_module.cpp` renderer, compiled separately for Pong, Snake, and Chutes and Ladders. This is game-private implementation documentation; the platform contract remains `docs/native/README.md`.

## Breakout (2026-10-15 source addition)

Original arcade **Classic Breakout** is a seventh built-in native module
(`classic-breakout`) in the existing **Native Games** shelf. It uses the same
ABI v1 loader and colored-triangle GLES2 renderer as the six shipped classics;
there is no external ROM, game executable, art download, music dependency or
separate Android application. Static court and title geometry are uploaded
once per GL context, while only the paddle, ball, remaining bricks, score,
lives and status are rebuilt on visible state changes.

- 10 columns × 5 rows (50 unique bricks), 10 points per brick, three lives;
  hitting all bricks wins, losing the third ball ends the match.
- Press Select/touchpad click to serve, and press after win or loss to
  reset. Sideways Daydream touchpad movement, compatible external axis, or
  tracked-controller pointer moves the paddle. O/Home remains a shell action.
- Native CPU-only `classic_breakout.h` implements bounded fixed 10 ms
  physics steps, wall/brick/paddle contact, one-brick-per-tick collision,
  scored removal, ball-loss/serve recovery, and explicit win/loss states.
- `classic_games_test.cpp` exercises determinism across split frame
  updates, unstarted ball anchoring, paddle clamping/nonfinite protection,
  walls, single-brick scoring, bounce, lives, final-brick victory and restart.
- Native module: `reverie_module_classic_breakout`, built through the
  existing `Android.mk` profile (`CLASSIC_GAME_KIND=6`), admitted only
  through the host's packaged C++ allowlist. No native ABI version change.

Source admission and signed APK verification are separate from real S9/Daydream
performance, binocular presentation, scroll selection, paddle sensitivity,
small text legibility, sustained battery, and end-to-end game feel. The
existing six classic games retain their previous modes and controls.

## Checkers, Go 9x9 and Chess (2026-10-11 source)

Three more packaged native ABI-v1 modules are registered as `classic-checkers`,
`classic-go` and `classic-chess`. They share the low-poly GLES2 Classic
Games renderer, existing native shelf, host-owned pointer, stereo, Quick Menu
and recovery. Rule/CPU state is a private `classic_board_games.h` game engine,
updated exactly once per frame, never during each eye draw. Procedural
triangle-only pieces/boards and tiny fonts require no additional image,
AI framework, internet, telemetry, or OCR.

- **Checkers:** American/English rules, 8x8, forced captures and chains,
  movement/promotion for kings, human light side, bounded greedy CPU.
- **Go:** 9x9, captures, suicide prohibition, simple ko, two passes,
  Chinese-style area scoring and 5.5 komi for CPU White; capture/liberty
  heuristic CPU that can answer a late-game pass. Superko is not implemented.
- **Chess:** 8x8, legal king-safe movement, castling, en passant, automatic
  queen promotion and checkmate/stalemate detection; weak deterministic
  one-ply CPU. Threefold repetition, 50/75-move draw rules and
  underpromotion selection are not yet included.

Select a piece then a destination (or directly place in Go). Go PASS is
an on-board selectable command or a host secondary action. Board cursor
also accepts bounded movement axes when no ray is available. A primary
press after a finished match resets the game. Each AI decision occurs in
`Update`, never `Draw`. This is a source admission; native compilation,
unit rules, APK packaging, stereo and Galaxy S9 interaction testing are
independent gates.

## Shared compact geometry and idle frame savings (2026-10-14)

The original six classics now use `reverie_native_flat_mesh.h` for unchanged shapes, shared with the two recon-board modules. Pong rebuilds on visible paddle/ball/status changes; Snake on head/food/score/status; Chutes and Ladders on token, turn and finish changes. Simulation remains independent of both stereo eye renders. Inactive frames no longer regenerate all 144 Snake grid cells or 100 Chutes cells and re-upload the VBO. Geometry parameters, gameplay, shell controls and ABI v1 remain unchanged. Host geometry assertions are added to `classic_games_test.cpp`; S9 compilation/runtime remains to be verified.

## Static GPU scene / dynamic GPU scene (2026-10-14 implementation)

All six classics now generate their immutable title frames, court/board grid, and fixed legends once in the module's GLES context creation callback. This geometry is uploaded to a dedicated `GL_STATIC_DRAW` VBO and its expanded CPU vector is discarded. The existing `GL_DYNAMIC_DRAW` VBO now contains only moving tokens, chess/checkers/Go pieces, Snake food and body, Pong ball/paddles, score/turn labels, cursor and selection markers. Dynamic geometry rebuilds only after a visible game state change; the static VBO persists through every frame and is recreated deterministically after GL context loss. Both eyes use the same buffers; camera matrices, control bindings, gameplay and shell recovery were not modified. Extra draw call per eye versus fewer uploads is a performance trade-off requiring S9 measurement, not a guaranteed battery saving.

## Gameplay and input

Pong uses a controller-directed paddle and a simulated opposing paddle, with first to seven points ending the match. Snake uses a 12×12 board, cardinal controller direction, food growth, and wall/body collision. Chutes and Ladders uses a 100-cell serpentine board with seeded dice, turn-taking, and fixed chute/ladder destinations. Simulation is updated once per frame; stereo rendering observes that state without advancing game rules.

## Snake food-placement correctness

`State::NewFood()` now marks occupied body cells in a fixed 144-byte stack array, counts the genuinely available cells, and uses **one seeded random selection over only free cells**. This replaces a capped series of random retries that could falsely declare the game finished while one or more empty cells remained. Food remains deterministic for a given seed and board state; it is never placed on the body. A full board alone triggers `finished` with `food == -1`. No heap allocation or additional frame-loop work is introduced; the bounded scan runs only when food is placed.

`app/src/test/native/classic_games_test.cpp` covers 128 seeds with one free tile, both outcomes with two free tiles, and genuine full-board completion, in addition to existing Pong, Snake movement, and Chutes and Ladders rules. A focused C++17 `-Werror` fixture for the corrected selection algorithm passed locally; integrated native/Android builds and physical headset testing must still be verified independently.

## Authority

Game simulation: `app/src/main/jni/native/classic_games.h`. Game UI and input: `app/src/main/jni/native/classic_games_module.cpp`. Module allowlist: `app/src/main/jni/native/reverie_native_host.cpp`. Build wiring: `app/src/main/jni/Android.mk`. Native tests: `app/src/test/native/classic_games_test.cpp`. Shell recovery and controller routing remain shell-owned.
