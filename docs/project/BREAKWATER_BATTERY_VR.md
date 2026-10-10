# Breakwater Battery VR — canonical native game specification

**Status:** Development-only VR prototype, physical Galaxy S9 acceptance pending. **Backlog:** RV-0640. The authoritative implementation is in app/src/main/jni/native/breakwater_campaign.{h,cpp} and breakwater_module.cpp. This game uses original assets/code and does not import Beach Head content.

## Pitch and campaign structure

A stationary coastal-defense gun battery faces a gradually escalating amphibious invasion. The player is seated, scans the horizon by turning their head, and aims the gun using the shell-calibrated handheld pointer. No artificial walking or jolting camera recoil.

The campaign has three defended locations: **Shingle Bay (two days), Iron Quay (three days), and Blackcap Island (four days)**. Every day and night is a separate wave period with a shop after **every** wave. Initial Casual/Regular/Veteran/Siege wave schedules per day are 2/3/4/5 and per night 1/2/3/4, increasing on later maps and days with a ceiling of eight per period.

Briefing → inter-wave Shop → Combat → Shop → next wave → sunset/night → sunrise/day → next location → Victory. Integrity falling to zero means Defeat. Nothing automatically skips the shop.

## Enemy transport rules

**The opening enemy is a rowboat, not a robot.** A rowboat carries exactly **one infantryman**. Stronger military landing craft appear later, each carrying **three to six infantry**. Ships have real position, speed and hull HP. **Sinking the craft before it beaches cancels every passenger**, while a surviving craft unloads its passengers as separate hostile shore infantry. Infantry that remain alive periodically inflict battery damage. Later progression introduces gunboats and then overhead strafing/bombing aircraft. Aircraft require the anti-air upgrade for ordinary gun targeting or a purchased airstrike; the basic gun cannot engage them. Distinct bomber/strafer patterns and enhanced AA models remain planned.

The initial module generates simple cuboid rowboats, military boats, aircraft and shore-infantry proxies. Their movement and destruction reflect the deterministic kernel; they are not fake decoration.

## Campaign credits and purchases

Players earn credits for destroyed hulls, eliminated shore infantry and surviving a wave. Purchasing is allowed only between waves; upgrades have real effects in the simulation.

| Choice | Base cost | Existing behavior |
|---|---:|---|
| Fire rate | 180 | 35 ms less shot delay per tier; maximum 4 tiers |
| Damage | 200 | +10 impact damage per tier; maximum 4 tiers |
| Clip size | 160 | +4 rounds per tier; maximum 4 tiers |
| Anti-air | 750 | Unlocks ordinary gun engagement of aircraft |
| Artillery strike | 260 | One consumable bombardment of surface vessels |
| Airstrike | 420 | One consumable strike against surface or air |
| Support turret | 470 | Automatically damages nearest surface craft at a fixed cadence |
| Repair | 140 | Restores 25 battery integrity, capped at 100 |

Advanced weapon mounts, separate caliber variants and impact/splash effects remain later features; the current lightweight gun tracer is generated procedurally and purchases affect the actual C++ gameplay calculations, not merely their labels.

## First playable controls and 3DoF contract

The Development-only module is launched from Native Apps after setting launcher logging to Development. The shell retains Back, Home, Quick Menu and recenter control. The gun is aimed with the normalized native pointer direction.

In the shop, point at any of the eight labelled upgrade cards and press **Select** to buy it. Point at **START** and press Select to begin the next wave; an available secondary action can also begin it. In combat, point at a hull or disembarked infantry and hold Select to engage; fire cadence depends on purchased rate. Every valid in-traverse gun shot now consumes a round whether it hits or misses. A shot aimed at aircraft before anti-air is purchased still spends ammunition and produces feedback but cannot damage the aircraft. Point at **ARTY** or **AIR** and press Select to spend a stockpiled strike. The turret fires automatically after being purchased. Invalid actions produce the existing shell-owned failure cue. Victory/Defeat provides an implemented restart. Phone test must prove readable target silhouettes and reliable pointer hitboxes; this is not physical validation.

The gun carriage is world-anchored and the rendered barrel follows the handheld pointer only inside a finite traverse/elevation envelope; firing outside that mount arc is rejected. Successful gun shots create a short-lived generated tracer ribbon, including misses, while the 1.15-second automatic magazine reload makes clip-size upgrades meaningful. Audible weapon report/reload, **visual** magazine reload, splash/impact feedback and more believable aircraft flight paths remain explicit next slices. An 80-byte versioned shop-state save preserves completed waves and bought upgrades through the module-private host save service. Mid-wave exit resumes at the prior shop; S9 exit/re-entry and APK update durability remain device gates.

## Runtime art and visuals

Follow the tiny pre-PlayStation, early-PC 3D aesthetic: simple vertex-color meshes, procedurally striped sea, day/night palette, chunky silhouettes and a 3×5 generated pixel-card text renderer. Future recipe-driven additions: landing ramps, wakes, smoke/explosion cards, weather, sky/cloud noise, per-location terrain, procedural RGB565 material atlas, distant fog and sprite-based effects. No large texture packs, modern PBR, heavy shaders, imported licensed game materials or duplicated per-eye scene simulation.

## Original MIDI-era music for every native game

Soundtracks should evoke **early Sierra MIDI composition** through new melodies and lightweight note-event sequences, not copied tunes or proprietary instrument banks. Breakwater cues: harbor brass title, shop intermission, daytime battle, night assault, aircraft-warning sting, victory/defeat. Red Ledger: weary smoky noir. Lantern Desk: chamber-intelligence tension. Forward Detachment: measured snare and low winds.

**Actual native MIDI playback does not yet exist.** The current module has host-owned feedback cues only. The shared note-event sequencer, software synthesis and audio-focus service must be implemented and measured on the S9 before calling music functional. Reference: docs/native/MIDI_MUSIC_STANDARD.md.

## Acceptance gates

1. Native C++ tests for boat HP, 1-passenger rowboat, 3–6 military landing party, prevented landing, infantry danger, upgrade behavior, turret and anti-air restrictions.
2. Android ARMv7 + ARM64 NDK build and signed installable phone-test APK, verified by remote read-back.
3. Independent Development-only launcher list and runtime gating.
4. Galaxy S9 + Daydream: stereo, seated comfort, shop Select, held fire, real boat sinking, shore-landings, day/night progression and emergency shell recovery.
5. Campaign save/update durability, actual projectile effects, audio/MIDI ownership, 15-minute thermal/frame pacing and battery test.

Do not promote the prototype to the normal launcher until the hardware gates have been met.