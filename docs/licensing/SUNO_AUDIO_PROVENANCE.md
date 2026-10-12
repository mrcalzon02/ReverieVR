# Suno music and owner-original ReverieVR recordings

The user identifies [Jovian-PostPunkRevival](https://suno.com/@jovianpostpunkrevival7375)
as their artist account and confirms commercial usage of the already
packaged **Starry Cereal** soundtrack under
`LicenseRef-Suno-Commercial-Use`. That permission does not imply a
Creative Commons or MIT license, nor permission to automatically download
other public-profile music. Future Suno imports from that profile still
need legitimate owner-approved downloads and matching checksums.

## Eleven additional original recordings (2026-10-12)

**The project owner now explicitly confirms all eleven supplied MP3s
are their own original musical products and that they have the commercial
rights required to include, use and distribute them in ReverieVR.**
This is the authorization for their inclusion in the normal application,
not an unsupported inferred grant from a public streaming page.

See [Owner Original Music](OWNER_ORIGINAL_MUSIC.md) and the
[track manifest](../../music/source/manifest.json) for the exact
recording names, source hashes, byte counts and game assignments.
These tracks use `LicenseRef-Owner-Original-Music`, independently of
Starry Cereal's Suno-specific license. We have not assumed which of
the additional recordings were produced by Suno, whether any involved
a particular plan, or transferred unrelated third-party rights.
We record the owner's express original-work/commercial-rights declaration.

## Build and runtime

The existing `app/src/main/assets/music/catalog.tsv` still records
Starry Cereal alone under its specific Suno label. The generated
`assets/music/owner_library.tsv` records all eleven
`LicenseRef-Owner-Original-Music` tracks in every APK variant.
`ShellMenuMusic` combines both catalogs at startup. Each of the eleven
original MP3s is copied once into generated build assets, using the
single Git master under `music/source/audio`. Gradle validates source
SHA-256, byte length, filename/ID, and staged SHA-256 before the APK
asset merger. The license audit checks required APK assets and original
source digests. No cloud, PC companion, Suno streams, OCR or visual
scanning are involved.

Adding 66,325,607 bytes (~63.3 MiB) of audio is a deliberate phone
storage and APK size tradeoff specifically requested by the owner.
The music is commercially authorized for inclusion inside ReverieVR,
but not licensed to arbitrary downstream parties for independent
commercial music redistribution or general MIT/CC reuse.

Future unrelated Suno tracks remain governed by the individual
download/entitlement workflow and the public-profile link alone
confers no authorization or bytes.
