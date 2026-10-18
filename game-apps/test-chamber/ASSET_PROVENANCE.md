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
`select.wav`, `wood_knock.wav`, `metal_clang.wav`, `coin_chime.wav`, `mechanism.wav` and `beacon.wav`. They are **not misrepresented** as assets extracted from NOX SOUND or original Red Ledger recordings. Daydream trigger or touch cycles through the six effects and toggles the Test Chamber native tint; `SoundPool` logs successful and unsuccessful playback under `TestChamberSound`.

**Deployment test:** `./gradlew :companion-test-chamber:assembleDebug`, then `python3 scripts/modules/verify-companion-contract.py --apk <actual-debug-apk-path>`. The verifier checks all five textured meshes, their GLB/PNG/index structure, all six real WAVs, that WAV assets remain uncompressed, JNI linkage, packaged GLES2 ABIs and the actual C++ entry point. Then launch on a Galaxy S9 and verify left/right eye geometry, five models (including the door handle), Daydream trigger and sound cycling, pause/resume, audio focus, returning to hub, and temperature. Do not claim the device test happened from a successful source/package audit alone.
