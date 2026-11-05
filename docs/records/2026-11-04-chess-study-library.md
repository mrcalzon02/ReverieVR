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

## Follow-up: citation completeness
Each of the 3,865 existing opening-index records now resolves to its
specific, inspectable source TSV (`a.tsv` to `e.tsv`) in the Lichess
CC0 repository. The original SAN sequences are unchanged. The RULES
tab displays a FIDE 2023 chess-law URL on the standard-rule pages,
cites the CC0 opening dataset on the catalogue/analysis page and
separately labels ReverieVR-specific controls without misattributing
those controls to FIDE.
Official FIDE reference: https://handbook.fide.com/chapter/e012023.
The existing classic-games regression also verifies A/E source-volume
routing and an invalid ECO's safe fallback.

## Staged completion contract — what will make this a complete library

**Stage A — audited reference and cited lessons (implemented in source).**
Preserve all 3,865 Lichess opening records exactly as historical CC0
facts, each linked to its ECO TSV. The 34 independently authored lessons
cover tactical mechanisms and broad positional principles. The content
is offline and the native chess engine remains authoritative. Gate:
all indexed pages remain reachable, the 34 articles contain named
sources, and the existing classic-games suite passes. Emulator/headset
acceptance is separately required.

**Stage B — annotated opening families (planned, not yet implemented).**
Start with common families across ECO A through E, not 3,865 shallow
templates. For each: classify opening and its move-order transpositions,
show one verified legal main line, explain strategic objectives for
*both* White and Black, identify pawn breaks and characteristic piece
placement, note realistic deviations and what each side should avoid.
Attach a durable URL/license/retrieval date to every claim and example.
No unsourced book popularity presented as a move probability or
evaluation. Only after quality checks, expand coverage systematically
by ECO family and variation. Ensure that continuing from a position
does not falsely claim a unique opening after transposition.

**Stage C — actual example positions and exercises (planned).**
Curate a *small* referenced set of source-authorized tactical and
endgame problems from public licensed chess material, e.g. the
Lichess CC0 downloadable puzzle database, not Lichess's
interactive lesson text/art. Store original source ID, its immutable
URL, source snapshot/hash and exact position (FEN) plus UCI/SAN
solution. Author a short explanation of why a candidate move works
and how tempting alternatives fail *after independent verification*.
Validate source positions for kings, legal side-to-move, promotion,
castling rights and en passant; replay the full candidate solution
through the **same native chess legal-move rules** used in live play.
Do not label puzzles 'verified' on metadata alone.

**Stage D — board-integrated instruction (planned).**
Render a legal reference position on the existing chess board or a
small separate study board with Back/Next through verified plies.
Keep practice-state and actual match/save distinct. Never change the
live game while reading a strategy article. Map the current position
to relevant study concepts when reliable, without inferring a tactic
merely from a matching name. Preserve controller, gaze and keyboard
navigation and the compact 3D text layout.

**Stage E — authenticity and performance gates (planned).**
Add content validation to existing classic-game tests: source file
license and hash; unique IDs; URL provenance; FEN parsing/legality;
exact multi-ply replay; read-only save/game-state isolation;
paged index bounds, article-page scroll and browser-less offline
operation; Android ARMv7/ARM64 inclusion, a real 3D study-page capture
and physical S9 stereo readability. Target zero broken controls,
zero silently truncated citations, and no unverified moves. Profile
memory, native binary growth, allocation behavior and frame pacing
before increasing dataset size. Do **not** introduce a duplicate CI
workflow or disguise functional failures as optional visual gates.

**Stage F — maintenance and progress ledger (planned).**
Snapshot approved input data with upstream commit/hash/date and
license, while retaining per-entry citation and source volume.
Keep a machine-readable admission report distinguishing imported CC0
facts, independently authored instructional summaries and future
source-derived exercises. Progress counts must distinguish:
(1) indexed opening lines, (2) annotated opening explanations,
(3) verified tactical exercises, (4) finished endgame guides.
Only publish a 'complete library' claim when the committed content,
installed APK, and real-device reader have all passed these gates.

The immediate next technical unit is Stage B's **one complete,
source-backed annotated opening-family vertical slice** with a
real legal sequence, corresponding independently verified
position, both-player plans and complete references. Only after
that exact slice compiles and works in the headset should the
bulk family import proceed.
