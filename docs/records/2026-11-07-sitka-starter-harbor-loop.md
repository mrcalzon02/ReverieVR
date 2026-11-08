# Sitka starter and harbor loop audit — 2026-11-07

Scope: establish the basic shore economy and usable interaction instructions before boat polishing/acceptance. Starting main: `c10ddd72cfd5150646c855f25e44955490d233c7`.

The owner confirmed there are no old saves. This iteration uses clean-start `SITKA-FISH-5`; it does not migrate older prototype saves. Ordinary save/reopen within V5 is tested.

## Implemented behavior

- A genuinely new game receives a Tier 0 pole, reel/spool, line, basic bait, shore kit and dinghy; one coho, one sellable shellfish, and one starter upgrade token. Inventory is persisted once on fresh creation, not replenished on resume.
- Basic bait is infinite and starter gear has no wear or repair cost. There is currently no finite equipment-durability system at any tier. The fallback cast path is exercised through 2,000 cast/cancel cycles.
- Landing a fish puts it in inventory. Lifetime species counts remain separate. No automatic credit award occurs on landing. The harbor sells one fish per fresh Select, using the existing species values; shellfish sells for 25 game credits. Full wallets reject sales without consuming inventory.
- The existing source-anchored proxy harbor desks now open at the sale service. Stick browses, Select confirms, Back closes. Services register pending finds/tokens and expose existing hull, fitting, equipment and four functional character-training tracks.
- A find must be registered before it grants upgrade access. Registration grants no money. Unique site records prevent duplicate turn-ins; the supplied starter token has its own field and does not impersonate a geographic discovery. Living wildlife and cultural finds remain observations, never sale inventory.
- First rod/reel/line/lure/shore-kit upgrades cost 35/28/23/21/26 credits. One registered starter token unlocks the first level. Further levels still require more registered discoveries/types and, for advanced training/hulls, city visits.
- The player can walk while idle, then stops moving while fishing or browsing. Office selection consumes the same press so opening/buying/selling cannot also cast. Held Select cannot repeat transactions.
- The existing licensed Lato panel now shows on-shore inventory, Tier 0 supply guarantees, cast/wait/bite/reel/tension/landed instructions and office interaction prompts. Selected shop rows show current costs, levels, prerequisites or empty inventory. Save failure is visible; dirty state retries on pause/exit.
- Deckwork training is hidden from this shop because its separate crabbing simulation is not connected to this module. Its schema/simulation definitions remain for future integration.

## Evidence and its limits

`python3 scripts/sitka/verify_gameplay.py --render` compiles and executes 37 Sitka programs with GCC C++17 warnings as errors. GLES checks require EGL/GLES development libraries; headless Mesa uses `EGL_PLATFORM=surfaceless LIBGL_ALWAYS_SOFTWARE=1`. `CPPFLAGS`, `CXXFLAGS` and `LDFLAGS` can locate a private toolchain without system installation.

The new `sitka_starter_loop_test.cpp` includes the actual module implementation and calls its Update with normalized input and a host save store: out-of-reach rejection, walking to the desk, hold protection, sale of both starter catches, token registration, live-menu rod purchase, save/reopen, pending-new-find gating, forced write failure and pause retry. It also exercises the permanent starter fallback.

The new `sitka_starter_loop_egl_test.cpp` calls actual module GL creation, Update, both eye renders and context-resource recreation on Mesa. Pixel readback requires visible text in both eyes, changed shore/office/sale frames and unchanged journal across repeated stereo rendering. Its map, water and harbor are synthetic fixtures; it bypasses packaged asset startup and certifies no real harbor route.

The existing 34 CPU checks and terrain/boat/bridge GLES suite remain. The two new integration programs are attached to the existing content-matrix GLES job; no extra workflow or APK release trigger was added. Remote CI, Android packaging and handset acceptance require their own evidence.

Native ABI contract passes. The documentation synchronization guard reports the pre-existing unrelated Test Chamber shader-helper mismatch (`ReverieNativeCompileShader`). This iteration does not label that guard green or modify its validator.

## Remaining implementation and acceptance gaps

| Requirement | Audited state | Required next work |
|---|---|---|
| Actual Sitka launch in the unified APK | Not registered in Android.mk/host/gallery | Assemble approved world assets, integrate the existing module and prove create/exit/recovery |
| Walk to nearby real harbor offices | Terrain collision and desk targeting implemented; synthetic route exercised | Install terrain-matched office entrances/desks and prove connected walkable routes in the packaged world |
| Moving player/characters | Grounded player locomotion exists; no Sitka visible-avatar/NPC adoption | Wire shared avatar/character representation and verify movement/collision in the world |
| Fish and shellfish inventory/sale | Starter and landed fish retained; both sale paths integrated | World shellfish harvesting and pot catch integration remain separate |
| Casting presentation | Live status/instructions now rendered | Physical pole/line/bobber/fish visuals and richer fishing audio remain unfinished |
| Equipment progression | 48 functional gear levels and four functional training tracks | Complete content gates, balance and headset readability/input acceptance |
| Boat hull/fitting purchases | Existing saved purchase path and runtime rebuild retained | Verify embark, handling, collision, helm controls, sounder and dock return in the actual world |
| Refined 18 boat models | Source assets/galleries only | Runtime rendering, mounts and collision adoption |
| Pot crabbing/Deckwork | Separate CPU prototype | Connect one journal/wallet, retained bait, world pots, work loop and sale UI |
| Headset/thermal validation | Unaccepted | S9 + Daydream controls, text, stereo, comfort and sustained performance |

The first shore loop is locally exercised, not a finished/playable Sitka release. The next iteration must supply and admit the real world/harbor slice, then run its startup and route interaction end to end before declaring boat acceptance ready.
