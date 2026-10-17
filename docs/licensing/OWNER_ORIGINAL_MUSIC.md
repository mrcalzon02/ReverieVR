# Eleven owner-original commercial soundtrack masters

**Authorization (2026-10-12):** the project owner directly declares that
all eleven uploaded MP3 recordings in `music/source/audio/` are their own
original works, that they hold the necessary commercial rights, and that
ReverieVR may package, publish, play, and distribute these songs as part of
the ReverieVR app and its first-party projects.

The [manifest](../../music/source/manifest.json) contains the eleven exact
track names, lengths, hashes, intended project/cue assignments, and this
authorization. The codebase checks the source and packaged checksums.

**Owner-original distribution label:**
`LicenseRef-Owner-Original-Music`. Unlike software MIT, this authorization
does not grant downstream users independent blanket music sublicensing or
Creative Commons rights. We don't assert a specific Suno generation history,
official download permalink or universal open-content rights for these files.
Starry Cereal retains its separate earlier Suno-commercial rights label.

All eleven tracks are included in every default APK build, adding
66,325,607 bytes of MP3 source content before APK overhead. No remote
streaming, OCR, computer vision, music metadata telemetry or PC companion
is required. Tracks are sourced once in Git and staged by build time
with strict SHA-256 verification.

**Assignments:** Sky Lobby Drift and Glass Lobby Drift alternate with
Starry Cereal. Mars des Souvenirs is the cue for Breakwater Battery.
Last Bugle Call is bound to the future Ministry of Intelligence native
module. The Last Note Hangs Unresolved is assigned to the **registered**
Between Deliveries: The Red Ledger native module
(`between-deliveries-red-ledger`) and loops through the shell's existing
licensed MP3 player after a successful module launch. The shell remains
responsible for music volume, mute, lifecycle pause and returning to Home.
The other six tracks are available in the user-facing music library.
