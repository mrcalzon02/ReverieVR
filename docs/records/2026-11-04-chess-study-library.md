# Source-cited native chess strategy library — 2026-11-04

## Source audit
The native chess implementation already had an immutable
`classic_chess_book.h` with **3,865 Lichess CC0 opening records**
(ECO code, name and full SAN), an opening/variation lookup, move
history, and **nine** short rules articles in a five-tab reader.
It did **not** include a substantive, independently cited catalogue of
chess tactics, position plans and endgames. Opening names and SAN lines
are descriptive opening metadata, not strategic explanations.

## Integrated correction
The game-private `classic_chess_study.h` contains 34 original
offline articles: 13 positional lessons, 18 tactical patterns, 3
endgames. Each topic stores a recognizable position cue, concrete
action plan, defensive caution, explicit source name and full source
URL. A new sixth `STUDY` tab uses the existing 3D text batching and
ray interaction: bottom +/-1 and +/-10 changes topic, ARTICLE arrows
advance the *contents* of that topic. Original opening, variants,
move history, rules, and ECO index continue to work. No additional
menu, texture, network requirement or engine evaluation was added.

Every URL was checked against its public source topic before initial
inclusion. The textual instructions are **original summaries**, not
copied Wikibooks chapters or Lichess practice puzzles. A source URL
does not authorize copying its images, code, or puzzle data.
Source provenance:
- ECO factual database and license: https://github.com/lichess-org/chess-openings
  — CC0 public-domain dedication, existing in-app opening source.
- Lichess's 21 individual practice/endgame lessons:
  https://lichess.org/practice — source topics linked individually in
  the study header, NOT assumed CC0 merely because the opening index is.
- Wikibooks chess strategy chapters:
  https://en.wikibooks.org/wiki/Chess_Strategy — CC BY-SA 4.0/GFDL
  content documented at https://en.wikibooks.org/wiki/Wikibooks:Copyrights.
  The game cites the resource instead of importing/adapting its prose.

The in-headset URL is rendered over several article lines; there is
no implied clickable browser integration while playing offline.
Bibliographic credit remains visible even without internet access.
No article asserts that a tactic is legal or winning in the current
board position; chess move legality remains the game engine's job.

## Acceptance
The pre-existing `classic_games_test.cpp` now also tests all 34
entries for nonempty long-form cue/plan/caution, unique titles, fixed
category counts, named source and allowed source URL prefix. This
continues to run in the existing classic-games functional CI job;
no duplicate runner/workflow is required. A same-change source/UI
review verifies the new tab has hit targets and the old tabs survive.

**Not yet demonstrated:** latest Android APK compilation, 34-article
optical readability and cursor hit testing on Galaxy S9, external
source link stability over time, or live FEN/puzzle demonstrations.
Next stages: carefully authored ECO-specific strategic descriptions
linked by opening index; verified legal FEN/SAN examples and per-position
puzzle explanations with explicit rights/attribution; then S9
binocular reader, text pagination and performance acceptance.
