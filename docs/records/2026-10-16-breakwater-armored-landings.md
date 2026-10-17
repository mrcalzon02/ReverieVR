# Breakwater — armored landing craft and deployed shore vehicles

**Date:** 2026-10-16  
**Authoritative branch:** `mrcalzon02/ReverieVR` `main` only  
**Starting head:** `abb7ff52f2aafd32e529805991d475d4af6ba795`  
**Status:** Source implementation and isolated armor-helper checks; integrated native/Android/physical game acceptance remains open.

## INTENT and OBSERVED

The user requested continuing Breakwater and specifically asked whether the landing craft already carried armored vehicles. Reconciled the real campaign and renderer: before this pass `UnitType` had Rowboat, LandingCraft, Gunboat, Aircraft; a landing craft unloaded only 3–6 infantry. There were no independent armor actors, vehicle HP, armored salvos, tank hit checks, vehicle models, or vehicle-destruction sound. The active boat drawing path uses the sprite atlas, not the legacy `EmitVesselSilhouette` geometry recipe; blindly drawing cargo in that older renderer would have added invisible code.

## EXECUTE

**Campaign and pool:** `Enemy::carries_armor` is a deterministic seeded manifest assigned only to eligible later-progression landing craft; the ordinary first-day rowboat is never armored. An armored transport carries up to three infantry *and* one light armored car. If sunk before landfall, its would-be cargo disappears with it. An actual landfall creates one independently addressable `ArmoredVehicle` (95 HP), and a fixed ten-vehicle combat array bounded by `NextCount`'s ten-craft maximum. The vehicle advances from the landing zone to z=-1.56, then fires four battery integrity damage every 3.3 seconds. Its `ArmorShotEvent` is generated only for actual hull damage. Killing it pays 110 credits, increments the armor-destroyed counter, stops further attacks, and unblocks wave completion. Waves, storage format, location/difficulty progression, head/controller controls, and original 80-byte shop serialization are unchanged. The units reset cleanly between waves.

**Penetration and target priority:** `breakwater_armor.h` supplies the deterministic cargo choice, per-weapon armor resistance, travel/fire rules and a projectile-to-sphere hitbox at the actual vehicle position. Normal weapon ballistics originate at the true cannon muzzle and now compete with boat, soldier, aircraft, cover and independent armored-car targets by impact depth. Target ID ranges remain stable for previous enemy, infantry and cover targets. The compact game HUD identifies ARMORED CAR with actual live HP; world brackets surround that exact selected actor. A machine-gun round does quarter damage, heavy MG half, flak one-third, cannon/missile full normal damage. Purchased artillery or airstrikes can destroy already landed armor and generate correct impacts, never damage undelivered cargo as a separate target.

**Visuals and audio:** Original faceted olive chassis with wheels/tracks, angled armored front, turret and projecting cannon built with the existing GLES2 XYZRGB primitives, shared across stereo eyes and linked to the vehicle's real x/z. The active *atlas ship* path now also overlays visible armored cargo above selected landing craft, not merely in its unused legacy boat-render helper. Real vehicle gun events produce 0.14-second gun-mouth flashes and battery impact pulses. Two additional bank-generated one-shot cues are numbered 20 (armored cannon) and 21 (armor destroyed), preserving existing cue IDs 1–19 and the owner-authorized `Mars des Souvenirs` passive music loop. No sampled sound, third-party image, new texture, music override, GitHub Action, or new save ABI.

## OBSERVE and VERIFY

- Exact `breakwater_armor.h` Git blob `913591a1dcec47d16382931b67edb0c685dd3d66`; its exact `breakwater_armor_test.cpp` blob `c2f81be9b1aa16b7fb06498a38e6ad515e4f4564`. Reconstructed both, read back their GitHub blob hashes and ran strict GCC and Clang C++17 `-O1 -Wall -Wextra -Wpedantic -Werror`; passed both. Clang AddressSanitizer and UndefinedBehaviorSanitizer passed as well. Host harness used a *small stand-in campaign declaration and earlier faithfully mirrored ballistic weapon formulas*, not complete authoritative `Campaign` or Android module dependencies. That scope is explicitly limited.
- Updated `breakwater_campaign_test.cpp` with source-level full-simulation regressions: locate a seeded armored first landing craft on Iron Quay (seed scan yields a legitimate candidate), run identical copies, destroy one transport while its counterpart lands, verify the lost vehicle never exists in the intercepted branch, then shoot the independently landed vehicle down with the real Heavy Gun's armor damage and counter effects. Also added `breakwater_armor_test.cpp` to `scripts/verify-breakwater-visuals.sh`. **The full native campaign test and native module were not compiled or executed** in this connector-limited workspace.
- Existing Android sound-bank regression source now validates distinct bounded armored cannon/destruction PCM and ID routing. **JUnit, Gradle APK, Galaxy S9 headset texture depth, native input, surf/music mix and audio timing have not been run.** Do not claim launch/device acceptance from GitHub readback alone.
- Per-file direct GitHub `main` updates returned commits, followed by source readback and checks; source modifications were targeted without stripping unrelated parallel changes.

## Follow-up root-cause repair — cannon overhangs the beach

The newly deployed cars exposed a previous false-positive acceptance
assumption in the preceding muzzle-origin implementation. The physically
modeled gun had its breech at z=-0.88 and projects the mouth another
1.25–1.63 units seawards, landing at z≈-2.3 or deeper. Disembarked
infantry move between z=-2.28 and their fire line z=-1.34, and armored
cars from z=-2.23 to z=-1.56, all behind that forward-facing muzzle.
The controller-origin ray used before the physical muzzle fix had
hidden this problem by generating imaginary near-shore hits.

`breakwater_gun_mesh.h` now defines one shared
`kMount={0.f,-.76f,.65f}` for live model mesh, recoil/muzzle flash and
`Muzzle` lookup, with a matching low, rear carriage base in
`breakwater_module.cpp`. Every muzzle now sits landward of the
beach's combatant hit spheres; no hand-ray origin fallback was added.
Extended `breakwater_cannon_trajectory_test.cpp` explicitly shoots
the real soldier fire-line sphere and armored-car sphere from all five
weapon muzzle origins. Exact test file SHA after repair
`109b0d90fc96f6ed10a0c3f9c845293ccffb105b`; strict GCC and Clang
C++17 tests passed as did Clang ASan/UBSan using the same previously
mirrored (not full original) supporting weapon mesh/ballistic code.
The separate full solid-gun mesh geometry regression source was also
updated for the new location but NOT run against exact live mesh
implementation during this session. S9 headset depth and comfort still
require full-device validation.

## Next device acceptance

Run the complete native game C++ test and Visuals runner on a locally available full clone; perform Android NDK compile, debug APK install and breakwater entry in S9. Confirm eligible armor is visibly carried inside the approaching craft; sinking it prevents unloading; live landfall creates car model at the correct beach line; Cannon/Heavy/Machine shots hit actual armor with expected penetration; physical shore cover blocks armor hitbox correctly; SAM remains aircraft-only; valid armor cannon events alone play armored cannon audio. Check no duplicate armor on slot reuse, no ghosts between waves or Quick Menu focus, intact `Mars des Souvenirs` loop, and sustained frame pacing with up to ten vehicles. Only then mark device acceptance.
