# Native chess — Black queen legality and capture motion (2026-11-04)

## Observation
Owner reported a Black queen apparently using a move illegal under chess
rules and specifically requested **slow visual motion**, especially a
capture where a piece otherwise vanishes with no clear cause. The
originally observed complete FEN / move sequence was not supplied.
Do not claim that report is disproven by passing source tests.

## Rule authority and input path
Source inspected on `mrcalzon02/ReverieVR` main:
- `classic_board_games.h` `ChessLegal` permits a queen to move only
  along a row, file or diagonal with a clear intervening path, and
  refuses moves that leave its own king in check.
- `ChessTurns` generates Black candidates from that same function;
  `ChessMove` checks the chosen move **again** before committing.
- The asynchronous CPU worker snapshots a state and accepts its
  result only through `ValidChessCpuTransition`, which independently
  reconstructs exactly one legal turn and verifies board state,
  castling rights, en passant, counters and SAN history.
- Before this change, the native renderer instantly drew the
  destination arrangement without any piece motion. A legitimate
  long queen slide or capture could appear to teleport.

## Implementation
Added game-private `classic_chess_move_tween.h` and wired it into
the existing `classic_games_module.cpp` board renderer. This layer
**never changes the authoritative rules state**. At a committed legal
ply, it records the previous board and last move; interpolates the
actual moving piece from the original square to destination using
bounded eased motion; and displays the previous position during
travel. Travel uses about 0.64–1.45 seconds based on squares traversed
and whether a capture occurs. Each move has a brief arrival hold:
0.20 seconds normally or 0.32 seconds for captures. Animation clocks
advance only during game Update, not during stereo eye rendering.

Captures: the victim remains visible in its original square until the
attacker's arrival, then the committed next board is displayed.
En passant removes the captured pawn from the adjacent square at
arrival. Castling moves the king and rook simultaneously. Pawn
promotion retains the traveling pawn appearance until it arrives,
when the chosen promoted piece appears. A late frame or invalid
negative/nonfinite delta cannot skip through invalid state.

User input cannot start a second board move during an ongoing
tween; the normal library/reference controls remain available.
Black CPU launch/poll is deferred until the preceding tween is done,
so the CPU does not appear to reply before the player's move has
visibly completed. The native UI also marks the Black move's actual
source and destination squares and shows `CPU Q d8-h4`-style
coordinates. Legal destination hint overlays are hidden until the
visualized board catches up with the committed state.

## Verification
Existing `classic_games_test.cpp` now contains an independently
implemented geometric Black queen ray oracle tested across thousands
of blocker/destination cases, an initial blocked d8 queen, a pinned
e7 queen, a legitimate d8-h4 checkmate, and the pre-existing exact
CPU transition gate. It also verifies tween motion progress,
capture persistence through travel/contact, en passant pawn
disappearance, two-piece castling, promotion timing, and invalid
frame deltas. No new test runner or framework was added.

The strict native chess/classics job under
[existing CI run 41959840342](https://github.com/mrcalzon02/ReverieVR/actions/runs/41959840342)
**PASSED** on C++17 GCC and Clang after a small C++ test declaration
correction, including all eight classic game families.
The wider all-games run was red due to a separate Breakwater mechanics
job; this is not a chess logic failure.
The current menu-archive and Android recovery capture workflows
remain necessary for an installed, rendered APK check. The signed
phone-test workflow can report green for its gate while skipping
APK production and must not be cited as a new APK.

## Pending reference-hardware acceptance
On Galaxy S9 + Daydream controller, verify from/to highlights,
queen diagonal/rook-like slides, capture victim persistence until
arrival, en passant, castling, promotion, and rapid button input
during tween; check that the library still opens, no doubled
stereo update occurs, audio feedback is appropriate, and performance
does not regress. Log `before` FEN, `lastFrom`, `lastTo`,
SAN history and APK commit if another illegal move is observed.
No in-headset result or shipped APK is asserted in this source
record. The existing rules engine remains the single authority.
