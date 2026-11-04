# Omega Front — accepted square faction portrait art

The original gold/steel-blue Omega symbols remain the **primary UI style reference** at `art/omega-front/style-guide/omega_primary_symbols.png`. They are not another faction.

**Five accepted faction sets (100 PNGs total)** are committed under `app/src/main/assets/native/omega-front/ui/icons/f06/` through `f10/`. Each has 20 independently sliced square 256×256 portraits. The five 1280×1024 atlases and supporting manifests are under `art/omega-front/accepted/` (the F10 manifest is also in its runtime folder).

- F06 The Lost Ninth — Roman-era cohorts and siege engineers.
- F07 Ashen Crown — feudal levies, knights and castles.
- F08 Copper Steppe Confederation — Bronze Age nomads, chariots and caravans.
- F09 Brassline Republic — early-industrial trenches and rail artillery.
- F10 The Farwake — distant-future precision infantry, drones and high-tech specialists.

The names remain **proposed** per `docs/project/OMEGA_FRONT_FACTIONS.md`, and support roles beyond each dossier are concept extensions. No new faction is invented by the artwork.

## Actual use and remaining verification

`app/src/main/jni/native/omega_front_portrait_catalog.h` maps all 100 catalog entries into the **native Faction Gallery**, available from the title and pause menus. `omega_front_texture_renderer.h` reads and decodes the selected portrait on demand, then maintains one GPU texture. `omega_front_module.cpp` renders a square portrait with a real text label.

The existing **64px tactical battlefield sprite renderer is separate**. F07–F10 are not silently made playable combat factions by displaying their artwork, and the runtime gallery is not the same as a finished tactical unit-inspection UI.

The C++ gallery menu path and the 100-portrait PNG decoder are covered by native regression tests. The Android GLES capture workflow records sample F06/F10 gallery frames as **diagnostics**, not release-blocking art assertions. Do not claim that actual headset display or every proposed unit has been separately play-tested.

Do not import the rejected Iron Syndicate, Skyward Dominion, Northern Alliance, mislabeled faction posters, or rectangular promotional cards.
