# Omega Front — Menu Chain, Controls, Campaign and Playability Audit

**Date:** 2026-10-28  
**Scope:** The shipping native `omega-front` module inside the ReverieVR hub, its Android/Daydream controls, 16-sector Hearthfall campaign, 24×24 tactical battles, persistent save and test chain.  
**Important distinction:** This is an implementation audit, not a claim that the planned seventy-arc grand campaign or a completed retail-quality user experience exists. Functional tests, GLES2 emulator checks and physical-controller checks are distinct evidence gates.

## Continent/region and turn-based battle update — 2026-10-28

After entering Campaign, the initial strategy view opens to a textured **30×20 continental overview**. **REGION** switches to the saved sixteen-sector Hearthfall operation map; **WORLD** returns to the continent. Clicking the continental body enters the playable district, but does not execute an unrelated march or spend campaign resources. The rest of the 600 geography cells are not claimed to be playable provinces. Regional terrain costs matter (rugged sectors consume an extra ration unless a real transit treaty waives the cost). All sixteen serialized sector IDs and v9 save positions remain unchanged.

Tactical combat is **no longer real-time**. The 24×24 board stays still indefinitely while orders are selected; friendly action queues and enemy AI execute only after explicit **END TURN**, available as a top-center clickable control and a fifth choice in the existing **MODES** menu for touchpad-only operation. The End Turn command resolves one turn, then pauses for further player decisions. The old tactical pause is a retained legacy hold option, not a running clock. Long-minus remains the command menu, and end-of-battle clicking still opens the real after-action result. An individual turn is checkpointed immediately; holding primary cannot execute more than one resolution. Native tests do not substitute for physical Daydream pointer/menu validation. [Detailed world and battle contract](OMEGA_FRONT_TACTICS.md).

## 1. Root cause

Before this pass the native game opened **directly in the sector map**, which was both the start screen and the active strategy interface. Players had to discover that the minus/secondary button cycled all eight strategic actions; a different behavior applied to the same button during tactical combat. There was no in-game controls page, safe title screen, deliberate Continue/New campaign choice, visible pause navigation, or direct order palette. The underlying `Session`, `Frontier`, `Controller`, `Battle`, `CampaignSave`, asset renderer and native SDK are substantial, but their functionality was not exposed by a coherent menu chain.

## 2. Implemented navigation contract

```text
Native games browser (ReverieVR hub)
  └── Omega Front → TITLE
       ├── CAMPAIGN → CAMPAIGN DOSSIER
       │    ├── BEGIN CAMPAIGN (no existing save)
       │    ├── CONTINUE CAMPAIGN (existing session)
       │    ├── NEW CAMPAIGN (existing save) → ERASE CONFIRMATION
       │    │    ├── ERASE AND START NEW → STRATEGIC MAP
       │    │    └── CANCEL → CAMPAIGN DOSSIER
       │    ├── CONTROLS → back to DOSSIER
       │    └── BACK → TITLE
       └── CONTROLS → back to TITLE

STRATEGIC MAP [16 live sectors, actual intelligence/claims/supply/ownership]
  ├── ORDERS tab → eight explicit strategic orders → map
  ├── MENU tab / long-minus → PAUSE MENU
  │    ├── RESUME → current battle/map
  │    ├── CAMPAIGN BRIEFING → DOSSIER → CONTINUE
  │    ├── CONTROLS → back to PAUSE MENU
  │    └── MAIN MENU → TITLE
  ├── adjacent travel / scouting / logistics / negotiation
  └── eligible DEPLOY → TACTICAL BATTLE [24×24]
       ├── select unit → set command mode → click order destination/target
       ├── MODES tab → Move / Attack / Repair / Resupply → battle
       ├── MENU tab / long-minus → PAUSE MENU
       └── completed battle → click after-action result
                                → resolve casualties/resources once
                                → strategic map or immediate counterattack
```

The interface is drawn with the existing real Lato text renderer, not a symbol-only surrogate font. The menu has explicit focus, selectable rows, short titles, highlighted selection, pointer hit zones and thumbpad navigation. Its lifecycle is **ephemeral**: the save contains campaign state, not a paused menu screen. Relaunching the module returns safely to Title rather than issuing a tactical command as soon as the game resumes.

**Warning:** The menu is currently an in-game panel, not a replacement for the ReverieVR native-game browser. Returning from Omega Front all the way to the hub remains host-managed; an unsupported fake “Exit” button is deliberately not shown.

## 3. Physical control and accessibility map

| Context | Touchpad axis | Primary / click | Secondary / minus | Additional |
|---|---|---|---|---|
| Title, dossier, confirm, controls | Move highlighted item up/down | Activate highlighted row | Back | Aim at row to focus it |
| Strategic map | Move 4×4 sector cursor | Execute selected order at focused sector | **Short tap** cycles eight orders | **ORDERS** tab opens direct list; **MENU** tab opens pause |
| Tactical battle | Move 24×24 tile cursor | Select a green unit, choose destination or target | Short tap cycles four order modes if selected; otherwise toggles pause; resets only after result | **MODES** tab directly selects all four modes |
| Any active map or battle | — | — | **Hold minus ≥ 0.65 s** to open menu | Native simulation stops processing while menu is open |
| Completed battle | — | Click report | Original result reset behavior retained | After-action application remains a one-time transition |

Strategic actions (existing mechanics) are **DEPLOY**, **FORAGE / CONVOY**, **RECRUIT**, **REPAIR / SECURE**, **WAIT**, **SCOUT**, **PARLEY** and **ACCORD**. They are not universally legal in every sector. The map retains route, intelligence, ration, battle, treaty and resource checks; a highlighted action is a selected *intent*, not a bypass of the rules.

Tactical actions are **MOVE**, **ATTACK**, **REPAIR SELF**, and **RESUPPLY SELF**. The selected mode is now preserved when choosing a soldier, and selecting the soldier again with Repair or Resupply queues the intended self-action rather than deselecting. The underlying battle engine decides legality and available resources. There is no cosmetic menu-only success.

**Fallback:** Both point-and-click row selection and thumbpad movement are available. No secondary-controller tracking, external keyboard or location permission is required for navigating this native module.

## 4. Campaign reality versus the grand design

### Implemented in code

- **16-sector Hearthfall sector map**, with terrain appearance, fog-of-war intelligence and actual focus versus current location.
- **Six scripted Hearthfall operations**. `kHearthfallOperations = 6` in `omega_front_frontier.h`; these have authored tactical encounters rather than seventy finished campaigns.
- Multiple faction doctrines, tactical simulations, mission objectives, casualties, resources, timed battle consequences and reports.
- Ongoing raiding, convoy interceptions, outpost attacks/defenses, relationship/accord logic and sector claim pressure beyond the six scripted encounters.
- Campaign checkpoint stored at `omega-front/frontier.v1`, fixed **2,048 bytes**, schema up to 9 with older-version compatibility; in-progress tactical encounters can persist.
- Shared PNG assets, source catalogs and faction unit sprites, mixed with meaningful 2D strategy and tactical state rather than decorative pseudo-3D hit targets.

### Planned, not present as a finished campaign

The canonical `docs/project/OMEGA_FRONT_GRAND_CAMPAIGN.md` defines **70 faction arcs across 10 movements**, generally 3–8 meaningful battles per arc, with a long-term goal of **600 sectors across 12 theaters**. The current six-operation/16-sector loop **does not satisfy that plan**. The campaign dossier accurately identifies the *current* Hearthfall theater instead of advertising seventy playable arcs that do not yet exist.

Remaining work for a full campaign menu must derive available arcs from real unlocked theater state; show objectives, faction, reward, supply cost, prerequisite, force strength, mission history and an explicit launch/abandon flow; provide a stable roster, upgrades, equipment, logistics and progression page; and preserve all of those choices across campaigns. Do **not** add fake campaign cards or inert equipment buttons to make the UI appear complete.

## 5. Playability and reliability gates

### Automated gates now in source

1. `app/src/test/native/omega_front_menu_test.cpp` traverses Title → Campaign → Begin; pause/controls; direct field and tactical order selection; cancel/confirm New; Continue; return to Main; touchpad-only focus; pointer hit zones.
2. The existing `scripts/verify-omega-front-playability.sh` discovers and builds **every** `omega_front_*_test.cpp` including the new menu test, plus previous campaign/save/combat/mission tests. Its exit status must remain a functional release signal.
3. `DownloadedNativeGameAssetsInstrumentedTest` now explicitly traverses **Title → Campaign → Begin** before clicking map cells and **Title → Campaign → Continue** after an Android runtime relaunch. It still checks real GLES2 frame changes, actual battle-start audio, army movement, checkpoint size and saved-battle pixels.
4. The existing ReverieVR hub CI validates that the native module is present and launchable and captures native menus. No new GitHub Action workflow or parallel game packaging system is required.

### Manual headset acceptance — not yet claimed

- S9 Android 10 Daydream controller and other supported Android input paths: no flickering selection, no accidental dual action on a held button, correct pointer zone, no gaze-only fallback while the controller remains active.
- First launch/no save: correct title frame, true Begin → sector map, no gameplay tick or resource consumption in menu.
- Existing checkpoint: Continue restores exact strategic/tactical state, including mid-battle; New requires second confirmation and replaces only Omega Front's save.
- Action bar: every strategic command can be selected directly; unavailable actions return legible reasons; scouting fog stays authoritative.
- Tactical bar: every command changes simulation behavior, including clicking selected soldier to Repair/Resupply when legal; all denied orders produce failure feedback.
- Battle complete: one after-action resolution; correct casualties, faction consequences, next mission or counterattack; no duplication after restart.
- Pause/lifecycle: simulation freezes in UI menus, resumed state is identical, exits do not corrupt the checkpoint.
- In all cases: text does not overlap in left/right eye views, both eyes render the same logical scene, no invisible hit targets.

## 6. Next development phases (not falsely marked complete)

**P1 — interaction and emulator verification:** confirm source compiles on the Android NDK, run the entire Omega Front headless suite, inspect real GLES2 capture including menus, repeat on Galaxy S9. Fix any functional failures before shipping.

**P2 — discoverability and errors:** per-action descriptions and context-dependent legality reasons; expandable map intelligence panel; campaign objectives without covering game space; persistent sound/selection preferences only after they genuinely operate.

**P3 — campaign UX:** build operation planning, faction chronicles, mission history, roster, tech, supply, loss/recruitment and achievement views on top of real `Frontier` state. Every page must be reachable from the campaign or pause screen and must have at least one meaningful operation.

**P4 — world expansion:** implement the documented additional sectors/theaters and 70 substantial arcs as executable simulation and narrative data, with save schema migration, deterministic battle/strategy tests and separate real-play pacing measurements.

**Release acceptance:** code commits alone are not Android/Daydream verification, and menu cosmetics must not be promoted into unrelated release blockers. Functional launches, controls, battle transitions, saves and APK integration are required; visual polish remains iterative.
