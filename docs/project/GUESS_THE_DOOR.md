# Guess the Door — native ReverieVR memory game

**Status (2026-11-04):** Development-only source stage. Android builds,
S9/headset runtime, requested spoken line and Mechanist sprite import
remain pending, not accepted.

## Gameplay
Twelve seeded rooms. A brief 2.3-second clue describes a **visible
physical feature** of the safe door. The procedural door model exposes
five independently seeded families with two visible choices each:
OAK vs IRON surface, plain PANELS vs CROSS BRACES, NO RIVETS vs FOUR
RIVETS, RING PULL vs BAR HANDLE, and CLEAN vs THREE SCRATCHES.
The clue is derived from the safe door's actual feature value, then the
other door's corresponding feature is made different. No arbitrary
SUN/MOON/KEY labels, invisible answers, or false matches. Staring at a
door opens an inspection popup describing its observable materials and
hardware without revealing success. Pull the matching door's lever
(pointer ray + primary); rocker left/right + primary works without a
usable pointer. Slow animations:
lever 0.7s, opening 1.5s, forward traversal 1.6s, pit fall 1.8s.
Incorrect choices fall into a pit and restart at room one **with the
same seed** so players can memorize the route. After twelve safe
choices, the player can start a newly seeded challenge. HUD displays
seed, room and falls. Room RNG is domain-separated, so modifications to
visual decoration cannot rewrite established safe choices.
Headset yaw is never forcibly rotated and shell recovery remains intact.

**Easter egg**: on the second room, pulling the right lever first
displays the requested caption "PULL THE LEVER, CRUNK!" during the
opening transition. It is currently caption-only; a self-recorded or
licensed spoken file and voice playback are not yet implemented.

## Existing Mechanist art (located, not yet imported)
Source: `mrcalzon02/TheMechanist`, `main`:
- `ROOT_SRC_assets/Mechanist_art_SRC_do_not_MODIFY/Mechanist art/TILES/Doors/Doors-C.png` (Git blob `26bc604d8e967971cb28985b7cee831c92630981`).
- `ROOT_SRC_assets/Mechanist_art_SRC_do_not_MODIFY/Mechanist art/TILES/Doors/Doors-O.png` (Git blob `8347cf04f331b02a73357c480ce8e7fb8f12296e`).
- Matching road-door and hatch sheets are adjacent.

The available GitHub binary reader could not return those PNG bytes.
The prototype uses compact vector-colored 3D doors and hinge
animation; it **does not** use actual Mechanist art yet. The
next step is to retrieve and visually inspect the original sheets,
match closed/open frame rectangles, verify ownership/license, crop
and pack approved pairs into one small texture atlas, admit
only exact paths in the game's bounded asset scope, and confirm
GLES2 stereo rendering and Device reference performance.

## Sound
The native host's existing Red Ledger metal, door, click and drawer
cues are requested for lever, opening, temporary footsteps and pit;
the native victory cue marks a clear. These are temporary audio
feedback, not original walking/pit Foley, and no vocal movie
sample is included.

## Acceptance gates
1. Run `scripts/verify-guess-the-door.sh`; pure C++ regression checks
   deterministic feature clues and unique matches, double-pull lock, progression, pit,
   seed restart, victory, and Easter trigger.
2. Build the Android NDK & APK. Confirm registered library and
   Development-only gallery launch; verify legacy games unchanged.
3. Test slow transitions, gaze tooltip, lever targeting, audio
   and shell Quick Menu on Galaxy S9 + Daydream controller.
4. Integrate checked source-door atlases and licensed audio speech,
   then run stereo, accessibility, FPS, battery and thermal tests.
