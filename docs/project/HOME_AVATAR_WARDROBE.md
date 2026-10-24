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

## 24 October 2026 — large wardrobe, body diversity and v2 identity schema

The wardrobe is now defined as a **composable character creator**, rather than
12 identically shaped figures in different hats. Initial authored selection:

| Independent category | Choices |
| --- | ---: |
| Named character themes | 48 |
| Outfit families | 24 |
| Headwear | 16, including None |
| Accessories | 16, including None |
| Hairstyles | 12 |
| Accent palettes | 16 |
| Outfit fabric colors | 16 |
| Eye styles | 4 |
| Body presentations | 3 (feminine, masculine, neutral) |
| Starter body types | 12 |
| Shape dimensions | 5 independently editable 7-step values |

These are separate fields rather than mutually exclusive gender/body/outfit
bundles. Short, tall, slender, curvy, heavily muscular, stocky and heavyset
men and women are possible, as are neutral presentations with any physique.
An exaggerated broad-shouldered “brickhouse” build does not force a male
presentation. Nothing is restricted by a subjective attractiveness score.
The named themes range from fantasy, retro and whimsical roles to heavy
industrial, spacefaring and period identities. Theme choices suggest costume
and accessories; they **do not silently overwrite a customized body**.

The body-profile page includes height, shoulder breadth, waist width, hip
breadth and muscularity. Applying a named starter physique seeds the five
dimensions, after which the player may edit each one independently. The
Outfits page separately controls clothing, headwear, accessories, hair and
garment color. Identity page contains theme, accent color, body presentation,
starter physique and eyes. PREVIOUS/NEXT PAGE buttons are inside the same
existing headset panel; Apply, Back and Surprise Me keep their original
meaning. The compact full-body editor thumbnail works in all home scenes.

### Geometry and saves

The preview's shoulder/waist/hip/muscle axes now build differing meshes,
with visible taper, broadened arms, shaped garments, hair and facial
presentation. Scale adjusts portrait height and anchors the feet; the
existing true-displacement procedural two-bone gait also varies leg
separation, muscle mass and foot width. Static hat/accessory choices are
hand-authored triangles including the eight newly introduced choices
in each group. Meshes are rebuilt **only when body/garment geometry changes**,
before the stereo eye draws; no shader skinning, network assets or new
runtime dependencies are added.

Avatar serialization changes from `v1` to `v2` while retaining the
original SharedPreferences key. Valid v1 strings migrate without losing
the original theme, palette, headwear, accessory or eye selection.
Malformed/future strings fall back to the default. Save remains explicit:
Randomize and stepping only modify the draft until APPLY + SAVE.
The v2 profile remains shell-owned; it does not modify native game saves.

### Validation and unfinished art work

Unit tests cover 48 theme traversal, v1 migration, save roundtrips,
independent body dimensions/presentations, all outfits and visible
headwear/accessory geometry. **The full Android Gradle test suite, APK build,
physical Galaxy S9/Daydream rendering, frame timing and optical checks
are still pending** and must not be claimed from GitHub source writes.
This is a morphable, first-party low-poly portrait system, **not yet a
rigged full 3D character roster with skinned clothing**. Next phases must
choose inspected owned model sources, implement actual shared skeletons,
fit/retarget costumes across extreme proportions, add clothing slots,
sustained device-performance gates, and validate full-body appearance
in the headset. Do not treat named data entries as imported 3D art.
