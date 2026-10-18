# Test Chamber companion — owned model & audio gallery

The independently installable Test Chamber is a **real owner-asset integration fixture**, not a stack of surrogate colored blocks. It retains the original native C++ procedural calibration room and adds five textured **Beckenburg** GLB model types to the live GLES2 stereo renderer. Six short **original PCM WAV** cues are packaged directly in the game APK. They are deliberately distinct to exercise decoding, SoundPool, touch/Daydream trigger events, hardware volume and Android lifecycle.

| Display | GitHub APK asset | Approved owner source |
|---|---|---|
| Wooden crate | `models/crate.glb` | Beckenburg Drive `wooden_crate_6.glb` |
| Wooden chair | `models/chair.glb` | Beckenburg Drive `chair_mp_5.glb` |
| Storage shelf | `models/shelf.glb` | Beckenburg Drive `shelf_mp_6.glb` |
| Dinner table | `models/table.glb` | Beckenburg Drive `dinner_table_1.glb` |
| Full-size door | `models/door.glb` | Beckenburg Drive `door_1_loose.glb` |

All five models were read from the approved Beckenburg Google Drive library. These are model binaries, including embedded original PNG atlases; the loader handles triangle indices, UVs and the door-handle node translation. Models are displayed across the front of the chamber at actual relative scale with nearest-neighbor PSX texture filtering. No outside public asset website or model was introduced.

The SFX assets under `src/main/assets/sfx/` were generated specifically for this diagnostic fixture as original mono 16-bit/22.05kHz PCM:
`select.wav`, `wood_knock.wav`, `metal_clang.wav`, `coin_chime.wav`, `mechanism.wav` and `beacon.wav`. They are **not misrepresented** as assets extracted from NOX SOUND or original Red Ledger recordings. The Daydream/Cardboard trigger, phone touch, and recognized Android gamepad buttons play the cue assigned to the current gaze target (or the beacon when looking away) and toggle the native chamber tint; `SoundPool` logs playback under `TestChamberSound`.

**Deployment test:** `./gradlew :companion-test-chamber:assembleDebug`, then `python3 scripts/modules/verify-companion-contract.py --apk <actual-debug-apk-path>`. The verifier checks all five textured meshes, their GLB/PNG/index structure, all six real WAVs, that WAV assets remain uncompressed, JNI linkage, packaged GLES2 ABIs and the actual C++ entry point. Then launch on a Galaxy S9 and verify left/right eye geometry, five models (including the door handle), Daydream trigger and sound cycling, pause/resume, audio focus, returning to hub, and temperature. Do not claim the device test happened from a successful source/package audit alone.

## Gaze-based object and audio check (2026-10-18)

Turning the headset to an imported model now raises its brightness without changing the original atlas. Pressing the Daydream/Cardboard trigger plays the corresponding diagnostic cue: crate → wooden knock, chair → select chirp, shelf → mechanism, table → coin chime, door → metallic clang; looking away → electronic beacon. A one-shot welcome beacon sounds only after all six PCM clips are loaded, the Activity is foregrounded, and Android grants audio focus; focus loss pauses the bank and suppresses new cues. The sound bank now pauses and resumes with the Activity, without replaying looping ambience. A touch tap is a fallback for manual phone testing. All of these are source-level additions until validated on an installed Galaxy S9.

The Android-free gaze/sound mapping regression is executable via `sh scripts/modules/test-test-chamber-targeting.sh` and is intentionally independent of the Android SDK/NDK. On 2026-10-18, the exact committed Java targeting source and test fixture passed with `javac` and `java` (five model rays, off-target cases, malformed inputs and six distinct cues). This does not replace device-side Android audio and GL tests.

## Companion build and runtime safeguards (2026-10-18)

The debug companion can use the *same* `phoneTestKeystorePath`, `phoneTestStorePassword`, `phoneTestKeyAlias`, and `phoneTestKeyPassword` Gradle properties as ReverieVR. Pass the already approved publisher key explicitly; the default Android debug certificate is NOT assumed to match the hub. Release assembly rejects a missing publisher keystore. Never commit the keystore or its passwords. Ordinary Android activation key codes (D-pad center, A, button 1, Enter, Space), primary mouse/gamepad button events, and Cardboard trigger are now recognized; key autorepeat does not retrigger cues. Back/Home remain system-controlled.

`TestChamberSounds` now requests Android audio focus while foregrounded, pauses and abandons focus on pause, and logs an explicit warning if focused playback fails. `scripts/modules/verify-companion-contract.py` checks model/WAV bytes against packaged APK bytes, all JNI symbols, the signer gate, input dispatch and audio focus. These are source/build checks; Galaxy S9 speaker/headset, controller and GPU acceptance remains unverified.

## Mesh-accurate gaze targeting (2026-10-18)

The companion now registers the decoded, transformed triangles from the five imported GLBs with its gaze picker as they are staged for the native renderer. Each model's submesh bounds are accumulated, and a forward world-space ray chooses the nearest intersected padded bounds. This replaces the earlier hard-coded station-point approximation, so scaled geometry and separate mesh pieces can be selected where they actually appear. The gaze path avoids temporary arrays. A source verifier compares the Java placement transforms against native GLES2 placement and rejects mismatches.

The Android-independent regression checks off-center geometry, all five targets, second submeshes, sky and rear gaze, invalid data, and six distinct sound assignments. Java source blob hashes match tested local files and GitHub readback.

Remaining gate: Android SDK/NDK APK compilation and actual Galaxy S9 stereo rendering, controller selection, and audible sound verification. No new APK release or headset acceptance is claimed.
