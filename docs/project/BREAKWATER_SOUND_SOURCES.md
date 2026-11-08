# Breakwater Battery VR — Sound Source Register

**Role:** PROJECT MANAGEMENT / SOURCE README  
**Parent:** [Breakwater implementation plan](BREAKWATER_BATTERY_VR.md) · [RV-0640 backlog](BACKLOG.md) · [Licensing policy](PUBLIC_DISTRIBUTION_AND_LICENSES.md)  
**Checked:** 2026-11-08 (actual shared-library bytes and the NOX pack's original license README)

**Status:** **46 normalized runtime WAVs imported from the approved shared library and documented first-party derivatives.** Current hashes, measured levels and recipes are in [the bank manifest](../../art/breakwater/audio_manifest.json). The earlier web candidates below remain historical candidates, not imported sources. Device listening and source-aligned APK evidence are tracked separately in the [execution record](../records/2026-11-08-breakwater-sound-overhaul.md). Breakwater's original-owner song `Mars des Souvenirs` remains separate.
**Art direction:** Generic late-1990s/early-2000s Western military arcade sounds; punchy, legible, low-latency, inexpensive on Galaxy S9. Prefer authentic mechanical impact and modest environmental reverb over cinematic sci-fi or excessive bass.

## Imported shared-library bank — 2026-11-08

The current bank is **2,608,902 bytes** across 46 canonical PCM16 mono WAVs
at 22,050 Hz. Seven environmental loops are six seconds each, with a
250 ms circular crossfade; the largest decoded sample is 264,600 bytes.
Only selected runtime derivatives are packaged. Original purchased/exported
masters remain in the owner's shared collection.

Sources are NOX SOUND Essentials nature, vehicle and water Foley recordings,
owner-original Between Deliveries grenade/metal exports, and the existing
first-party `NativeCombatWaveform` recipes. The NOX README explicitly states
CC0; its original bytes are preserved as
[license evidence](../licensing/sources/nox-essentials-readme.pdf). Owner
exports follow [the existing grant](../licensing/OWNER_ORIGINAL_BETWEEN_DELIVERIES.md).
The [public source catalog](../../tools/breakwater/audio/source_catalog.json)
pins every admitted original's filename, byte size, SHA-256 and rights.
Private storage IDs are excluded from the public records.

| Family | Runtime coverage | Peak ceiling | Active RMS target |
|---|---|---:|---:|
| Controls/progression | Focus, activation, rejection, equipment, reload start/ready, menu open/back, difficulty/restart, support arm/disarm, station move, empty gun, aim limit, cooldown, unavailable support, purchase/wave/results | -12 dBFS | -22 dBFS |
| Combat | Five distinct weapons, hull/water/sand impacts, boat/armor destruction, incoming fire, SAM, artillery and airstrike | -9 dBFS | -18 dBFS |
| Environment | Surf, wind, night, boat machinery, rowing texture, approaching aircraft, gun traverse | -18 dBFS | -24 dBFS |

These are per-category measured PCM targets, not equal loudness for every
kind of action. The offline tool trims source silence, removes source DC,
applies deterministic filters/edits and smooth-knee peak compression, then
remeasures the actual quantized output. The manifest identifies every
layer and transformation. Boat/aircraft engines are deliberate treatments
of recorded vehicle machinery; rowing is constructed from recorded water
movement. No authentic boat, oar, aircraft or firearm field recording is
claimed where none was found. Synthesized weapon components remain paired
with recorded accents; the sand cue is explicitly first-party procedural.

The shell reserves two control, four combat and seven loop voices. Fixed
gains bound the bank's simultaneous sample peak below 0.50, leaving room
for the existing music player's maximum 0.50 gain. Loops follow the actual
scene and nearest active sources; 900 ms leases, explicit pause/exit stops,
audio-focus handling and stale-callback invalidation prevent orphan audio.
The game still runs inside the one ReverieVR APK.

Reproduction instructions and tool: [tools/breakwater/audio](../../tools/breakwater/audio/README.md).
Validation entry point: `python3 scripts/verify-breakwater-audio-assets.py`
(optionally `--apk <signed.apk>` to inspect the actual packaged bytes).

## Historical linked source candidates

Each link goes to an original asset/download page, **not an unverified mirror**. The license shown is the publisher's declaration on that page, not proof that every asset in a downloaded ZIP has been individually audited. CC0 license terms: https://creativecommons.org/publicdomain/zero/1.0/. **Candidate** means approved for audition and per-file due diligence, not approved for ingestion without the intake record below.

| ID | Source (creator and original page) | Page-stated license | Highest-value Breakwater uses | Intake decision |
|---|---|---|---|---|
| BW-SFX-001 | [Kenney — Impact Sounds](https://kenney.nl/assets/impact-sounds) (Kenney; 130 files) | CC0 1.0 | Metal/hull collisions, sand/water impact accent layers, battery hits | **P0 candidate**; audition individual samples |
| BW-SFX-002 | [Kenney — RPG Audio](https://kenney.nl/assets/rpg-audio) (Kenney; 50 files) | CC0 1.0 | Reload hardware, switches, footsteps, equipment handling | **P1 candidate**; audition for appropriate modern equipment character |
| BW-SFX-003 | [Kenney — Interface Sounds](https://kenney.nl/assets/interface-sounds) (Kenney; 100 files) | CC0 1.0 | Shop purchase/decline, reload ready, victory and wave transitions | **P1 candidate**; keep the existing UI feedback fallback |
| BW-SFX-004 | [The Free Firearm Sound Library](https://opengameart.org/content/the-free-firearm-sound-library) (Ben Jaszczak, Brian Nelson, Kevin Heras, Matthew Nanney; OGA archive) | CC0 1.0; project creators explicitly dedicate the original recordings | Four/five distinguishable firearm reports, with a dedicated cannon/missile treatment built from separate layers | **P0 candidate**; do not commit the 194-MB archive; select short excerpts and verify original metadata |
| BW-SFX-005 | [25 CC0 Bang / Firework SFX](https://opengameart.org/content/25-cc0-bang-firework-sfx) (rubberduck) | CC0 1.0 | Heavy cannon crack, distant barrage, rocket launch/explosion layers | **P0 candidate**; fireworks may require heavy editing for believable gun reports |
| BW-SFX-006 | [Sci-Fi RTS War Unit Sounds](https://opengameart.org/content/sci-fi-rts-war-unit-sounds) (qubodup; pack revised to CC0 in 2024) | CC0 1.0 on current asset page | Military-radio, artillery control, naval unit reactions, limited mechanical cues | **P1 candidate**; choose non-derivative-feeling sounds and confirm downloaded pack is current CC0 release |
| BW-SFX-007 | [Synthesized Explosion](https://opengameart.org/content/synthesized-explosion) (qubodup) | CC0 1.0 | Boat destruction, artillery explosion, distant secondary detonation | **P0 candidate**; synthesized provenance is useful for legal simplicity |
| BW-SFX-008 | [Beach Ocean Waves](https://opengameart.org/content/beach-ocean-waves) (jasinski; submitted by qubodup) | CC0 1.0 | Shingle Bay surf and shoreline wash, gentle loop beds | **P2 candidate**; select a loop segment, no long source recording in APK |
| BW-SFX-009 | [Water Waves](https://opengameart.org/content/water-waves) (transitking; submitted by qubodup) | CC0 1.0 | Boat wake, near-shore splashes, projectile water impacts | **P0 candidate**; source page links four small FLAC wave excerpts |
| BW-SFX-010 | [Engine Sounds (2)](https://opengameart.org/content/engine-sounds2) (pauliuw) | CC0 1.0 | Boat-motor low loop or equipment motor, if it passes listening review | **P1 candidate**; advertised as modern/sci-fi—reject if too futuristic |
| BW-SFX-011 | [Random Gunfire SFX](https://opengameart.org/content/random-gunfire-sfx) (iamoneabe) | CC0 1.0 | Generic light gunfire alternate layered report | **P0 backup candidate**; require per-file audition |
| BW-SFX-012 | [Tiny Naval Battle Sounds Set](https://opengameart.org/content/tiny-naval-battle-sounds-set) (qubodup) | CC0 1.0 stated on page | Potential navy splashes, misses, cannon impact | **HOLD — provenance review.** Page discussion describes third-party claims about explosion audio derived from military videos and earlier replacement of disputed sounds. Do not import any part by blanket pack approval; investigate the **specific** candidate files independently first. |

### Explicitly excluded from the default public source library

- [Sonniss GDC Game Audio bundles](https://sonniss.com/gameaudiogdc/) — royalty-free **within synchronized finished productions**, but **not CC0**; [version 2.0 of the license](https://sonniss.com/gdc-bundle-license/) effective 2026-08-27 prohibits distributing the recordings as a standalone sound-effect collection, sample pack, or reusable project asset. **Do not commit its raw or transformed clips** to public `main`. May only be considered in a separate distribution approach after confirming the exact license and intended distribution.
- Any individual file without a verified source URL or license, including CC-BY-only, NC, SA, proprietary game extracts, unknown mirror uploads, and voice lines falsely advertised as public domain. Avoid recognizable third-party games' signature sound designs.

## Historical armored landing cue integration (2026-10-16)

In addition to `Mars des Souvenirs` (the existing owner-original,
packaged/looped Breakwater passive theme), two new original synthesized
one-shot cues are now wired to **authoritative vehicle events**:
`REVERIE_NATIVE_SOUND_ARMORED_GUN=20` only after a deployed armored car
actually damages the battery, and
`REVERIE_NATIVE_SOUND_ARMOR_DESTROYED=21` only when a live armor object is
actually killed by a weapon, artillery, or airstrike. Both are generated
at bank initialization and played by the existing static AudioTrack sound
service; they are **not** samples extracted from the owner's song.
This brings the new secondary event set to sixteen effects beyond the
five mounted-gun sounds, with separate existing reload/support cues.
No external CC0 sample has been imported, and the owner soundtrack
remains separate under its original license. Android/headset mix and
loop continuity are still unverified.

## Historical per-cue sourcing priorities (before shared-library intake)

| Sequence | Cue IDs | Candidate IDs | What constitutes acceptance |
|---|---|---|---|
| P0-A | `gun_machine`, `gun_heavy`, `gun_flak`, `gun_cannon`, `gun_missile` | BW-SFX-004, -005, -011 | Five audibly distinct short reports, one actual shot = one trigger, mixed at non-clipping volume |
| P0-B | `impact_metal`, `impact_sand`, `impact_water`, `boat_destroyed` | BW-SFX-001, -007, -009 | Actual collision/destroy event only, consistent with hit target and location |
| P0-C | `battery_hit`, `gunboat_salvo`, `infantry_volley`, `aircraft_bomb`, `aircraft_strafe` | BW-SFX-004, -005, -007 | Trigger only on authoritative positive-damage event/sequence; cap polyphony |
| P1 | `boat_motor`, `row_oars`, `ramp_drop`, `aircraft_flyby`, `sam_launch`, UI/shop | BW-SFX-002, -003, -006, -010 | Correct on/off/one-shot transitions, stereo direction, pause/Quick Menu ownership |
| P2 | `surf_loop`, `coastal_wind`, `night_air`, wave/mission stingers | BW-SFX-008, -009 plus new individually checked sources | Smooth low-voice, quiet loop beds; phase/location correct; music remains audible |

**Technical gate update (2026-10-15 source):** the legacy
`request_feedback` remains a coalesced bitmask. A separate optional
`request_sound` ABI-v1 host tail now carries five ordered weapon
IDs and uses a bounded eight-event packet/JNI drain; five
**original synthesized** differentiated reports are preloaded in
Android and generic `FIRE`, `RELOAD`, `READY`, `ARTILLERY`
and `AIRSTRIKE` remain fallback cues. The source records only an
implementation, **not Android compilation, verified headset audio,
downloaded CC0 files or released APK**. Event IDs for impacts,
vehicle and aircraft sounds and actual per-file CC0 intake remain
pending.

## Per-file intake record (copy a row when a sound is actually admitted)

| Field | Required entry |
|---|---|
| Source ID and original URL | BW-SFX-### and exact asset page; source author and uploader |
| Exact original file / parent archive | Name, source revision/download date, original file format, package-contained license |
| Copyright/license | Exact SPDX expression (e.g. `CC0-1.0`), evidence URL and date, uncertain provenance discussion resolved |
| Bytes verification | Local **source SHA-256** and edited/encoded **packaged SHA-256** |
| Edit steps | Cropping, sample rate, normalization peak, silence/tail removal, mono/stereo, codec, gain and loop-point recipe |
| Runtime use | `cue_id`, linked simulation event, priority, concurrency/voice budget and lifecycle stop policy |
| Packaging evidence | Exact APK asset path and playback on verified signed APK, no unwanted source archive duplication |
| Quality gate | Device headset audibility, L/R balance, clipping, music coexistence, latency and sustained thermal/CPU cost |
| Status | `candidate → source_checked → selected → imported → wired → tested_on_device → released` with dated evidence; never advance status on plan alone |

Only after bytes are actually imported should the rights and hashes be added to `docs/licensing/ASSET_REGISTRY.json` / per-file REUSE or SPDX metadata. Do not add imaginary entries to the shipped-asset register now. No GitHub Actions, automatic license-gate workflows, remote runtime downloads, or new audio settings controls are requested by this record.
