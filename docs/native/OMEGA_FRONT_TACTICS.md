# Omega Front — Initial Headless Tactical Combat Kernel

**Implementation state:** source-implemented and host C++ tests passed locally; **no hosted VR module, no GL renderer, no packaged APK, no device acceptance**.
**Program:** RV-GAME-06; first bounded implementation toward P1 (Hearthwell waterworks defense).
**Authority:** `app/src/main/jni/native/omega_front_tactics.h` is the game-specific C++17 rules source. This is NOT a shared ReverieVR SDK helper or an ABI change.

## Implemented now

- Fixed 24×24 logical tactical grid, with road, rubble, wall, deep water, and ordinary ground; walls block fire and deep water blocks foot movement.
- Eight combatants in the default Hearthwell seed fixture (four defenders, four attackers); per-side unit identity, HP, ammunition, armor and three distinct roles (rifle, engineer, heavy).
- Input-facing bounded per-unit FIFO order queues: move, attack, repair the pump and resupply at the ammunition crate; order cancel; invalid/hostile orders rejected.
- Deterministic cardinal BFS occupancy-aware movement, seeded attack variance, target-range checks, shared Bresenham line of sight, rubble damage reduction, finite ammo, finite repair materials and depot rounds.
- Enemies approach their known pump objective and attack visible defenders first; they cannot seek hidden defender coordinates.
- A win requires all hostile troops disabled while defenders and the pump survive; losses include pump destruction, all defenders lost, or the bounded 160-pulse encounter timeout.
- Simulation advances **only through `Battle::Step()`** (one fixed 250-ms logical pulse); reading ground, unit status or deterministic checksum does not mutate state. Ended battles are frozen.
- Seed/state/terrain/units/queues/stock/result fields contribute to a stable explicit-field FNV-1a checksum for replay comparisons.

## Reproduce host checks

From the repository root:

```sh
bash scripts/verify-omega-front-tactics.sh
```

The runner builds `app/src/test/native/omega_front_tactics_test.cpp` under either GCC or Clang (set `CXX=clang++`) using C++17 strict warnings and runs it. It does **not** access GitHub Actions. Additional optional sanitizers were run locally during implementation:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Iapp/src/main/jni/native app/src/test/native/omega_front_tactics_test.cpp \
  -o /tmp/omega_front_tactics_test && /tmp/omega_front_tactics_test
```

**Observed local checks on 2026-10-15:** g++ and clang++ host runners both passed; g++ ASan+UBSan passed. Remote source and test file blob SHAs were compared against exactly these locally compiled source files.

## What this deliberately does NOT prove

This header isn't listed as a native shared library in Android.mk, and no `reverie_native_module_entry_v1` exists for Omega Front. The game will **not appear in the launcher** and it is **not playable in VR** yet. No 70 campaign missions, progression, seventeen factions' strategic AI, world morphology, sector generator, saves, chibi atlases, civilian population, proper fog of war or accurate dynamic faction logistics are implemented by this kernel. This is only one real combat fixture and its underlying minimal combat rules.

The queued attack-order visibility rule uses current unit coordinates at order time, with a nine-tile local visibility radius and occluding walls; full faction-wide fog memory, sensor intel and ranged weapons are separate follow-up tasks.

The initial fixed sides are `Hearthwell` and `Raiders`, and the default scenario is a tactical **test fixture**, not an assertion that the player has already fought each of the canon 17 factions.

## Next technical gates

1. Bring the **actual kernel into a hosted module** via an allowlisted `omega-front` ABI entrypoint, complete Android.mk library definition and real per-eye GLES2 sprite rendering. Ensure shell-global recovery/Home is never swallowed; support gaze and normalized select/secondary controls without requiring touch-and-click to move.
2. Adapt command UI and tile cursor to seated 3DoF head tracking, with readable closest-range board placement and reliable pause/route inspection.
3. Add robust persistent save schema, faction fog/visibility snapshots, civilian waterworks integrity and routed supply, then dynamic morphology and authored opening first campaign battle.
4. Run actual APK/NDK integration and signed build verification separately. Test on Galaxy S9/Daydream in short and sustained sessions before claiming device admission.

**Acceptance boundary:** source-level deterministic tactical kernel implemented, independently compiled and locally tested; P1 overall and mobile integration still incomplete.
