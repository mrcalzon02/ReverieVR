# Classic Native Games — implemented gameplay notes

**Status:** Source-implemented; Galaxy S9 + Daydream controller acceptance pending. Eight built-in classics use the native ABI v1 module host and the common `classic_games_module.cpp` renderer, compiled separately for Pong, Snake, Chutes and Ladders, Checkers, Go, Chess, Breakout and Reversi. This is game-private implementation documentation; the platform contract remains `docs/native/README.md`.

## Home movement, legible classics and indexed Chess (2026-10-16)

- **Shell locomotion:** Home and DOS Library traverse at exactly 1.40 m/s, twice the preceding 0.70 m/s cap at full touch. Opt-in hosted native-game locomotion remains 0.70 m/s; controller release, click, deadzone, bounded travel and menu gates are unchanged. Deterministic JVM regressions assert both rates.
- **Text and piece representation:** `classic_vector_art.h` replaces the shared 3×5 rect-per-pixel uppercase/numeric alphabet with contiguous, lightweight vector strokes (the same common text path for all eight games). On the active Chess board the six piece classes are visually distinct crowns/mitres/horse heads/battlements/king cross rather than single-character P/N/B/R/Q/K labels; light/dark disc contrast and selection markers remain. This is *original runtime flat-mesh artwork*, not imported SVG painting. Checkers kings and Go/Reversi discs remain appropriate board tokens.
- **Chess SAN:** Both human and asynchronous CPU moves append actual standard algebraic notation to the authoritative game state, including move ambiguity, castling, captures, promotions and check/checkmate suffixes. Reset clears the record; legal search and side-to-move ownership remain authoritative.
- **Offline indexed references:** `classic_chess_book.h` compiles **3,865** named ECO A–E lines from the [Lichess CC0 opening dataset](https://github.com/lichess-org/chess-openings) at `a6189a30dc273ccb21fc2536a9a2fefd5592a67a`. Prefixes match exact recorded SAN, and candidate next moves are independently admitted by the live `ChessLegal` / `ChessMoveNotation` rules before display. An out-of-book choice is explicitly marked a simple material heuristic, not a named opening or engine-certified move.
- **Useful compact panels:** Chess draws **CPU LAST** on the left with its actual opponent SAN and in/out-of-book comparison. The right **MOVE BOOK** pane lets a pointer/click toggle between **NEXT** (a legal suggested move with reference) and **EXAMPLE** (the indexed opening/variation name). The tab is a real hit region, not a phantom interface. Labels are condensed to fit VR side rails without adding pages.
- **Sourced vector assets for future rasterization:** All twelve original [Livius SVG chess pieces](https://github.com/kmar/chess_svg_piece_sets), CC0 dedication and provenance are vendored at `art/source/chess/livius/` (source commit `af40ea51e87eddee1ae7ee35ae312893a1271233`). These **are not yet the runtime sprites**; using the original SVG fills/outlines in GLES2 will require a deterministic conversion/atlas, separate shader/UV acceptance, and actual S9 readability/frame-time validation. Runtime currently uses the custom vector silhouettes described above. Do not conflate staged source artwork with shipped GL visuals.
- **Evidence:** Native regression source tests assert stroke geometry and six distinct piece types, SAN history and legal book moves; Android NDK compilation, full suite execution, SVG atlas conversion, signed update and Galaxy S9 controller/stereo/thermal acceptance are **not established** by the source commit. No ROMs, network service, new public JNI ABI or GitHub Actions were introduced.

## Reversi 8×8 — eighth complete native classic (2026-10-15)

New built-in module `classic-reversi` implements a full 8×8 match:
four-disc opening, eight-direction flipping, forced automatic passes when a
player has no legal move, and terminal scoring by Black/White disc counts
even if inaccessible vacant squares remain. The human is Black and plays
first. White is a CPU with bounded iterative alpha-beta: four-ply maximum,
4,000 visited positions across all iterations, discard-on-exhaustion root
choices, terminal score priority, and positional/corner/mobility evaluation.

The shared native board renderer marks legal Black destinations, shows
live and final counts, and reports an automatic pass without adding a
separate tab. Daydream cursor, head/controller ray and Select are reused.
Select restarts the completed match; Home stays shell-owned. Existing
game-private CPU snapshot workers prevent search from running within
the two stereo rendering passes. Native regression coverage includes
opening legality, flipping, no-op illegal moves, pass handling,
Black/White/draw adjudication, worker isolation, CPU corner capture
and search budget. No third-party ROM or new framework is needed.
Native/Android compilation, stereo readability, controller precision
and sustained Galaxy S9 performance remain separate evidence gates.

## Chutes and Ladders — complete classic board (2026-10-15)

Chutes and Ladders now uses the common 100-square board's complete nine
ladders and ten chutes rather than the previous arbitrary five of each.
One fixed source table drives **both** the exact game rules and the line
geometry, preventing mismatches. Squares are numbered in the one-time
static board mesh; colored connections sit behind the player tokens, and
the latest six-outcome spin appears beside both positions.

Both human and automatic CPU spins must reach square 100 **exactly**;
an overshoot leaves the pawn in place and passes the turn. Landing on
square 80 climbs directly to 100. The RNG and alternating turns are
unchanged; Chutes and Ladders is luck-based rather than an adversarial
search game. Native regression coverage now checks all 19 transitions,
no repeated start squares, human/CPU overshoots, exact victory, the
80-to-100 ladder, and full-match completion. Reference:
https://assets-us-01.kc-usercontent.com/500e0a65-283d-00ef-33b2-7f1f20488fe2/9993cf54-5c86-40a4-b386-c0194b98bb9a/B77420920_Retro_Chutes_and_Ladders_INST_15_FAR.pdf

This is a source implementation; integrated Android compilation and
S9/Daydream headset readability still require independent verification.

## Chess search and nonblocking CPU turns (2026-10-15)

Chess now enumerates all four promotions for both colors throughout search,
recognizes horizon stalemate, preserves move-count progress in hypothetical
positions, and gives checkmate precedence over automatic move-count draws.
Iterative alpha-beta keeps only completed iterations, orders captures and
promotions first, and has hard total budgets of 1,500 / 6,000 / 12,000 visited
positions at Easy / Normal / Hard, with target depths of 2 / 3 / 4 plies.
The compact AI button cycles difficulty; Secondary does the same when no
piece is selected and retains promotion cycling for a selected seventh-rank
pawn. Restart preserves the selected difficulty.

The claim action also handles an identified legal intended move creating a
third repetition or reaching 100 quiet half-moves. It records the chosen
intended move in lastFrom/lastTo without playing it or handing over the turn.
A materially losing CPU can exercise the same earned claim. This implements
FIDE 9.2.1.1 / 9.3.1 in the untimed computer-game interface; tournament
scoresheets, arbiter penalties and clocks remain outside this interface.
Reference: https://handbook.fide.com/chapter/e012023

Chess, Checkers and Go CPU responses now run on one game-private worker per
module, using an owned snapshot. Update polls without waiting and commits a
completed result once; both eyes keep rendering the unchanged live board
while it thinks. Workers never call GLES or host services. Destruction joins
the worker before native-library unload. Pause can leave computation running,
but results are applied only after Update resumes. Launch/worker exceptions
are logged and show a restartable CPU error rather than inventing a draw.
This removes synchronous search from Update, but thread startup, snapshot
copying, result transfer, unload latency and sustained S9 thermal cost still
need measurement. No native ABI or shell recovery change.

Deterministic coverage includes legal repetition cycles, prospective claims,
pawn-progress reset, horizon stalemate, both-color promotion generation and
execution, CPU draw claims, exact node caps, all difficulty levels, and
worker snapshot isolation / one-time result consumption for all three games.
Host strict C++17 tests and ABI tests pass. The existing native doc guard
currently fails on an unrelated pre-existing Test Chamber shader-consumption
anchor (BuildProgram calls CompileShader indirectly); it has not been weakened.
Android signed-build and physical headset validation are separate gates.

## Breakout (2026-10-15 source addition)

Original arcade **Classic Breakout** is a seventh built-in native module
(`classic-breakout`) in the existing **Native Games** shelf. It uses the same
ABI v1 loader and colored-triangle GLES2 renderer as the six shipped classics;
there is no external ROM, game executable, art download, music dependency or
separate Android application. Static court and title geometry are uploaded
once per GL context, while only the paddle, ball, remaining bricks, wall number, score,
lives and status are rebuilt on visible state changes.

- 14 columns × 8 rows (112 bricks per wall), row-weighted 7/5/3/1 points
  from top to bottom, three lives and two successive walls;
  top-wall rebound narrows the paddle and paddle-contact milestones
  increase speed;
  clearing both walls wins, losing the third ball ends the match.
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

## Checkers, regulation Go 19×19, and Chess (2026-10-15 rule correction)

Checkers (8×8) and Chess (8×8) keep their standard board sizes.
The Go module `classic-go` now has all **361 intersections** of the
regulation 19×19 board, not a small-board approximation. The rules engine,
19×19 renderer, nine star points, and native host title update together.
Go's original 9×9-only cutoff, board arrays, scoring traversal and liberties
were expanded, with tests for distant-corner placement and edge captures.

- **Go:** 19×19, exact positional superko, captures and no-suicide.
  Two consecutive passes now begin an **explicit scoring-review phase**,
  instead of silently finishing the game. Select a group to mark or unmark
  all its stones as dead; the right SCORE control confirms Chinese area
  scoring (7.5-point white komi) and displays the actual black/white
  scores. RESUME on the left restores play with every original stone,
  clears marked groups and restarts the pass sequence, preserving the
  actual next player after the second pass. White-first/Black-second
  passing correctly returns play to White, not Black. Native Secondary
  operates PASS during normal play and SCORE during review. These controls
  share the existing VR pointer/select surface and do not change Home.
  This does not yet provide an independently adjudicating Go opponent:
  the CPU accepts human dead-group markings; automatic life/death
  judgment, seki settlement and configurable rulesets remain open.
  The CPU searches all 361 intersections, protects basic edge/corner
  eyes and reciprocates Black's pass when its current 7.5-komi area
  estimate already leads. Tactical move evaluation now recognizes
  connected white groups in atari, rewards legal escapes that preserve
  their stones, and rejects new self-atari even when making a capture.
  Native regression cases include a one-liberty rescue and the mirrored
  forced capture on the full 19×19 board. This remains a heuristic
  opponent, not independently validated life/death judgment or advanced
  territory search. The Android build / Galaxy S9 gates are separate.
- **Checkers:** WCDF American/English 8×8: 12 men per side,
  compulsory diagonal capture, complete multiple jumps, and crowning
  ending the turn. Draw claims are available on three completed
  occurrences **or in anticipation of a legal third repetition next
  turn**, and after 40 moves *per side* without a man advancing or a
  capture. Forced multi-jumps cannot be interrupted by a draw claim.
  The CPU may exercise a genuine draw claim when losing; it still
  honors complete capture sequences. Alpha-beta evaluates four
  complete turns in the opening and up to six in sparse endgames,
  with three real selectable search levels: Easy 1,500 nodes,
  Normal 6,000 nodes and Hard 12,000 nodes, preserving forced-capture
  quiescence at leaf nodes. Iterative deepening now commits only fully
  evaluated depths, preventing a node-cap interruption from favoring
  partially searched candidate moves. The AI button and Secondary
  cycle difficulty during the human turn and restart preserves it.
  Negotiated draws, resignation and clocks remain pending, as does
  Daydream-device AI latency profiling. Rule reference: https://nccheckers.org/NCCA/WCDF%20Checker%20-%20Draughts%20-%20English%20Rules.htm
- **Chess:** standard 8×8, king-safe moves, castling and en passant,
  four promotion pieces (queen, rook, bishop, knight), checkmate and
  stalemate. FIDE drawn-game handling now tracks exact positions with
  side-to-move, castling rights and effective en-passant; an on-board
  claim-draw action becomes available after threefold repetition or 50 full
  moves without capture/pawn progress. Fivefold and 75-move termination
  are automatic, with mate taking precedence. Detects unequivocal dead
  material states (lone kings, solitary minor piece, same-color bishops).
  Select a pawn on the seventh rank and use native Secondary to cycle
  the promotion choice before placing it; ordinary moves still queen
  by default. The CPU now evaluates legal human responses through iterative bounded alpha-beta
  search at the selectable levels described above. It is still
  a lightweight phone-friendly opponent, not a tournament-grade engine;
  its search leaves explicitly recognize checkmate so it cannot
  mistakenly value a mate-in-one loss as a material gain;
  it can claim an earned draw while materially losing; agreed draws and clocks remain open.
  The native GUI needs Galaxy S9 acceptance for claim/promotion controls.

For all three, choose a piece and a destination (or place in Go).
Rules and CPU advance only once per native Update, never separately in
stereo-eye draws. CPU quality, Go legibility at 361 intersections, endgame
adjudication and controller precision need S9/Daydream testing.

## Shared compact geometry and idle frame savings (2026-10-14)

The original six classics now use `reverie_native_flat_mesh.h` for unchanged shapes, shared with the two recon-board modules. Pong rebuilds on visible paddle/ball/status changes; Snake on head/food/score/status; Chutes and Ladders on token, turn and finish changes. Simulation remains independent of both stereo eye renders. Inactive frames no longer regenerate all 144 Snake grid cells or 100 Chutes cells and re-upload the VBO. Geometry parameters, gameplay, shell controls and ABI v1 remain unchanged. Host geometry assertions are added to `classic_games_test.cpp`; S9 compilation/runtime remains to be verified.

## Static GPU scene / dynamic GPU scene (2026-10-14 implementation)

All six classics now generate their immutable title frames, court/board grid, and fixed legends once in the module's GLES context creation callback. This geometry is uploaded to a dedicated `GL_STATIC_DRAW` VBO and its expanded CPU vector is discarded. The existing `GL_DYNAMIC_DRAW` VBO now contains only moving tokens, chess/checkers/Go pieces, Snake food and body, Pong ball/paddles, score/turn labels, cursor and selection markers. Dynamic geometry rebuilds only after a visible game state change; the static VBO persists through every frame and is recreated deterministically after GL context loss. Both eyes use the same buffers; camera matrices, control bindings, gameplay and shell recovery were not modified. Extra draw call per eye versus fewer uploads is a performance trade-off requiring S9 measurement, not a guaranteed battery saving.

## Gameplay and input

Pong now restores the original horizontal court with left/right opposing vertical paddles, upper/lower wall rebounds, contact-angle effects and a first-to-11 score. The human controls the left paddle using vertical Daydream input or head-tracked pointer height; the right paddle remains a bounded CPU opponent rather than falsely claiming the original two-human arcade mode. Snake uses a 12×12 board, cardinal controller direction, food growth, and wall/body collision. Chutes and Ladders uses a 100-cell serpentine board with seeded dice, turn-taking, and fixed chute/ladder destinations. Simulation is updated once per frame; stereo rendering observes that state without advancing game rules.

## Snake food-placement correctness

`State::NewFood()` now marks occupied body cells in a fixed 144-byte stack array, counts the genuinely available cells, and uses **one seeded random selection over only free cells**. This replaces a capped series of random retries that could falsely declare the game finished while one or more empty cells remained. Food remains deterministic for a given seed and board state; it is never placed on the body. A full board alone triggers `finished` with `food == -1`. No heap allocation or additional frame-loop work is introduced; the bounded scan runs only when food is placed.

`app/src/test/native/classic_games_test.cpp` covers 128 seeds with one free tile, both outcomes with two free tiles, and genuine full-board completion, in addition to existing Pong, Snake movement, and Chutes and Ladders rules. A focused C++17 `-Werror` fixture for the corrected selection algorithm passed locally; integrated native/Android builds and physical headset testing must still be verified independently.

## Authority

Game simulation: `app/src/main/jni/native/classic_games.h`. Game UI and input: `app/src/main/jni/native/classic_games_module.cpp`. Module allowlist: `app/src/main/jni/native/reverie_native_host.cpp`. Build wiring: `app/src/main/jni/Android.mk`. Native tests: `app/src/test/native/classic_games_test.cpp`. Shell recovery and controller routing remain shell-owned.

