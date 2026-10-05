# DOOM Shareware v1.9 bundled sample content

ReverieVR may include the original DOS **DOOM Shareware v1.9** archive as
default sample content for the embedded DOS runtime.

## Exact archive

- Filename: `doom19s.zip`
- Author/publisher: id Software
- Historical version: DOOM Shareware v1.9
- Size: 2,450,688 bytes
- SHA-1: `8d0fbbbeba5ecb692a99f97e55dfb5365cfe5b77`
- SHA-256: `cacf0142b31ca1af00796b4a0339e07992ac5f21bc3f81e7532fe1b5e1b486e6`
- Current archival mirror used by the fetch helper:
  `https://www.gamers.org/pub/idgames/idstuff/doom/doom19s.zip`
- Historical id Software/idgames location:
  `ftp://ftp.idsoftware.com/idstuff/doom/doom19s.zip`

The binary archive is intentionally not committed to the ReverieVR Git
repository. Run `scripts/fetch-doom-shareware.sh` or the Windows `.bat`
equivalent. The helper verifies the SHA-256 before making the archive available
to the Android build.

## Redistribution posture

DOOM Shareware is copyrighted software, not public domain.

Historical shareware terms permit electronic redistribution in compressed form.
The original license also restricts charging or receiving consideration for
receipt/use of the shareware without id Software permission. Debian's
`doom-wad-shareware` package records a 1999 written clarification from John
Carmack that the DOOM shareware WAD is freely distributable.

ReverieVR therefore uses the conservative packaging rule:

1. distribute the **original `doom19s.zip` archive byte-for-byte**;
2. do not alter, repack, patch, or substitute files inside that archive for the
   distributed payload;
3. preserve this provenance record and the license/vendor material contained in
   the original shareware distribution;
4. copy/install/extract only on the end user's device after distribution;
5. do not represent the shareware as open source, GPL, public domain, or owned
   by ReverieVR;
6. do not bundle registered/commercial DOOM IWADs;
7. free/noncommercial ReverieVR releases include the verified shareware archive
   by default;
8. a release intended to charge for the bundled shareware or otherwise create a
   commercial distribution relationship must obtain appropriate permission or
   explicitly omit the shareware payload.

The Gradle escape hatch for an intentionally no-DOOM build is:

    -PexcludeDoomShareware

That flag exists for licensing/distribution variants; it is not the ordinary
free-release default.

## Runtime behavior

When present, Android packages the unchanged archive as:

    assets/default-content/doom19s.zip

On application startup ReverieVR verifies the SHA-256 again, copies the archive
unchanged into app-private DOS-module storage, and registers:

- module: **DOOM Shareware v1.9**
- binding profile: `dos-doom-shareware`

The current archive is the original shareware installer distribution. Until the
DOSBox Pure host's install/autostart flow is finished, the module may initially
enter the original installation path rather than jumping directly into E1M1.
That remaining behavior is a runtime implementation task, not a reason to
repackage the copyrighted archive.
