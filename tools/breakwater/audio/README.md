# Breakwater sound bank preparation

The shipped bank is in `app/src/main/assets/native/breakwater/audio/`.
`art/breakwater/audio_manifest.json` records actual output bytes, source hashes,
recipes, source licenses, levels and tool versions. No original sound pack
or private Drive link is required at runtime.

## Reproduce

Use the original filenames in `source_catalog.json` from the approved shared
sound library, without modifying those masters. The source check rejects any
hash or size mismatch. The original NOX license README and owner-original
asset grant are recorded in `docs/project/BREAKWATER_SOUND_SOURCES.md`.

Requirements: Java (the project uses Java 17+), Python 3 with NumPy, FFmpeg
and ffprobe. These are offline preparation tools, not Android dependencies.
From the repository root, choose scratch paths for originals and outputs:

```bash
mkdir -p /tmp/breakwater-audio/classes /tmp/breakwater-audio/synth
javac -d /tmp/breakwater-audio/classes \
  app/src/main/java/io/github/mrcalzon02/reverievr/NativeCombatWaveform.java \
  tools/breakwater/audio/SynthExport.java
java -cp /tmp/breakwater-audio/classes \
  io.github.mrcalzon02.reverievr.SynthExport /tmp/breakwater-audio/synth
python3 tools/breakwater/audio/prepare_breakwater_audio.py \
  --source-dir /path/to/approved/originals \
  --synth-dir /tmp/breakwater-audio/synth \
  --output-dir /tmp/breakwater-audio/converted \
  --catalog tools/breakwater/audio/source_catalog.json
python3 scripts/verify-breakwater-audio-assets.py
```

If Java includes the compiler module without a `javac` launcher, use
`java --module jdk.compiler/com.sun.tools.javac.Main` in its place.
The source catalog pins the current original synthesizer output as well
as the recordings. A later synthesizer change requires an intentional
catalog/recipe update, not an unchecked acceptance of different bytes.

## Mix and lifecycle contract

Control/progression clips target -22 dBFS active RMS with -12 dBFS peaks;
combat targets -18/-9; loops target -24/-18. Active RMS uses 20 ms blocks
within 25 dB of the loudest block, excluding quiet tails. The tool uses a
smooth-knee compressor to meet the target without hard sample clipping.
Seven loops have six-second periods and 250 ms circular crossfades.
The source and output metrics state what was measured; listening on the
reference handset remains necessary.

`BreakwaterAudioPolicy` owns the fixed cue mapping, gain/rate limits and
900 ms leases. `BreakwaterAudio` owns asynchronous SoundPool loading and
bounded voices. Native code chooses actual event/scene intent; the shell
owns lifecycle, focus, recovery and output. The same recorded clip can
serve related controls, but rejection never adds a success sound.

The aircraft/boat and rowing sounds are designed treatments of machinery
and water recordings. They are not mislabelled location recordings.
Weapons retain identified first-party synthesized components where the
shared collection had no rights-cleared firearm recording.
