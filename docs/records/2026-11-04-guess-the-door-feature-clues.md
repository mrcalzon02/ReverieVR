# 2026-11-04 — Feature-derived Guess the Door clues

**Source stage:** Deterministic rules and vector geometry; Android runtime
and original Mechanist art still await separate acceptance.

A clue must describe a *physically drawn* detail of the safe door,
never a random symbol or an answer inferred from hidden seed flags.
The supported attributes are oak-plank / iron-sheet material,
panels / cross-bracing, no / four rivets, ring / long-bar handles,
and clean / three scratches. The puzzle derives a clue from the safe
door's selected feature value and forces the opposite feature
value on the other door. Other features may match or differ at random.
The same DoorDesign objects drive clue, gaze inspection and scene mesh.

The 2.3-second hint is followed by visible door construction and
optional non-answer-giving gaze inspection. Twelve-room route,
same-seed pit restart, slow lever/door/walking, and Easter
egg trigger remain unchanged.

**Regression:** 80 seeds x 128 room indices = 10,240 room combinations
check deterministic geometry descriptors, exactly one clue match,
every feature/value pair, transition, pit reset and victory, with
the dedicated native smoke under GCC and Clang at -Werror.

**Unaccepted:** Real Mechanist closed/open sprite pair selection,
sprite-accurate attribute annotation, APK/NDK full build,
headset readability and Galaxy S9 performance. On sprite import
never claim features that are not visible in the actual pixels.
