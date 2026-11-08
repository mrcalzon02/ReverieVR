# Breakwater shared-library sound overhaul — 2026-11-08

## Scope and starting authority

The owner requested a broad sound overhaul of Breakwater inside the native
games application, using the existing shared sound library, normalized control
feedback and environmental audio. Starting `main` was
`12d4018c90e56a3767b8ffc58a2e5394440caec1`. This remains the hosted
`breakwater-battery` module in the unified ReverieVR application.

The previous game used generated static AudioTrack reports and generic shell
tones, with Home's Dunehack ambience also selected while Breakwater was hosted.
The shared library's actual NOX nature/vehicle/water recordings and authorized
owner-original grenade/metal recordings are now admitted as runtime derivatives.
No web candidate pack was substituted for the owner's collection.

## Implemented sound and interaction

- The 46-file bank covers controls/progression, five weapons, actual impacts,
  destruction, hostile attacks, support actions and seven environmental layers.
- Focus is announced after 45 ms on a stable target with a 100 ms minimum
  interval. Slots, difficulty, shop, armory, rack, support console, stations and
  results participate. Activation/rejection suppresses same-frame focus.
- Equipment, reload start/ready, empty gun, aim limit, cooldown, unavailable
  support, support arm/disarm, menu open/back, difficulty, restart, purchase and
  station movement have contextual feedback. Held fire does not repeatedly
  reject during its normal cadence or reload. Equipping an owned item does not
  claim another purchase.
- Actual failed checkpoint writes announce rejection and suppress the
  corresponding control confirmation. Optional-save hosts retain their normal
  controls. Real wave starts and combat outcomes are not silenced by a failed
  save; the current in-memory campaign remains usable.
- Actual weapon reports precede impacts; the automatic support turret now has
  its firing report. The native scheduler preserves outcomes/actions and
  coalesces identical simultaneous impacts within the existing eight-event
  limit. No draw/eye callback generates sounds.
- Surf and wind follow the game scene; night ambience follows its period.
  Nearby boat machinery, rowing and aircraft follow the nearest active actor,
  including the heavy transport's real placement. Headset listener position
  and orientation determine stereo pan; controller aiming does not.
- Traverse audio follows meaningful mounted-gun motion and stops over controls.
  The seven loop intents share the actual two-event spatial packet through a
  fair 120 ms refresh. Zero-gain stops take precedence; a stale lease expires
  after 900 ms with a short fade.

The preserved `Mars des Souvenirs` soundtrack remains in `ShellMenuMusic` with
its existing separate music controls. The Home ambience no longer overlays
Breakwater. Existing native IDs 1–27 retain their meanings; contextual controls
and fixed loop IDs append at 28–47 without changing the ABI structure layout.
Old hosts retain supported generic feedback through the optional callback path.

## Normalization, budgets and source evidence

The admitted WAVs total **2,608,902 bytes**. They use canonical PCM16 mono at
22,050 Hz; the largest decoded clip is 264,600 bytes. Seven six-second loops
have a 250 ms circular crossfade. Runtime conversion and downloading are absent.

| Family | Active RMS target | PCM peak ceiling | Maximum voices | Playback gain |
|---|---:|---:|---:|---:|
| Control/progression | -22 dBFS | -12 dBFS | 2 | 0.20 |
| Combat | -18 dBFS | -9 dBFS | 4 | 0.22 |
| Environment | -24 dBFS | -18 dBFS | 7 | 0.06–0.12 per layer |

The preparation tool removes source DC and unwanted silence, applies recorded
edits and smooth-knee compression, and measures the quantized result. Active
RMS uses 20 ms blocks within 25 dB of the loudest block. Measured output error
is below 0.01 dB, and no output sample clips. Every loop boundary step is below
the clip's ordinary 99th-percentile adjacent-sample step.

The conservative bank peak sum is below 0.50 even at maximum legal gains and
all voice slots; the existing soundtrack has a separate 0.50 maximum. The
platform and listening device still determine perceived loudness and latency.

The [manifest](../../art/breakwater/audio_manifest.json) records actual source
and output hashes, transformations, per-file measurements and tool versions.
The [source register](../project/BREAKWATER_SOUND_SOURCES.md), asset registry,
REUSE entries, original NOX CC0 README and existing owner-original grant retain
provenance. Boat/aircraft/rowing sounds are designed treatments of recorded
vehicle/water material. Weapon components remain identified first-party
synthesis where no cleared firearm recording existed; the sand cue is
procedural. These are not claimed as authentic field recordings.

## Playback ownership and recovery

`BreakwaterAudio` owns asynchronous loading, three bounded SoundPools, stream
handles and loop fades on the UI thread. `BreakwaterAudioPolicy` owns cue
mapping, gains, rate limits and leases. Native code supplies event/scene intent.
Queued work captures the session generation before crossing threads, so old
events cannot play after exit, replacement or pause.

Quick Menu, controller loss, invalid listener, Activity pause, focus loss,
headphone/output interruption and exit stop game voices. Lifecycle pause and
resume discard old loop leases and gunshots. Breakwater requests game audio
focus while active; permanent focus loss and output interruption can be retried
through the existing Quick Menu Resume action. Tracking recovery alone never
requests focus from another application. Exit releases the pools.

Quick Menu controls share the normalized control pool while gameplay is
suspended. Unready or failed Breakwater cues never use louder legacy tones or
combat voices. A failed pool is released and rebuilt after a per-pool minimum
three-second backoff; unrelated categories remain available. Missing assets
and failed decode retries also obey that deadline. Successful playback or
intentional priority suppression alone commits a cue's rate limit. Compound
requests never replay a successful component through another bank.

## Executed evidence and remaining gates

The final actual-module Update regression passes 2,247 assertions on
GCC, with the same result under AddressSanitizer/UndefinedBehaviorSanitizer.
It exercises real ray hit tests and the actual bounded host queues, all five
guns, controls, held-input behavior, outcomes, seven-layer queue pressure at
30 Hz and 100 ms updates, old input-prefix allocation, listener transforms and
audio-free frame/eye rendering. LeakSanitizer could not run in the traced
execution environment; this is not a leak-clean device claim.

Real save callbacks additionally cover successful and rejected difficulty,
purchase, armory, debrief and checkpoint-clear writes, valid serialized
payloads, one failure callback and no held-error repetition. The entire
existing `breakwater_campaign_test.cpp` passed strict GCC C++17 with assertions
active: `Breakwater boat / landing / upgrade smoke passed`.

The actual Java bank/policy/music/routing backend passes **16 scenarios and
226 checks**, including asynchronous/late decoding, pool replacement, load and
play failures, priority eviction, 2/4/7 voice budgets, lease expiry, normalized
Quick Menu controls, focus loss while already suspended and mix headroom.
The separate UiFeedback lifecycle/routing harness passes **18 checks**; the
checked-in `BreakwaterAudioPolicyTest` passes **7 JUnit tests** unchanged.

`python3 scripts/verify-native-abi-contract.py` passes. The independent
`scripts/verify-breakwater-audio-assets.py` reads actual PCM and validates
every mapping, source/output hash, registry entry, format, level and seam.
All 46 prepared WAVs plus the manifest reproduced byte-for-byte in a separate
output directory. The native documentation guard still reports the baseline
Test Chamber shader-helper mismatch, also recorded before this work; that
unrelated implementation is not changed or waived here. The guard's separate
same-change documentation mapping passes for this change. The existing
23-shared-asset format/hash/scope/package check also passes.

Android/package evidence is recorded after its actual build and archive
inspection. Physical Galaxy S9/Daydream listening, Bluetooth output latency,
subjective balance with music and sustained thermal performance remain
device acceptance gates. No local stub test is described as handset audio.

## Reproduce the focused checks

```bash
python3 scripts/verify-breakwater-audio-assets.py
python3 scripts/verify-native-abi-contract.py
bash scripts/verify-breakwater-audio.sh
bash scripts/audio/verify-breakwater-backend.sh
```

The native runner needs C++17, zlib and GLES2 link development files. Its
`BREAKWATER_GL_CFLAGS` / `BREAKWATER_GL_LIBS` overrides select an existing local
toolchain. `BREAKWATER_AUDIO_TRACE=1` prints deterministic actual event traces;
that is simulation evidence, not an audio capture. The backend runner uses the
actual Java bank/policy/routing sources and deterministic Android media stubs.
The standard Android unit task also runs the pure policy tests. Use
`python3 scripts/verify-breakwater-audio-assets.py --apk <signed.apk>` to compare
the actual released archive and require stored, byte-identical WAV entries.
