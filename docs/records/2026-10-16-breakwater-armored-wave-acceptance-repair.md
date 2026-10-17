# Breakwater — long-wave checkpoint and armored defense integration repair

**Date:** 2026-10-16
**Repository:** `mrcalzon02/ReverieVR` on `main` only
**Baseline:** `431945a1d7575e5bc7ecaefd1d89d1575cd7f7c6`
**Acceptance:** Source committed and pure C++ checks passed. Full native Campaign/NDK/APK and Galaxy S9 headset unverified.

## Intent / observed defect

Continue substantive Breakwater development after adding armor-bearing landing craft. A source inspection uncovered two integration faults:

1. The extended 244/280/316/352-wave campaign's `waves_this_period()` returned 12–25 individual waves during actual shipped periods, but `Campaign::DeserializeShop` still rejected every checkpoint where the saved index exceeded **8**. A player could fight later waves but not reliably resume them from the existing 80-byte shop save.
2. The purchased autonomous turret only targeted living surface vessels. Once an armored car survived landing, the turret ignored it even when it was the nearest attacker. The combat HUD showed next aircraft, offshore gunboat and infantry volleys, but no warning of an imminent armored shell.

## Execute

- Repaired serialized index validation to permit 1–32 before narrowing; candidate location/day/night/difficulty now runs `waves::CheckpointAllowed` on the original, full 32-bit index, accepting only indices 1..the true `ForPeriod` bound and rejecting overflow/wraparound/cross-period corruption without partially overwriting live state. No save version, 80-byte length, or user-data migration was introduced.
- `Campaign::TickStep` support-turret selection now compares the nearest live non-aircraft surface vessel and the nearest live deployed armored car. It fires at whichever is actually closer. On an armor hit it applies the same `armor::ArmorDamage(WeaponKind::HeavyGun,15)` penetration rule (7 damage), and grants armor kill credit/status only on real destruction. Turret shot events and reusable impact rendering retain their existing sequence semantics. Aircraft remain excluded.
- `armor::SecondsToFire` predicts a real car's next damaging shell as max(time remaining to z=-1.56 at 0.29 units/sec, remaining attack cooldown), and returns -1 for nonexistent, destroyed, nonfinite actors. `Campaign::incoming_threats` returns the earliest live armored ETA.
- The fixed-size combat HUD reuses its one SHORE threat card. If the armor shell would be first it displays `ARMOR nS`; otherwise the original `SHORE nS` soldier countdown wins. AIR RAID and GUNBOAT cards remain independent. No new floating explanatory panel or redundant page.
- Updated native regression sources and canonical project documentation.

## Verify / boundaries

The source-exact `breakwater_wave_schedule.h` and `breakwater_wave_schedule_test.cpp` correspond to Git blobs `9728e0fe0439a6902113fb41a0600a90a3ff6f61` and `6e038f59a1e0f615118b9ef045da4f27204eec20`. Reconstructed files in the local test sandbox matched both hashes; strict GCC and Clang C++17 `-O1 -Wall -Wextra -Wpedantic -Werror` and Clang AddressSanitizer/UndefinedBehaviorSanitizer passes each succeeded. The test enumerates all valid checkpoint indices of all eighteen periods/four difficulties, rejects zero, beyond-period values and 257/65,537/0xffffffff full-word aliasing, and statically checks final Siege 25 daytime and 21 nighttime limits.

The exact `breakwater_armor.h` / `breakwater_armor_test.cpp` Git blobs matched `0f2e7628164f980b73cd90b742c4f86b3230e2d6` / `183bcef930df5e22280dba3395aa3303772257bd`. GCC, Clang and Clang ASan/UBSan all passed the landed-car movement/cooldown, true next-shell estimate, invalid/absent actor behavior, armor hit and damage regressions, using the existing small *mock Campaign declaration* and source-mirrored weapon/ballistic dependency. This is not equivalent to compiling the whole native game.

The full `breakwater_campaign_test.cpp` fixture was expanded for 25/21 late checkpoints and a real landed-armored-car HUD warning. **That full test, full native module/link, Java/Android test suite, APK, and Galaxy S9 controller/stereo/audio/thermal acceptance have NOT been executed**. Repository access worked through the GitHub connector, but direct shell GitHub DNS remained unavailable; do not infer APK readiness from remote source readback. No new GitHub Action, force-push, destructive rewrite, or source/license substitution occurred.

## Follow-up acceptance

From a complete local checkout, run the project C++ campaign and visuals tests, native doc/source guard, NDK/Gradle assemble, and Galaxy S9 smoke test. Verify save and reopen at wave 9+, final Siege wave 25, full-period advancement, purchased turret shooting deployed armor if no closer ship, real ARMOR countdown taking precedence only when imminent, first-day unarmored rowboats, intact support/SAM behavior and owner-authorized `Mars des Souvenirs` loop. Keep event-level game audio and headset scene efficiency intact.
