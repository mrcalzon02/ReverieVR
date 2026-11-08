# Procedural Test Chamber — working integrated showcase (2026-10-30)

## Project recovery and readable controls — 2026-11-08 source

The native workshop can now recover from a bad default project. It checks
the other slots for an existing valid project before choosing an unused
fallback slot, and reports which unreadable projects were retained.
Next Project skips unreadable alternatives while keeping their original
bytes untouched. If every slot is unreadable, the module still launches
with a read-only preview and an actionable Retry Projects screen. A retry
can reopen a repaired slot without destroying and recreating the module.

Dirty edits remain in memory on write failure, including an error returned
after replacement bytes became visible. Project switching and Play Test
wait for a successful save. Quiescent edits retry after two seconds;
pause and the optional `prepare_exit` callback flush synchronously, and
ordinary host exit remains blocked while edits or active recovery remain
unresolved. Pause/resume requires both action buttons to be released.

The showcase header/footer and every workshop tool use bounded centred
Lato text. The tool column's rendering and ray hit regions share the same
visible horizontal and row constants, with separate help/status rows.
Real Mesa GLES captures include both eyes for showcase, edit and recovery
screens. Fault tests exercise actual module Create, Update, pause/resume
and exit callbacks, in addition to the existing scene/motion/light/sound
core tests. This does not replace physical headset readability, storage,
audio and performance acceptance. See
`docs/project/NATIVE_RECENT_GAMES_REPAIR_2026-11-08.md`.

The installed **ReverieVR native Test Chamber** has now been expanded from the original two procedural objects into a practical, low-allocation visual/audio regression environment. The source is the bundled `app/src/main/assets/native/test-chamber/scene.rvscene`, parsed with the strict existing REVSCENE v1 editor contract. The full layout has **30 authored primitive parts**. They deliberately form recognizable composite objects (an arch/door frame, chair, table, crate stack, display plinth, beacon, light samples, orbiting specimen and rotor) in addition to primitive stone/metal/glass/gold material examples, with the avatar/wardrobe mesh still present. No random or unrelated internet artwork substitutes for the test assets.

## Real native rendering

`app/src/main/jni/native/test_chamber_module.cpp` retains its OpenKTG procedural texture and one context-lifetime static room/avatar/authoring mesh VBO, with explicit object draw ranges. A GLES2 shader applies per-vertex moving-point-light falloff, adjustable ambient lighting and emissive material intensity, plus bounded uniform-only animation for the rotor and floating specimen. There are no CPU mesh rebuilds per frame or per eye. All user instructions are antialiased **licensed Lato Semibold atlas glyphs** shared with other native games, decoded once per GL context. The engine preserves the existing two-eye, GL-state restoration, avatar wardrobe and shell locomotion contracts. These lighting passes are real programmed falloff/emission examples, **not** yet a physically based renderer with dynamic shadows, spotlight cones or imported skeletal animation.

## Sound and interaction

A controller primary press cycles six observable lighting/material/audio modes, with the active mode shown as real text inside the chamber. Each step sends one of six **actually registered host sound cues**, using the existing shell sound path; an activation feedback cue is used when that API is not supported. These are existing Red Ledger click, metal, coins, book, door and mechanism/drawer sample effects, **not** the companion application's separate WAV bank. The independent Test Chamber companion continues to package its own five GLB object models and six WAV cues; those assets are not falsely described as imported into the integrated native renderer until a proper supported asset importer/copy path is validated. The app preserves all player-exit and global shell recovery actions.

## Acceptance checklist

- Strict authoring parser loads the full 30-piece REVSCENE and remains within its 512-piece and 150-kB caps.
- Android NDK links `-lz` for the genuine Lato atlas, with no glyph strokes or bit-pattern text.
- Android emulator must launch the native Test Chamber and capture its authored scene without GLES2 errors; both eyes and context creation must still work.
- Inspect the captured 3D scene for the composite objects, legible instruction labels and distinguishable materials.
- Controller primary selection must cycle lighting states and audible cues. This is **not** proven by a static screenshot or a standard build.
- Validate real S9 headset comfort and sustained frame timing separately. Do not claim it from CI success.

## Sitka distinction

Sitka is visible in the same gallery as an **inspectable development project**, not a broken faux native game. The existing Sitka terrain proof has no licensed actual Sitka coastline assets or implemented fishing loop yet. Its procedural identity illustration is clearly marked as project artwork rather than a captured playable game.

## Emulator-informed exhibition layout refinement

The first real 512×512 Android-eye capture proved the Lato glyphs, shader creation and all authored meshes rendered, but foreground props blocked the view of the showcase. The subsequent layout revision moved the thirty exhibits farther from the default camera, expanded the room in horizontal/depth dimensions, shifted the avatar and revised the procedural noise palette from cyan-dominated to neutral for accurate visual distinction of wood, metal, gold and glass tints. The rotor pivot and light falloff center were updated with the scene positions. The native parser test now checks that all specimens remain inside the revised room and clear of the initial near view. A fresh installed emulator capture, not only the signed APK, is still the final visual evidence gate for these revised placements.
