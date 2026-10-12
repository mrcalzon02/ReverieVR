# Suno music distribution — Jovian-PostPunkRevival

Artist/profile (confirmed by ReverieVR owner, 2026-10-11):
https://suno.com/@jovianpostpunkrevival7375

At review the publicly accessible profile displayed **738 songs**. A
public-profile link confirms discoverability, not the download entitlement
or the audio bytes necessary to include a track in a self-contained APK.

**Owner authorization:** The owner has identified the profile as their
own and authorized commercially entitled published works for ReverieVR's
native player and default distribution. The already-bundled track **Starry
Cereal** is specifically owner-confirmed as held under paid Suno
commercial-use rights. This is not a blanket CC BY, CC0 or MIT grant
to anyone who clones the repository.

**Suno 2026 terms:** Commercial exploitation requires the track to be
obtained via a permitted download on a qualifying paid tier, and a
commercial right can vary by generation, free-plan history, remix,
extension, or uploaded third-party material. Public streaming is not a
permitted download; do not stream-rip, scrape audio CDNs or reconstruct
clips from exposed player endpoints. Document official download
entitlement individually before adding track audio to the APK.
Sources: https://suno.com/terms-of-service ,
https://help.suno.com/en/articles/9601665 ,
https://help.suno.com/en/articles/2410177 .

**2026-10-12 uploaded source candidates:** Eleven additional MP3s were
supplied directly to the project. A SHA-256/size inventory and intended
uses are recorded at `music/source/manifest.json`, while the binaries
remain pending authenticated import to the public repository. No Suno
track permalinks, generation-plan records, sampling/rework provenance,
or legal title-by-title clearance accompanied the files. Do not
automatically presume that all eleven are Suno recordings or that any
is entitled to commercial APK inclusion. This source-only inventory
does not modify `assets/music/catalog.tsv`, the native music player,
or the approved APK license registry. A later approved packaging pass
must update those together with evidence.

**Build/runtime:**
- `app/src/main/assets/music/catalog.tsv` is the frozen music index
  shipped in the base APK. This repository currently includes Starry
  Cereal alone, in `res/raw/starry_cereal_menu.mp3`.
- The `Music Library` button in the VR Home opens a native local music
  player. It must never depend on Suno's website, network or desktop PC.
- Additional owner-approved downloadable tracks can be imported as
  `assets/music/tracks/<id>.mp3` or `.wav`, with SHA-256 and per-file
  `LicenseRef-Suno-Commercial-Use` entries. The distribution registry
  and catalog are updated together. Actual APK inspection must reject
  unregistered audio and hash mismatch.
- User-managed download files are separate from the public Suno page;
  do not auto-download all published tracks based on the profile count.
- Keep the base APK realistically small for the Galaxy S9. If all tracks
  cannot fit comfortably, use owner-authorized offline music packs or
  explicit on-phone import; do not quietly force a PC/cloud music runtime.

This record should be updated with the exact track permalink, legal
download evidence (privately held where appropriate), source SHA-256,
and package SHA-256 for every new admission. No evidence has been
received for the other published audio files yet.
