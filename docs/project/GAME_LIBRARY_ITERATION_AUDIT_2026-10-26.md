# ReverieVR integrated game library — iterative completeness audit

**Scope/authority:** `mrcalzon02/ReverieVR` main, the exact 15 titles in `modules/game-identity-matrix.json`; compare actual source against `docs/project/` design contracts. This is a living engineering queue, **not a statement that everything is finished or Galaxy S9 tested**.

## Verified baseline

- [All 8 functional CI groups #40400854855](https://github.com/mrcalzon02/ReverieVR/actions/runs/40400854855): **SUCCESS**. Strict GCC/Clang game simulation checks for Breakwater, Red Ledger, Gumball, Omega, the eight native classics, both reconnaissance projects and shared native ABI.
- [Installed Android hub #40400318092](https://github.com/mrcalzon02/ReverieVR/actions/runs/40400318092): **SUCCESS**. 12 Android instrumentation tests, every one of 15 packaged game modules actually starts and produces an individually checked 512×512 real JNI/GLES2 framebuffer. These prove boot/first render, not completing entire games on hardware.
- Exact real game inventory: **8 classics + 7 other natives** (Breakwater Battery, Red Ledger, Gumball Armada, Omega Front, Lantern Desk, Iron Sight, Procedural Test Chamber).
- Separate DOS archive titles DOOM Shareware, Ken's Labyrinth and Jetpack **not** included in the 15 native modules; need actual DOSBox executable launch/framebuffer acceptance later. Existing fetch/checksum gates are not DOS gameplay proofs.

## Classic game audit (rules and progression)

| Native classic | Existing executable progression assertions | Remaining truthful boundary |
| --- | --- | --- |
| Pong | Actual left/right paddles, hit-angle plus measured paddle-velocity transfer, point scoring, first-to-11 both ways, restart, deterministic split-step timing | CPU tuning, S9 hand/controller feel and sustained FPS |
| Snake | Deterministic food selection excluding entire body, four-way movement, growth, wall/self collision and restart; near-full/full 144-cell board checks | S9 comfort, small-screen direction UI; no unsupported persistent campaign |
| Chutes & Ladders | Full 100-cell board and 19 actual jumps, exact-100 landing, human vs delayed CPU turn, forced overshoots, complete auto-opponent match, terminal restart | S9 legibility and dice cadence, not missing progression |
| Checkers | Mandatory jumps/crowning, completed multi-capture, repetition/40-move draw, progressive bounded CPU search, terminal/reset, difficulty preserved | Agreed draws, resignation/clocks and headset responsiveness still absent |
| Go | Full 19×19 superko and life/death manual review, group mark/unmark, area scoring, komi, resume, pass/review and bounded heuristic CPU | Automatic life/death/seki adjudication and advanced Go AI **unfinished** |
| Chess | King-safe legal play, promotion/castling/en passant, SAN, opening reference, 50/75-move and repetition draws, check/mate, asynchronous CPU, terminal/reset | Negotiated draws/clocks and real S9 search latency remain open |
| Breakout | 112 bricks, two walls, three lives, speed/paddle mechanics, partial score, win/loss, restart | S9 controller sensitivity and complete long-run device acceptance |
| Reversi | Proper 8×8 flips, forced pass, CPU legal moves, full-board/blocked terminal scoring, winner/draw and restart | S9 pointer precision and AI quality ceiling |

**Finding:** These eight have substantial real rules and progression, not inert previews; no arbitrary rewrite is warranted. Next useful feature work should be an isolated specific missing rule, verified against the original game's chosen rules, not a broad “fix classics” patch.

## Other native game gap ranking

1. **Red Ledger / Between Deliveries — foundational gameplay progression gap:** [Core Customer-Service Design](BETWEEN_DELIVERIES_CORE_SERVICE_DESIGN.md) says supplier-specific product preferences, repeating clientele, cleanliness expectations, disruptive behavior and upgrades must influence a persistent bar ecology. Current `Trade::RateService` already rewards requested products; `PatronEnvironment`, `CleanlinessAfterService` and `AcceptsCleanliness` existed but were **not connected to live patron interactions or saved state**. Phase 1 now commits `red_ledger_clientele.h` plus in-module true service integration, a physical bar-condition HUD readout, atomic format extension and v4 load compatibility. It persists six affinity tracks and actual venue condition, reduces premium reputation when demanding patrons dislike venue condition, and uses purchased interior levels for wear resistance and between-day recovery. **Still missing:** attracting/displacing future customers based on the new affinity/cleanliness history, customer fights/retention and broader world/narrative economy. New strict host test and Android capture gate **pending completion**; do not claim this feature accepted until both pass.
2. **Gumball Armada — partial 3D and campaign:** live source has connected 7×7×5 shipbuilder, shared enemy assembled geometry, highlighted branching three-lane map with 350ms deliberate second-click launch, finite waves and deterministic assault formations. Full 3D collision/hit volumes, developed multi-phase commander bosses, true fleet-building and rescuing the missing Candy King remain incomplete according to [design](GUMBALL_ARMADA.md). Do not falsely resolve the King upon defeating the prototype generals. Prior exact Android map state captures and host simulation tests green; S9 still open.
3. **Omega Front — strong early vertical slice, enormous unbuilt grand strategy:** live tactical campaign has validated first-theater operations, four faction doctrines, logistics and actual sprite rendering; [70-campaign design](OMEGA_FRONT_GRAND_CAMPAIGN.md) explicitly labels the **full 70 faction arcs, 210+ meaningful battles, 12 theaters and Level 100 × ten Reclamations** as proposal, **NOT PLAYABLE**. Expand incrementally with distinct real scenarios, save consequences and battle outcomes; never claim the entire grand campaign is implemented based on the current 16-sector slice.
4. **Breakwater Battery — substantial campaign, acceptance and polish:** 3 defense environments × 9 day/night periods, armored landings, ballistic weapons, shop and allied turrets implemented/tested; documented open gaps remain authentic on-device rendering and audio/visual reliability, physically changing gun model per purchased weapon, enemy visibility and balance across 244–352+ waves. New weapon rack/asset work needs actual head/controller tests, not fabricated captures.
5. **Lantern Desk and Iron Sight — compact reconnaissance prototypes:** their existing complete evidence-to-report and patrol-to-debrief *prototype cycles* pass functional tests, but the design contracts describe deeper uncertainty, knowledge segregation, vehicle/dismount prep, supply, faction choices and many distinct missions. Do not label the full Ministry/Iron Sight adaptations finished from passing short missions.
6. **Procedural Test Chamber:** real native GLES2 start/render and independently packaged companion run, but it is a shared asset/environment diagnostic room, not a long story game. Check imported model/sound interactions and scene selection function rather than inventing victory conditions.

## Engineering rules for continuing

- One tightly scoped game function at a time; link implementation to the exact outstanding design requirement and add **functional** regression before or with code. Prefer real battle/service/match progression over cosmetic tests.
- Maintain the all-games workflow, the registry identity gate and the installed Android GLES2 capture; do not weaken assertions or remove currently functioning games.
- Each source slice: preserve original game ID, packaged models/art, save compatibility, Daydream pointer and touchpad, real font glyphs, native GL and 30 FPS design budget.
- Report separately: source committed; host tests passed; Android emulator passed; physical Galaxy S9 accepted; signed updater-valid APK **actually released** (not inferred from a green release workflow gate).
- On a failed write, don't claim commit; on a failed test, diagnose or leave an explicit failure. Don't force-push, create duplicate tests for cosmetic concerns, or launch expensive repeated CI reruns simply to watch waiting jobs.

**Next progression step after phase-1 Red Ledger verification:** use durable supplier affinity and actual cleanliness to influence *next day's patron selection* while retaining deterministic seeds, atomic save migration and a separate multi-day rough-bar/upgraded-bar scenario. Only then implement compatibility/disorder consequences. Parallel classics work should start from a specific observed AI/rules gap, with the existing complete games left intact.
