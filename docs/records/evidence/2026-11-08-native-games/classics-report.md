# ReverieVR classic native games: audit and conservative finish track

Audit baseline: `12d4018c90e56a3767b8ffc58a2e5394440caec1` (`main`). Audit date: 2026-11-08. Scope: eight packaged classic modules. No production code was changed by this audit; tests and diagnostic harnesses live beside this report. No GitHub Actions were started.

## Decision

**The eight classics have implemented playable rule loops and substantial passing host regressions, but none meets the requested release bar because none saves or restores its match.** This is a reproduced failure in each actual compiled module, not an assumption from the absence of a serializer. Every tested module accepted real module input and progressed; every one retained its in-memory state across `Pause`/`Resume`; every one reverted to the initial state after `Destroy`/`Create`. The supplied persistent host slot fixture received **zero reads and zero writes** for all eight.

The shared module constructs a fresh board/game unconditionally (`classic_games_module.cpp:1249–1263`) and its `Resume` and `Pause` callbacks are empty (`1340–1341`). The native handbook explicitly calls for bounded persisted game state and reference-device acceptance (`docs/native/README.md:35–45`, `49–58`, `95–101`). A shell Resume/Continue label cannot substitute for restored native match state.

**The alleged illegal Chess move was not reproduced in the current move generator or the tested actual pointer paths.** Independent reference move-tree counts matched exactly at 20 position/depth combinations across six standard positions. Actual module pointer tests rejected blocked White and Black queen moves and committed the expected White and rotated Black pawn moves. This narrows the remaining investigation to the reported position, input event sequence, asynchronous result history, and presentation; it does not establish that every possible device defect is absent.

All eight entries are registered with `developmentOnly=false` (`GameModuleRegistry.java:39–54`). That admission flag is not a test result or a release certificate.

## Evidence and limits

| Evidence | Observed result | What it establishes | What it does not establish |
|---|---|---|---|
| Existing `scripts/verify-classic-pong.sh` | Exit 0; g++ Pong dynamics and full classic deterministic suite pass; clang++ absent/skipped | Production simulation, move legality cases, terminal cases, CPU safeguards, several geometry/artwork checks | Physical controller behavior, stereo comfort, long Android sessions, save restoration |
| Eight directly compiled production `classic_games_module.cpp` variants | Build exit 0 each; run exit **1** each for `FAIL_SAVE_RESTORE` | Real `Update`, `Pause`, `Resume`, `Destroy`, `Create` lifecycle; actual pointer/primary actions; reproducible lost progress | GLES rendering, operating-system kill semantics, real disk atomicity |
| Independent Chess perft | Exit 0; 20 exact matches | Production legal move generation and transitions against an external reference corpus | Every game state, draw-history correctness, input timing, UI or Android |
| Chess production module input probe | Exit 0; 8 scenarios pass | Pointer-to-square mapping for both colors; blocked queen attempts; legal pawn moves; New Game/Cancel/Black; reference Close; AI control | Entire interactive menu tree, physical hit reliability, full match |
| Existing Chess panel-layout program | Exit 0 | 45-degree panel transform and pointer-ray roundtrip | Visibility and comfort through headset optics |
| Root-provided authentic Android initial menu screenshots | Visually inspected all eight | Actual initial rendered content, not a fabricated mock-up | In-session/terminal menus, restored states, readability on S9/Daydream optics |

The Android screenshots are in `../remote/hub/native-menu-classic-*.png`. Parent audit reports their source as `c10ddd72` (parent of baseline, without classic-file changes) and a passing 13-test Android instrumentation artifact including all sixteen initial native launches. Treat that as parent-verified historical Android evidence, distinct from tests executed locally in this child audit. Parent owns the precise CI artifact/run citation.

The lifecycle harness links unused graphics functions out with function/data sections and linker garbage collection. It does **not** stub a renderer or claim a render pass. Its host storage fixture retains real slot bytes across module instances; game serialization would execute unchanged if present. The assertion checks progressed state versus recreated state, and records read/write callback calls. No new game logic was substituted.

The first iteration of the Chess input fixture incorrectly rotated Black coordinates twice; `BoardX`/`BoardY` already rotate them (`classic_games_module.cpp:148–160`). That diagnostic fixture was corrected before results were recorded. The initial fixture assertion failure was not a product defect. The corrected run passes, with the actual module still doing the hit test and move application.

## Per-game assessment

### Pong — complete compact match loop; persistence and device finish work remain

Implemented: bounded court physics, player and imperfect CPU paddles, swing-dependent rebounds, serve between points, first to eleven, and restart. Pointer movement and normalized movement input reach the production paddle update. The existing suite exercises swing transfer, inverse braking, rebound bounds, CPU targeting, missed returns, scoring, both winners, and restart (`classic_games.h:90–142`; `classic_games_test.cpp:1350–1419`). The actual module was served and advanced for thirty updates before the lifecycle test.

The authentic initial screenshot has a clear title, opposing colored paddles, ball, score, and **PRESS TO SERVE**. It does not communicate first-to-eleven or pointer versus pad operation on that first screen; the active-state prompt changes to **MOVE PADDLE TO BLOCK** (`classic_games_module.cpp:978–991`). That is a specific onboarding/QoL opportunity, not evidence that controls fail.

Finish in order: (1) versioned save of score/opponent, paddle positions, ball position/velocity, moving/finished state, clock and RNG, restoring at a safe served/paused boundary; (2) concise controls and match target, explicit winner text, replay/abandon decision that respects the retained match; (3) full human/CPU match and interruption at serve, rally and match end; (4) phone input latency, frame pacing and long rally testing. Conservative release gate: a scored match survives module/process recreation with the same deterministic next state and the user can distinguish resume, replay and new match.

### Snake — complete fixed-grid survival loop; input/readability acceptance needed

Implemented: 12×12 board, food, growth, forbidden reversal, wall/self collision, restart, and true full-board completion. Food selection scans actual free cells, with tests for near-full and full boards (`classic_games.h:36–67`, `143–163`; `classic_games_test.cpp:1421–1467`). Module input began a run and advanced the real snake before lifecycle reset.

The screenshot visibly presents **UP/RIGHT/DOWN/LEFT**, the snake, food and score. UP/DOWN controls overlap the upper/lower playfield footprint (`classic_games_module.cpp:993–1030`). Food and snake geometry are drawn in front of the button backgrounds by depth, so this audit does **not** claim hidden food; button text and the playfield still deserve an occupied-cell visual test. The displayed grid is readable in the 512px capture, without physical-optics evidence.

Finish in order: (1) preserve the ordered snake body and length, heading and pending desired direction, food, RNG, tick accumulator, moving/finished state and score; (2) test fast direction changes, held input and menu-open input release, and move controls if they obscure occupied cells; (3) distinguish collision from board-complete results, add an understandable resume/start cue; (4) device test slow and fast head/pad changes, near-full board, pause at a tick boundary and recreate mid-run. Do not make difficulty modes a completion requirement unless intended scope is expanded.

### Chutes & Ladders — complete chance-game loop; presentation needs the largest arcade QoL pass

Implemented: 100-square board, a single authoritative table of nine ladders and ten chutes, die rolls, human/CPU turns, exact landing at 100, overshoot forfeiture, ladder-to-100 win and restart (`classic_games.h:13–21`, `69–88`, `164–171`). Existing tests exercise exact-roll/overshoot cases for both sides and a deterministic played-to-finish game (`classic_games_test.cpp:1470–1538`). The actual module rolled and progressed before lifecycle reset.

The screenshot contains all numbered squares and actual connecting chutes/ladders. The small numeric labels and crossing colored links are visibly dense at 512px; this is a concrete target for readability acceptance, not a hardware failure diagnosis. Source draws each square label at `.017f` and draws direct segments through the board (`classic_games_module.cpp:1033–1049`). Players currently teleport to the post-link square in the simulation. Status changes give position, spin and turn, with no authored intermediate movement animation (`1052–1072`).

Finish in order: (1) save both positions, RNG, last roll, turn owner, CPU-delay clock and winner state so reload cannot reroll or skip a turn; (2) clearly show roll → landing → chute/ladder → final square, readable numbers and distinguishable endpoints, plus exact-finish rule; (3) exercise an entire match with all transition types, overshoot, shared square, both winners and replay; (4) verify headset numerals and turn comprehension. Animation can be minimal, but outcome explanation must be understandable.

### Checkers — substantive rules and bounded CPU; finish the match-management surface

Implemented: 8×8 play, mandatory captures, capture continuation, kinging, blocked-player win, human/CPU turns, three difficulty settings with bounded searches, position repetition and quiet-move draw claims, including prospective repetition (`classic_board_games.h:275–564`; `classic_games_test.cpp:569–737`). Actual pointer selections committed a legal opening human move. Existing tests exercise multi-jumps, crown behavior, claim conditions and CPU capture search.

Initial screenshot has high-contrast pieces, **YOUR TURN**, **AI NORMAL**, and **SELECT PIECE THEN DESTINATION**. In-module active-match restart/abandon is not presented like Chess's New Game; finished/error state accepts a primary press to reset (`classic_games_module.cpp:1576–1580`). This becomes more important when saving is added: Home should cease being the accidental reset mechanism. Difficulty and draw controls have real handlers (`1498–1508`, `1591–1618`).

Finish in order: (1) save cells/kings, side, forced capture continuation, result, quiet-ply count, repetition history, difficulty, RNG and last move; rebuild asynchronous CPU safely after restore; (2) add explicit new match/abandon flow and understandable forced-capture/draw explanations; (3) full matches on all difficulties, restoration mid-multi-jump and while CPU is thinking, king-endgame repetition and quiet draw; (4) headset piece/king distinction and accurate targeting. A board-only save is inadequate because it would change legal continuation and draw entitlement.

### Go — complete core 19×19 rule/scoring loop; longest acceptance burden among the classics

Implemented: 19×19 captures, suicide rejection, positional superko using stored board history, human Black/CPU White, pass, two-pass score review, dead-group marking, Chinese area score with 7.5 komi, score acceptance and resuming with preserved next side (`classic_board_games.h:568–714`; `622–698` for state-changing review actions). Existing tests cover captures, ko/superko, passing, score review, dead stones, accepted outcome and resumed turn. Actual pointer input placed the center stone before lifecycle reset.

Initial screenshot shows the full 19×19 grid, **YOUR TURN**, **PASS**, and **TWO PASSES: REVIEW DEAD GROUPS**. This is the smallest target spacing of the classics: `.128f` versus `.320f` for 8×8 boards (`classic_games_module.cpp:145–160`). The PASS/review accept and RESUME handlers are real (`1583–1590`). The AI is a bounded heuristic opponent; passing legality does not establish teaching quality or playing strength.

Finish in order: (1) a bounded, validated exact save of board, side, pass count, complete relevant superko history, review/dead marks, resume side, scores, outcome and RNG; (2) review-state usability: explain group toggles, score ownership, komi, acceptance and Resume; test dense-grid hit error and provide adequate selection feedback; (3) full human/CPU sessions through disputed dead groups, both passing orders, live ko and resumed scoring; (4) physical 19×19 targeting/readability and prolonged CPU/frame behavior. Avoid silently truncating superko history to fit a save: that changes legal moves. Define the supported history bound or a compact exact representation deliberately.

### Chess — strong current rules evidence; reproduce the reported context before changing legal rules

Implemented: choose human White/Black with rotated board, king-safe moves, castling, en passant, queen/rook/bishop/knight promotion, checkmate/stalemate, conservative dead-position cases, threefold/50-move claims, prospective claim checks, automatic fivefold/75-move draws, SAN history, captured-piece tallies, three CPU levels, opening/reference UI, attack overlays and presentation-only move tweens (`classic_board_games.h:740–991`, `1032–1134`; `classic_chess_board_ui.h`; `classic_chess_move_tween.h`). Existing CPU transition validation accepts exactly a legal single move and rejects corrupt/stale output (`classic_board_cpu.h:15–53`, `78–91`).

Independent canonical move-generation results all matched. Separately, the actual module accepted pointer e2–e4, rejected a White queen attempt to jump its d2 pawn, rejected a Black queen attempt to jump d7 in the rotated view, and accepted exact rotated d7–d5 after a production-generated e2–e4 setup. New Game → Cancel, New Game → Black, left reference → Close and angled AI difficulty activation all passed.

The screenshot displays readable piece identities and central board. Its forward view crops both outer side wings. Source intentionally puts the wings at 45 degrees (`classic_chess_panel_layout.h:5–30`), and the ray/geometry regression passes. Treat reachability/readability under head turn as an outstanding acceptance test, not automatically clipped broken UI. Earlier historical ledger text describing queen-only/limited chess (`ACCEPTANCE_LEDGER.md:294`) must not be mistaken for the current engine's implemented scope.

Finish in order: (1) save cells, side, castling/en-passant rights, halfmove clock, repetition map, SAN history, chosen side, difficulty, captures, promotion selection and result; discard/regenerate transient CPU worker/tween objects safely; (2) instrument a compact move record of position before, accepted action, position after, worker serial and presented tween so an alleged illegal move can be reconstructed; (3) complete all dialog/reference/promotion/draw interactions and interrupted games with both colors; (4) headset wing discovery, long SAN/reference content, dense board targeting, sustained CPU latency and thermal acceptance. Perft is strong move-generator evidence, but it intentionally does not adjudicate repeated-position histories, UI sequencing or device input.

### Breakout — complete two-wall arcade loop; durable runs and court clarity remain

Implemented: 14×8 brick wall, three lives, row-based points, fixed `.01s` physics tick, paddle steering, serves, speed-ups, top-row behavior, narrowed paddle and two-wall victory (`classic_breakout.h:13–119`). Existing tests cover physics, scores/lives, wall reset/victory and finite/bounded states (`classic_games_test.cpp:1258–1333`). Actual module input served and advanced the live ball before reset was reproduced.

The initial screenshot clearly shows score, lives, wall number, paddle and **PRESS TO SERVE**. Individual bricks visually merge into narrow horizontal color rows at this capture scale; source leaves only `.006f` horizontal gaps (`classic_games_module.cpp:934–939`). This is a court readability test target, particularly after scattered bricks are removed. It is not proof of wrong collision geometry.

Finish in order: (1) preserve the complete brick mask, wall, lives, score, paddle/ball/velocity, fixed-step remainder, paddle-hit count, narrow/top-row flags and outcome; (2) test isolated remaining bricks, state cues and end/replay legibility; (3) run both walls and all lives through actual module control, interrupt between life loss/serve and wall transition; (4) target device ball tracking, input latency and sustained timing. Reinstating only score and lives would lose the run.

### Reversi — complete rules/end loop; small set of usability and state tasks

Implemented: 8×8 directional flips, valid-move indication, exact disk totals, forced pass, ending when both sides cannot move (even with vacant squares), win/loss/draw and bounded CPU search (`classic_board_games.h:130–273`; `classic_games_test.cpp:1166–1255`). Actual module pointer input committed one legal human move. Existing tests cover multi-direction flips, pass, CPU operation and terminal scoring.

The screenshot shows **SELECT A MARKED SQUARE**, four legal indicators, current turn and both scores. Indicators are small bright dots; actual headset recognition needs acceptance. Automatic pass deserves a clear transient explanation so a consecutive turn does not look like broken turn order. This audit found no rule defect in executed cases.

Finish in order: (1) persist board, side, pass/result state, move count, RNG and difficulty/CPU policy if user-selectable; (2) explicit pass/result/new-match feedback and robust legal-square contrast; (3) full games including forced-pass chains and both-blocked ending with empty squares, then recreation at each state; (4) physical pointer and contrast testing. This is one of the smaller finish tracks once shared persistence is supplied.

## Shared completion and release sequence

1. **Preserve earned progress.** Implement explicit versioned per-game encodings, corruption/length/value validation, transactional load, save after committed board turns and safe arcade checkpoints, save on pause, and retry/error feedback. Keep RNG and rule history. Do not serialize raw STL memory, mutexes, futures or GL objects. Existing host slot isolation can serve all eight; no new host ABI is justified by this audit.
2. **Make state transitions comprehensible.** Verify start/resume/new match/replay/abandon behavior, terminal prompts, score/turn feedback and reference/control panels. Check interruption while a human piece is selected, a capture chain is pending, AI computes, a promotion is pending, a Go review is open, or an arcade ball is live.
3. **Exercise actual hosted runs.** Use deterministic replayable action logs plus real module callbacks; pass reset/recreate/context-loss tests; inject missing/corrupt/full/failing save storage and recover without overwriting the last valid checkpoint. Retain failing fixture and logs before edits. Re-run only tests covering the repaired risk, then the existing native/host integration gates required by repository policy.
4. **Establish physical acceptance.** Each game needs the supported phone/headset/controller, readable and reachable UI in both eyes, no stuck actions on disconnect/menu return, stable frame time with active AI, and a sustained battery/thermal session. Capture source/package identity and short screen recordings/screenshots of start, active play, terminal state and restored state.
5. **Release only against recorded gates.** Code content may be compact and nearly complete while acceptance remains open. Publish a game as release ready only after its save defect is resolved and its per-game interaction/terminal/device tests pass. No percentage or calendar estimate is warranted from line count or passing compilation alone.

## Reproduction commands and outputs

Run from repository root unless stated otherwise. The exact absolute command arrays for every lifecycle build and invocation are in `lifecycle-results.json`.

```sh
bash scripts/verify-classic-pong.sh
```

Observed exit 0. See `existing-native-tests.log`. The script builds/runs Pong dynamics and full classic tests using each installed supported compiler. Only g++ was installed; clang++ is explicitly a skip.

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  -I app/src/main/jni/native \
  /workspace/scratch/d76b27d6a834/audit-work/classics/chess_perft_probe.cpp \
  -o /workspace/scratch/d76b27d6a834/audit-work/classics/chess_perft_probe
/workspace/scratch/d76b27d6a834/audit-work/classics/chess_perft_probe
```

Both exit 0. See `chess-perft.log`. Expected values were verified against [Chess Programming Wiki, Perft Results](https://chessprogramming.org/Perft_Results). This independent fixture uses the production `ChessTurns` and `ChessAfter` functions; the FEN parser supplies positions only.

| Reference position | Tested depths | Expected = actual leaf counts |
|---|---|---|
| Initial | 1–4 | 20; 400; 8,902; 197,281 |
| Kiwipete | 1–3 | 48; 2,039; 97,862 |
| Rook/pawn endgame | 1–4 | 14; 191; 2,812; 43,238 |
| Promotion/castling | 1–3 | 6; 264; 9,467 |
| Underpromotion | 1–3 | 44; 1,486; 62,379 |
| Middlegame | 1–3 | 46; 2,079; 89,890 |

Build the lifecycle probe separately with `-DCLASSIC_GAME_KIND=N`, where N=0 Pong, 1 Snake, 2 Chutes, 3 Checkers, 4 Go, 5 Chess, 6 Breakout, 7 Reversi:

```sh
g++ -std=c++17 -O1 -pthread -ffunction-sections -fdata-sections \
  -Wl,--gc-sections -DCLASSIC_GAME_KIND=0 \
  -I /workspace/scratch/d76b27d6a834/audit-work/classics/include \
  -I app/src/main/jni/native \
  /workspace/scratch/d76b27d6a834/audit-work/classics/classic_lifecycle_probe.cpp \
  -o /workspace/scratch/d76b27d6a834/audit-work/classics/lifecycle-0
/workspace/scratch/d76b27d6a834/audit-work/classics/lifecycle-0
```

All builds exit 0. Every corresponding run exits **1**, reporting `progressed=1 pause_resume_preserved=1 recreate_restored=0 reset_to_initial=1 read_save_calls=0 write_save_calls=0 result=FAIL_SAVE_RESTORE`. The aggregate Python runner exits 0 because it deliberately captures failing child test results instead of aborting; that runner status is **not** a pass for persistence.

Use the same flags with `CLASSIC_GAME_KIND=5` to compile `chess_module_input_probe.cpp`. Corrected probe exit 0; details in `chess-module-input.log`. Compile `app/src/test/native/classic_chess_panel_layout_test.cpp` using `g++ -std=c++17 -O1 -Wall -Wextra -Wpedantic -Werror -I app/src/main/jni/native`; exit 0 and log `chess-panel-layout.log`.

The scratch-only compiler include dependency is genuine Khronos headers, fetched through authorized GitHub reads: OpenGL-Registry `api/GLES2/gl2.h` blob `dd412ec37e5ee7c3e53752f692c8270832ad0f48`; `gl2platform.h` blob `5bcce6d89e0992419294ce95a269427a2811e2f7`; EGL-Registry `api/KHR/khrplatform.h` blob `01646449caedc958effc3b450360aefd260671ab`. None of those headers changes a gameplay result or replaces a game function.
