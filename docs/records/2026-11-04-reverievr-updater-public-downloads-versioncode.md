# 2026-11-04 — Android phone-test in-app update and Downloads repair

Scope: ReverieVR Android shell updater only; no game logic, content, signing or save-state mutation.

## Observed defect

On Galaxy S9 / Android 10, the download notification completed but ReverieVR rejected the APK as "not newer than the installed ReverieVR build". The updater compared GitHub release **publication timestamps** with the installed build timestamp before downloading; the verifier compared the downloaded APK's actual Android `versionCode` with the installed package only afterward. A later-published stale release could therefore appear as an update, download in full, and be rejected. Source code also directed DownloadManager to app-specific external storage rather than the public Downloads folder.

The GitHub releases feed examined at the start of this repair showed the newest published phone-test release as `phone-test-1625-1` (published 2026-10-29, versionCode 215403423); newer development commits and successful skipped-release gates did not prove a new signed APK had been published.

## Repair

- Pre-download release selection requires the declared Android `versionCode` to be greater than `BuildConfig.VERSION_CODE`, with the verifier independently checking the downloaded APK against its release metadata, SHA-256, package name and signing identity. Workflow run numbers and `published_at` are no longer install eligibility criteria.
- Version-code differences are shown in rejection feedback, without permitting downgrades.
- On Android 10+, DownloadManager writes a uniquely named APK to the **public Downloads folder** with the APK MIME type; the same managed download URI is used for the Android package install intent. Android 8/9 retain their previous app-specific download fallback until legacy permission handling is separately implemented.
- No release signing gate, digest check or Android package installer validation was removed.
- The changed `.github/phone-test-trigger` requests the existing signed phone-test release workflow; no duplicate workflow is introduced.

## Acceptance and evidence boundary

Source/host validation: verify updater unit tests, Android compile, signed APK's actual versionCode and signer, GitHub release read-back. **Physical Galaxy S9 in-place update, visible public Downloads entry, user settings and save persistence remain device acceptance gates, not claimed verified here.**

Once an APK is published, test an installed older build updating to the new signed APK and check My Files > Internal storage > Download, launch, game saves and signing continuity. Do not treat an Actions gate marked success with a skipped publication job as a released APK.
