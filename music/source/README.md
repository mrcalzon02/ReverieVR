# ReverieVR Source Music Library

**Inventory:** 11 uploaded MP3s, 66,325,607 bytes (about 63.3 MiB). This
directory is for soundtrack **source assets**, not automatically packaged
application data. Track identities, original names, byte counts, SHA-256
fingerprints and intended use are defined in [manifest.json](manifest.json).

**Source binaries verified:** All eleven original MP3s are in
`music/source/audio/` on remote GitHub `main`. They are the only
tracked masters. Debug and phoneTest Gradle builds now verify their
byte lengths and SHA-256 checksums, then stage playable copies in
ignored Gradle build output; the full 66.3 MB is **not duplicated as
Git-tracked audio**. A separate pending-rights preview catalog
`music/source_preview.tsv` is generated only for those test variants,
and the current player combines it with the approved Starry Cereal
catalog. Unrestricted release variants exclude pending tracks.
No new streaming or network connection is introduced.

## Assignments (intent, not shipped playback)

| Exact source name | Destination | Status |
|---|---|---|
| The Last Note Hangs Unresolved.mp3 | Bar project | Source cue, pending import |
| Mars des Souvenirs.mp3 | Beachhead 2000-style native project | Source cue, pending import |
| Last Bugle Call.mp3 | Ministry of Intelligence | Source cue, pending import |
| Sky Lobby Drift.mp3 | ReverieVR lobby alternate | Source cue, pending import |
| Glass Lobby Drift.mp3 | ReverieVR lobby alternate | Source cue, pending import |
| Ballroom of the Dead.mp3 | General music library | Unassigned source |
| Deep Embrace.mp3 | General music library | Unassigned source |
| End Of Deep.mp3 | General music library | Unassigned source |
| Nowhere King.mp3 | General music library | Unassigned source |
| Pressure Under Europa.mp3 | General music library | Unassigned source |
| World Below.mp3 | General music library | Unassigned source |

Starry Cereal remains the currently packaged default. Sky Lobby Drift and
Glass Lobby Drift are alternate candidates for a future session/track
rotation, **not** active lobby playback yet. The user-provided originals
remain outside `app/src/main/assets` and `app/src/main/res`, so adding
source files does not silently inflate the phone APK by ~66 MB. The
`NativeMusicCatalog` must continue advertising *only* audio actually
present in the APK; a missing source file must not create an inert in-VR
music item.

## Rights and distribution boundary

Source transfer does **not** imply MIT, CC BY, CC0 or public-domain rights
for the audio. Rights and official download/commercial-use entitlement
have **not** been independently attested per track. Existing Suno-specific
packaging rules require each song's paid-download and per-track records
before admission to the distributed APK. Music may also originate outside
Suno; do not misattribute unidentified uploads.

Publishing these source MP3s to this **public** repository redistributes
their bytes. Before running the importer with `-Publish`, confirm rights to
publish the recordings and any included third-party samples. Record a
file-specific license/provenance review; do not let the repo's software
MIT or documentation CC BY glob accidentally license these audio files.
`LicenseRef-Pending-Asset-Rights` means **rights review required**, not
permission granted.

## Completed binary import (historical Windows instructions)

From an up-to-date, authenticated local checkout of `main`, extract the
`Import-ReverieMusic.ps1` script from the supplied music ZIP into the
checkout root, then execute:

```powershell
powershell -ExecutionPolicy Bypass -File .\Import-ReverieMusic.ps1 -Bundle "C:\path\ReverieVR_Source_Music_11_Tracks.zip" -Publish
```

The importer validates all 11 source MP3 filenames, lengths and SHA-256
values *before* changing repository content, refuses divergent existing
files, commits only the specified source binaries, pushes without force,
retries transient push failures up to three attempts, and reads back
`origin/main` to verify its commit. If push or read-back fails, the
local commit stays intact and must **not** be reported as remote success.

The importer finished successfully; the manifest records
`source-binaries-checked-in`. This historical reimport command is no
longer required. Test-build music playback is now integrated, while
an unrestricted commercial/public release still requires individual
rights and attribution review.
No GitHub Actions change is necessary.
