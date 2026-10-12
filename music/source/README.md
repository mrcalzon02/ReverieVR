# ReverieVR Owner-Original Music Library

**11 original MP3 recordings • 66,325,607 bytes (63.3 MiB).**

The project owner explicitly confirms authorship/ownership and commercial
rights for all eleven recordings as of 2026-10-12, and instructs ReverieVR
to distribute and play them in normal builds. The authorization is recorded
in [OWNER_ORIGINAL_MUSIC.md](../../docs/licensing/OWNER_ORIGINAL_MUSIC.md)
and [LicenseRef-Owner-Original-Music](../../LICENSES/LicenseRef-Owner-Original-Music.txt).
The music is not licensed under the code's MIT grant, the documentation's
Creative Commons license, CC0, or public domain.

All eleven original MP3s are checked into `music/source/audio/` on GitHub
with exact byte lengths and SHA-256 checksums in [manifest.json](manifest.json).
They are the sole Git-tracked masters. The regular Gradle build uses
`stageOwnerMusicAssets` to validate, stage and package all eleven
recordings into `assets/music/tracks/<track-id>.mp3` in **every**
APK type (debug, phoneTest, release). It also generates
`assets/music/owner_library.tsv`, combined by the offline native music
player with the existing Starry Cereal catalog. The extra 66.3 MB is an
intentional full-APK cost. No additional network request, streaming or
PC companion is needed.

## Original tracks and application routing

| Original recording | Project or use |
|---|---|
| The Last Note Hangs Unresolved.mp3 | Bar project; future native module cue |
| Mars des Souvenirs.mp3 | Breakwater Battery (Beachhead-style) game's looping music |
| Last Bugle Call.mp3 | Ministry of Intelligence; future native module cue |
| Sky Lobby Drift.mp3 | ReverieVR lobby alternative |
| Glass Lobby Drift.mp3 | ReverieVR lobby alternative |
| Ballroom of the Dead.mp3 | General music library |
| Deep Embrace.mp3 | General music library |
| End Of Deep.mp3 | General music library |
| Nowhere King.mp3 | General music library |
| Pressure Under Europa.mp3 | General music library |
| World Below.mp3 | General music library |

All eleven recordings are included in the Music Library regardless of
game assignment. Automatic Home music cycles Starry Cereal, Sky Lobby Drift,
and Glass Lobby Drift, across session starts and track completion; none
of the other nine recordings is silently used as lobby music.
A hosted game with an assigned soundtrack plays that specific cue.
Unmatched native games, DOS and the video player continue muting
shell music while playing.

The two future bar/Ministry module identifiers exist as *routing policy*
only, pending their actual admission to ReverieVR. No functionality is
claimed for modules that are not yet registered.

## No duplicate imports or alternate licensing

**Do not reimport these MP3s from an old ZIP.** GitHub already contains
their exact original binary data and the Gradle build stages it from
there without creating a second tracked blob directory.
Do not automatically classify the recordings as public-domain, CC or
Suno-only content; the explicit original-owner commercial license covers
this set. New third-party audio requires a separate rights assessment.

Galaxy S9 actual playback/thermals and assembled APK verification remain
their own device acceptance gates; source commits alone do not prove
a signed APK was built or installed.
