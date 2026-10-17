# Between Deliveries — audible owner-original soundtrack routing (2026-10-16)

## Root-cause / source evidence

The actual native module ID is `between-deliveries-red-ledger` in both `NativeModuleRuntime.ID_RED_LEDGER` and `red_ledger_module.cpp`. `VrActivity.onNativeModulePlaybackRequested` passes the successfully launched module ID into `ShellMenuMusic.setHostedNativeModule`. That player selects a **packaged** owner-original catalog track, opens its seekable MP3 through Android `MediaPlayer`, applies the existing music volume/mute settings, and loops while hosted. On exit, `setShellActive(true)` resumes the normal Home playlist; `pauseForLifecycle()` retains app-focus audio behavior.

The existing `NativeMusicPlaybackPolicy` had the owner's bar track **incorrectly** bound to the retired planning label `bar-project`, not the registered ID. As a result `cueForNativeModule` returned null, the catalog lookup returned -1, and the player was disabled on launch. The fix changes the cue mapping **only** for the real Between Deliveries ID to `the-last-note-hangs-unresolved`; removes the dead alias; keeps Breakwater, Ministry and Home music assignments untouched.

The actual original MP3 `music/source/audio/The Last Note Hangs Unresolved.mp3` is **already** in Git: 6,568,782 bytes, catalog SHA-256 `6be461d1d39bfd5f84d63a00562eb1fec905b121c231a4a2fa2c919800f6987d`, owner-commercially authorized 2026-10-12 under `LicenseRef-Owner-Original-Music`. No track is duplicated, substituted, downloaded at runtime or publicly relicensed. The existing Gradle `stageOwnerMusicAssets` validates each owner master, produces `music/tracks/the-last-note-hangs-unresolved.mp3` and `music/owner_library.tsv`, and includes these in every variant.

## Verification and limitations

* Verified exact Java route source (Git blob `7809ce101f20ee5e95a7400019dd44318c5aaa49`) by independently reproducing its blob hash, then compiling that **exact** route class using host `javac`. The smoke test **passed**: correct Red Ledger cue, obsolete alias not routed, Breakwater unaffected, and no Red Ledger music leaking into lobby rotation.
* Added a JUnit source regression checking `NativeModuleRuntime.ID_RED_LEDGER`, exact commercial music id, owner MP3 path and license, and obsolete-alias rejection. Full Android Gradle/JUnit task, resulting APK contents, Galaxy S9 `MediaPlayer` decode, Android audio focus/volume behavior and headphone listening are **not verified** by the host smoke test.
* This soundtrack is **owner-authorized original music**, not the original Unreal `Content/Sounds/Music` packages. The private Drive project also has 14 additional music `.uasset` containers (including Western Concordant and UED faction anthems). These have **not** been exported into ordinary playable WAV/MP3 or rights-reviewed as individual files for redistribution. Do not claim they play in the current native game. When they are exported/licensed, the existing shell music catalog and cue policy are the correct integration surface, **not** a second game-specific MediaPlayer.

## Acceptance

Source routing and host-policy smoke test passed. No Android/device audio claim. After packaging, enter the registered Red Ledger module from Development, verify `The Last Note Hangs Unresolved` audibly loops at controlled volume, test pause/resume and mute, exit to Home to verify prior lobby playlist restoration, and verify other native games never accidentally play Red Ledger's cue. Do not use GitHub Actions.
