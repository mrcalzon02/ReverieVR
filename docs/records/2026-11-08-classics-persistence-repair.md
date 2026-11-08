# Classic games: persistence and visible Snake controls repair

Date: 2026-11-08
Baseline: `43a1626d40788067128915aa77deb356434ae9b3`
Scope: Pong, Snake, Chutes and Ladders, Checkers, Go, Chess, Breakout and Reversi.

## Problem and resulting behavior

The audit executed the real classic module lifecycle for all eight games. Each advanced through real input, retained state during a same-instance pause, then returned to defaults after destruction and recreation. None called the host save services. The Snake probe also showed that a ray through the visible UP or DOWN button started the snake to the right; only invisible points farther above/below the board selected those directions.

The classic module now checkpoints through the existing module-scoped host save callbacks, restores exact game state before play, and displays a Continue panel after restoration. The first neutral input frame and a subsequent fresh activation are required. Pause prevents simulation updates and attempts a checkpoint. Ordinary exit uses the optional `prepare_exit` callback: it returns success only when no progress is pending or the latest checkpoint is confirmed. Failure retains the live instance for recovery. Destroy makes a last-chance attempt for legacy hosts and unavoidable teardown; its void callback cannot prevent forced destruction.

A failed checkpoint pauses gameplay behind an actionable Retry panel. It remains dirty and retries on a bounded two-second cadence, including while a controller remains held and gameplay is disarmed. A successful retry waits for Continue. Unreadable, unsupported, truncated or corrupt saved progress blocks new gameplay and writes. Retry re-reads it transactionally. There is no implicit reset or automatic replacement of an unreadable save.

Snake uses the same four button rectangles and depth for rendering and ray intersection. Visible focus and activation/failure feedback accompany the existing rule that forbids an immediate reverse turn.

## Exact persisted state

| Game | Preserved state and continuation |
| --- | --- |
| Pong | Both scores, serve/rally/terminal flags, ball and paddle positions, ball velocity, previous player paddle position used for swing transfer, CPU paddle and RNG state. |
| Snake | Complete body, length, food, score, current and queued directions, sub-tick clock, movement/terminal state and exact RNG state. |
| Chutes and Ladders | Both positions, die, turn ownership, pending CPU-roll delay, terminal state and exact RNG state. |
| Checkers | Board, side, selection, forced continuation square, every exact repetition position/count, quiet-ply counter, result, last move, difficulty and search diagnostics. A mid-jump checkpoint resumes the required remaining capture. |
| Go | All 361 intersections, every exact positional-superko board, side, passes, pending CPU state/RNG, dead-stone markings, scoring-review phase/revision, resume-side ownership and accepted scores/result. |
| Chess | Board, side/human color, castling and en-passant rights, half-move clock, exact repetition positions/counts, complete SAN history, promotion choice, captured-piece counters, result, difficulty, game serial and search diagnostics. |
| Breakout | Every brick, wall, remaining count, score/lives, paddle/ball/velocity, fixed-tick remainder, hit count, narrow-paddle/top-row flags and win/loss/serve state. |
| Reversi | Board, side, forced-pass count, selection/last move, result, ply and search diagnostics. |

Presentation-only hover, encyclopedia/new-game panels, animation interpolation and GPU objects are rebuilt. The committed logical chess position remains authoritative; restoration does not replay a partly rendered move. Worker futures are not serialized. Pending CPU work restarts from the exact saved board/RNG, with no old future crossing the destroyed module boundary. The existing chess CPU transition guard still verifies the resulting legal move.

## Format and interrupted-write behavior

`classic_games_save.h` owns a game-private schema. It records a magic value, version, game kind, explicit little-endian fields, payload length and a 64-bit accidental-corruption checksum. It does not serialize raw object memory, pointers, padding or STL implementation layouts. Decoding checks byte counts, enum/boolean forms, finite/ranged numbers, piece/body bounds, history consistency and checksums into a temporary candidate before assigning live state. Unknown schemas and wrong-game payloads are rejected. This checksum is a corruption check, not an authentication mechanism.

Go histories are exact packed two-bit positions; chess/checkers repetition positions use exact packed pieces plus side/rights and occurrence count. SAN strings are preserved. History ordering is canonical when encoded. There is no history truncation, approximate hash-only superko, or substituted initial position.

A snapshot may occupy up to 64 pages of at most 65,536 bytes each: 4 MiB total. Two bounded banks (`classic-a-00.v1` through `classic-a-63.v1`, and bank B) are committed by a checked `classic-match.v1` manifest. The module writes the inactive bank completely, then publishes its manifest through the host's atomic save callback. An interrupted page write leaves the previous committed bank intact. At most 8 MiB of page slots plus the small manifest are retained per game, including stale tail pages; pages beyond the current manifest count are ignored.

A manifest callback can return an error after a rename made new bytes visible, for example when final directory synchronization fails. The module treats that outcome as unresolved. It reads the manifest to learn which bank must be protected, writes a new complete inactive snapshot and requires a successful new manifest callback. Merely reading visible bytes never clears dirty state or authorizes ordinary exit. If reconciliation cannot read a valid manifest, it performs no further writes.

An invalid or larger-than-4-MiB snapshot fails visibly, leaves the live history complete and retains the preceding commit. This is an explicit capacity bound rather than a claim of unlimited match length. There is no classic legacy-save migration because the audited implementation had no saves.

## Checkpoint cost and schedule

There is no persistence work in either eye draw. Updates compare an allocation-free fingerprint of fixed-size game state and history sizes. They do not encode, sort or walk complete rule histories every frame. Public rule operations that change history also change the tracked ply, review revision, game serial or history size.

Board changes, meaningful arcade events (start/finish, score, life/brick changes, queued direction, roll/turn changes), Pause and ordinary exit trigger checkpoints. Active changes without one of those events checkpoint at a two-second cadence. Full history serialization occurs only at a checkpoint. Initial fresh state alone does not produce a save. Initial encoded snapshots range from 193 bytes for Breakout to 969 bytes for Go, before the separate 56-byte manifest.

The design relies on the host honoring the save callback's atomic/durable success contract. Shared host synchronization and exit policy are a separate coordinated repair, not implemented inside this game-private lane.

## Executed verification

Commands below ran in the isolated checkout; none launched a workflow or claimed an APK/headset result.

```sh
CLASSIC_TEST_GL_INCLUDE=/workspace/scratch/d76b27d6a834/audit-work/classics/include \
  bash scripts/verify-classic-saves.sh
bash scripts/verify-classic-pong.sh
```

The new verifier compiles the standalone codec/store suite with g++ C++17 and `-Wall -Wextra -Wpedantic -Werror`, then compiles and runs the production module separately for each of the eight game kinds. Graphics functions are linked out of the lifecycle tests; they are not replaced with fake rendering implementations. The storage fault fixture implements the real host callback result contract with an in-memory store that persists across module instances.

Observed passing scenarios include:

- Real input advances every game; Pause commits the latest exact state; destruction/recreation restores identical encoded state.
- Paused Update cannot advance simulation; held primary/secondary/axes remain disarmed after Resume; Continue consumes its own fresh edge.
- Write failures refuse ordinary exit, preserve the live state and previous committed storage, retry without user input, and recover the exact latest progress. A held controller does not prevent storage retry.
- Read failures and corrupt saves block writes, including during ordinary exit and Destroy, and recover after storage becomes readable again.
- Real pending CPU futures are created, then discarded/joined across destruction. Recreated Checkers, Go, Chess and Reversi workers finish the expected opponent move. Go/checkers/Reversi results match deterministic production simulation; chess passes the production legal-transition guard.
- Codec round-trips preserve future simulation/RNG, chess threefold claims and SAN, a mandatory checkers second jump, checkers repetition claims, Go two-pass/dead-stone review and final scoring.
- Final states are encoded and restored for Pong victory, Snake collision, a production-played Chutes finish, Fool's Mate, Breakout life exhaustion and second-wall victory, and a production-played Reversi finish. Checkers terminal capture and accepted Go score are covered by the rule-phase cases.
- Every truncation length of representative game snapshots is rejected transactionally. Corrupt payloads, unknown versions, wrong-game payloads, trailing bytes, noncanonical booleans, a checksum-correct NaN and an oversized untrusted history count are rejected.
- An exact 900-board Go storage fixture crosses the 64 KiB host-slot boundary. Interrupted second-page writes preserve the old complete snapshot. Late manifest error/reconciliation, corrupt/missing pages and a 47,000-position over-limit fixture retain prior storage and complete source history. These large synthetic sets are codec-capacity fixtures, not claims of legally played Go games.

The repaired Snake visible-center results are UP `77 → 65`, RIGHT `77 → 78`, DOWN `77 → 89`, LEFT `65 → 64`. The LEFT case first uses production simulation to make a legal upward move; otherwise Left would be a forbidden reversal from the initial right-facing snake. Each pointer case uses a released/pressed/released edge. The old invisible UP region no longer turns the snake.

The existing Pong and classic rules/CPU regression suite passes with g++; clang++ is unavailable and is explicitly skipped.

A separate scratch GLES probe renders the actual module on Mesa OpenGL ES 3.2 for all eight kinds. It captures Continue, failed-write Retry and unreadable-save Retry in both eyes: 48 frames. All draws return success without GL errors; render calls leave game state and write counts unchanged. The visible Continue button center activates through production Update. Representative Snake and Chess captures were visually inspected: the recovery text and button are visible and unobscured. These are software GLES captures with a controlled camera, not Android screenshots or physical VR readability evidence.

The codec suite also runs under AddressSanitizer and UndefinedBehaviorSanitizer. LeakSanitizer is disabled because this execution environment reports that it cannot operate under its tracing arrangement; no leak-check pass is claimed.

## Remaining release evidence

This repair removes the reproduced classic recreation loss and Snake target mismatch. It does not establish release readiness for any game. The outstanding audit gates remain: integrated Android storage/lifecycle fault runs, Quick Menu/Home/relaunch interaction with the shared host changes, real headset/controller/gaze readability and comfort, device pause/process-death behavior, long sessions, intended rule/UX coverage, and release packaging gates. An OS kill between checkpoints can still lose the most recent uncommitted interval; a failed host write cannot be made durable by the module alone. Forced teardown after an unresolved failure retains the previous committed disk save but cannot retain process memory.
