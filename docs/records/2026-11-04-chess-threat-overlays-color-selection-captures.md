# Native Chess: threat overlays, choice of side and captured-piece tally

**Scope:** ReverieVR / app/src/main/jni/native / classic-chess, 2026-11-04.
**Status:** code implemented and exact remote commit verified; deterministic
classic-games host tests passed; installed Android and Galaxy S9 optical
acceptance pending.

## Intent
Players requested visible, clickable board-control overlays across the
top for **Neutral**, **My Threats**, **Opponent Threats**, the ability to
start a new game playing **White or Black**, and actual captured-piece
tallies for both sides below the board. This builds on the earlier
slow legal-move tween and original source-cited chess reference.

## Authoritative implementation
- `classic_board_games.h`: Chess retains FIDE-aligned White +1 and
  Black -1 squares, with `chessHumanColor` (default White) independent
  of side-to-move. `StartChessGame(-1)` restarts with a Black human
  and White CPU playing first; `Reset(Chess)` preserves that choice.
  `ChessLegal`, `ChessTurns` and `ChessMove` remain the only
  move gate for either player. Promotion, draw claims and CPU search
  use the chosen side. CPU evaluation and mate scoring flip according
  to the CPU's color without rewriting any piece rules.
- `classic_board_cpu.h`: off-thread chess worker starts whenever
  the side to move equals `ChessCpuColor()`, validates precisely one
  king-safe legal ply and its captured-piece counters for either
  color. A monotonically increasing `chessGameSerial` prevents an old
  CPU result from entering a newly started game.
- `classic_chess_book.h` and `classic_chess_reference.h`:
  advisory ECO suggestions are made only on the human turn, and the
  last CPU move is identified correctly whether Black or White.
  Library and historical SAN data stay in original White-first order.
- `classic_chess_board_ui.h`: three board overlays use the same
  `Attacked(board,square,color)` rule as native king-safety checks:
  Neutral highlights squares attacked by neither color; My Threats
  highlights squares controlled by the player's chosen color;
  Opponent Threats highlights the CPU's controlled squares.
  Controlled squares include pawn diagonals and squares defended by
  same-side pieces. They are not predictions of winning captures.
  Clicking the selected button toggles the overlay off. Overlay
  colors are subdued and no board texture or captured piece is hidden.
- Orientation: selecting Black rotates the **display** and pointer/
  directional-cursor square mapping by 180 degrees. Internal squares,
  move legality and SAN remain standard. The existing move tween is
  rotated identically so its path stays aligned with the displayed
  board.
- New Game: right-wing button opens a visible two-color selection
  panel with White, Black and Cancel actions. A color choice resets
  captures, checks and visual tween state, updates current opening
  guidance, and changes input ownership. New games can be initiated
  even during a prior CPU computation: the serial mismatch safely
  discards its eventual stale result.
- Captures: `ChessMove` increments separate victim-piece counts
  keyed by the capturing side, from actual captures including
  en passant. Pawn promotion is not counted as the loss of a pawn.
  The bottom two rows display P/N/B/R/Q totals for **YOU TOOK** and
  **CPU TOOK**, regardless of which color the human chose. During a
  capture animation, the display withholds the new count until
  the victim visually disappears on contact.

## Verification
The existing native `classic_games_test.cpp` includes:
- White-start and Black-human/White-CPU first-turn assertions;
- legal one-ply White CPU worker transition and stale-game epoch
  rejection after New Game;
- side-relative screen-to-square orientation and overlay hit zones;
- exact initial squares attacked by each color, and a neutral square;
- captured-pawn totals from normal captures and en passant;
- no false capture from promotion, and Black human underpromotion;
- correct source-cited opening role and Black/White CPU move traces;
- animation/tally consistency, preserving the queen legality and
  move tween cases from the preceding change.

This test uses the existing GCC+Clang classic game functional workflow;
no second runner has been installed. Actual Android menu rendering,
pointer hit testing in stereo, and reference Galaxy S9 performance
are **not established by C++ host tests**. The phone-test workflow's
green gate is not a new built APK when its build-and-release job was
skipped.

## Hardware acceptance checklist
On Galaxy S9 Daydream controller and headset:
1. New Game -> White and New Game -> Black start cleanly. White always
   moves first, including CPU White when user plays Black.
2. Board and ray/cursor squares agree for both orientations. Queen
   and king initial positions remain on d/e files and moves legal.
3. Three top buttons can be activated, show correctly controlled or
   neutral squares, and toggle off without blocking piece selection.
4. Both capture counts show below the board, clear on reset, do not
   count promotions and only advance when the victim disappears.
5. Pawn underpromotion, castling, en passant, and moving/captured
   piece tween retain correct visual paths from either orientation.
6. Opening reference, difficulty, save/recovery and shell Home
   remain operational. No doubled input, stale CPU transition or
   stereo-frame simulation update.
7. Confirm actual APK SHA, per-eye capture, render load and signed
   artifact before marking device accepted.

Known limitation: these are *controlled squares*, not a full
chess-engine tactical danger evaluation. Pins can still mean a
nominally attacked square is not reachable by a legal move without
exposing the attacker's own king; this follows the standard attacked
square concept used in king-safety checks.
