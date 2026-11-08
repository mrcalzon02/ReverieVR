# Snake pointer control confirmation

**Confirmed in the unchanged production module: clicking the visible UP or DOWN button center starts Snake but leaves it moving RIGHT.** Pointer clicks farther above/below, within the actual implemented hit regions, correctly turn UP/DOWN. This is executable confirmation of the source mismatch identified in `host-menu-followup.md`.

Audit baseline: `12d4018c90e56a3767b8ffc58a2e5394440caec1`; current checkout `8a5196aa97a73cdae1393e27529bd51914fc1b0f`. No production edits, broad suites or workflows. One probe, four cases.

## Executed results

| Pointer target | Intersection with module input plane | Desired direction after click | Actual direction after next movement tick | Head cell | Result |
|---|---:|---|---|---|---|
| Visible UP background center `(0, +1.02, -2.97)` | `y=+0.999394` | RIGHT | RIGHT | `77 → 78`, expected `65` | **FAIL** |
| Visible DOWN background center `(0, -1.02, -2.97)` | `y=-0.999394` | RIGHT | RIGHT | `77 → 78`, expected `89` | **FAIL** |
| Actual upper hit-region control `(0, +1.50, -2.91)` | `y=+1.50` | UP | UP | `77 → 65` | PASS |
| Actual lower hit-region control `(0, -1.50, -2.91)` | `y=-1.50` | DOWN | DOWN | `77 → 89` | PASS |

Every case recorded `fresh_edge=1`, `released_after_click=1`, `moving_after_press=1`. Build exit **0**; regression probe exit **1** because both visible-button expectations failed. The Python capture runner exited 0 after recording that failing child status; it is not a passing gameplay test.

## Explicit fixture and limits

The probe includes the actual `classic_games_module.cpp`, compiled with `CLASSIC_GAME_KIND=1`. Each case calls fresh `Create(nullptr)`; no game state, heading, position, score or RNG is directly modified. The module's normal initial state has heading RIGHT, stopped movement, and head cell 77.

The pointer originates at `(0,0,0)` and is normalized toward the listed world coordinate. Inputs use `REVERIE_NATIVE_POINTER_TRACKED_CONTROLLER` and complete ABI-sized structures. The primary sequence is release → press → release (`0,1,0`) with `.02s` deltas, followed by three `.1s` real `Update` calls, sufficient for one default `.24s` movement tick. No alternate game simulation or hit-test implementation is substituted. The computed input-plane intersection is diagnostic output only; production `Update` performs the actual hit test.

The two positive control cases establish that fresh pointer edges and vertical steering work when aimed at the implemented regions. Unused graphics functions are linked out. This is an actual module input/simulation test, **not** an Android/GLES rendering or physical controller test.

## Source cause and required repair

`classic_games_module.cpp:1015–1021` draws UP/DOWN background centers at `y=±1.02`, height `.36`, depth `-2.97`. Text is at depth `-2.885` (`127–132`); aiming at either text anchor also remains below the magnitude of the direction threshold. The input plane is `z=-2.91` (`1353–1359`), but direction changes require `hy>1.38` or `hy<-1.38` (`1740–1744`). Generic `Press()` still runs after those failed direction tests (`1747`), explaining why the wrong-direction click starts play.

Use shared button geometry for rendering and hit testing, then retain this visible-center/control-region regression. Do not compensate by changing Snake's reversal or movement rules.

## Reproduction artifacts

- `snake_pointer_probe.cpp`: single diagnostic source.
- `snake-pointer-results.json`: exact compile/run command arrays, output, exit codes and source SHA-256.
- `snake-pointer.log`: complete four-case execution log.

Build flags: `g++ -std=c++17 -O1 -pthread -ffunction-sections -fdata-sections -Wl,--gc-sections -DCLASSIC_GAME_KIND=1`, with the existing scratch Khronos include directory and production native include directory. Exact absolute paths are preserved in the JSON. Original classic report and prior evidence remain immutable.
