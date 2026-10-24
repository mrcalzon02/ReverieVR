# Home avatar wardrobe — design and integration

**Requirement established: 2026-10-24.** ReverieVR has a complete shell-owned, offline avatar identity rather than a cosmetic fake button.

## Experience
The existing white-room mirror gets a neighboring low-poly wooden wardrobe with physical gaze/controller ray interaction. The **Avatar Wardrobe** Home button provides access from the forest and beach environments as well. The same shell input actions and five-second controller-free gaze fallback select targets, with ordinary Back and Quick Menu recovery.

Twelve initial identities: Everyday Dreamer, Astral Ranger, Forest Warden, Deep-Sea Diver, Clockwork Robot, Candy Admiral, Moon Witch, Pixel Paladin, Noir Sleuth, Friendly Phantom, Retro Racer, Cozy Critter. Player customization independently adjusts palette (8), headwear (8), accessory (8) and eyes (4). Each identity initializes a different themed ensemble. Randomize previews a draft; Apply/Save commits the choice to a versioned local SharedPreferences profile, and Back/Discard cancels changes. Invalid/newer save formats fall back to default.

The existing planar mirror still renders headset/controller diagnostics. Its additional low-triangle avatar figure is a **stylistic preview**, not simulated full-body tracked motion or a full optical reflection. Preview changes live in the editor: a small 2D thumbnail is always available in the editor, including forest/beach scenes, while the birch-room mirror shows the larger silhouette. Committed state is used afterward. These starter assets are authored in first-party code, not unrelated stock sprites, and require no network service, Google dependencies or additional native game module.

## Expansion contract
Future owned/licensed 3D models may replace the starter silhouettes. Normalize rig scale, skeleton and named attachment slots for body, outfit, hat, accessory and face. Use S9-friendly geometry/material budgets with LODs; never ingest a whole purchased source pack. A future versioned read-only cosmetic descriptor can be exposed through the native module SDK after its compatibility and ownership contract is defined. Cosmetic preferences must not alter a game's economy, combat or saves.

## Acceptance gates
- Wardrobe is physically next to the mirror; controller click and gaze-dwell can open it.
- Home button works in all environments and Back/Quick Menu always recover.
- All 12 themes and every cosmetic selection render differently and have working controls.
- Apply persists through app restart, Back cancels, Surprise Me only changes draft.
- Mirror diagnostic geometry remains visible and aligned per eye.
- Android unit tests and APK build must pass; S9/Daydream headset validation must confirm readability, occlusion, stereo, input, recovery, rendering budget and warmth. **Neither build nor physical headset validation is claimed by source commit alone.**

## Locomotion / IK extension (24 October 2026)

Home-room and DOS-library travel speed doubles **again** from the previous
1.4 m/s to 2.8 m/s at full analog deflection, using the existing
`BoundedViewRelativeLocomotion` speed multiplier 4.0× of its 0.70 m/s
base. Native-game speed multipliers remain at 1.0×, the movement bounds
remain unchanged, the 28% touchpad deadzone stays intact and movement
still halts immediately on release, menu focus or controller loss.

The avatar preview's lower body now has analytic two-bone IK, with a
thigh, shin, stable knee pole, alternating gait targets, ground-contact
stance and foot lift. Motion is driven by **actual bounded locomotion
displacement**, not by a merely held touchpad; standing at a travel
boundary returns to idle. Joint and mesh scratch buffers are reused on
the GL thread and updated once per headset frame for both eyes.
The Daydream headset provides no real-world ankle tracking; this is a
procedural gait rig and vanity preview rather than a true body-tracking
system. Full 3D skinned avatar adoption remains an art/integration task.
