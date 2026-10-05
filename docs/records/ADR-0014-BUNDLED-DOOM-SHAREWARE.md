# ADR-0014 — Bundled DOOM Shareware Sample Content

**Status:** Accepted as implementation baseline  
**Date:** 2026-10-05

## Context

ReverieVR needs a legally distributable DOS title that can serve as:

- a first-launch demonstration;
- an input-binding reference target;
- a 320x200 / intended-4:3 framebuffer validation title;
- a DOSBox Pure runtime smoke test;
- a recognizable test for head-yaw-to-mouse and controller-to-keyboard mapping.

DOOM Shareware v1.9 is a strong technical fit, but it remains copyrighted
shareware. It is not public domain and its data is not GPL-covered merely
because the DOOM engine source was later released under the GPL.

Historical DOOM shareware terms permitted electronic redistribution in
compressed form and restricted charging/consideration without permission.
Debian's long-standing `doom-wad-shareware` package also records a 1999 written
clarification from John Carmack that the DOOM shareware WAD is freely
distributable.

## Decision

ReverieVR may include **DOOM Shareware v1.9** as default sample content in
free/noncommercial distributions under the following conservative rules.

### Distributed payload

The distributed third-party payload is the original archive:

- `doom19s.zip`
- size: 2,450,688 bytes
- SHA-1: `8d0fbbbeba5ecb692a99f97e55dfb5365cfe5b77`
- SHA-256:
  `cacf0142b31ca1af00796b4a0339e07992ac5f21bc3f81e7532fe1b5e1b486e6`

ReverieVR does not redistribute a hand-modified or repacked shareware tree as
its authoritative payload.

The archive is nested unchanged inside the APK asset set. Local copying,
installation or extraction occurs only on the end user's device.

### Provenance

The fetch helper uses the idgames archival mirror at:

`https://www.gamers.org/pub/idgames/idstuff/doom/doom19s.zip`

and verifies the exact SHA-256 before the archive becomes eligible for
packaging.

The binary is not committed into the ReverieVR Git repository. The repository
commits the fetch process, expected digest, provenance and license/distribution
policy.

### Release behavior

A normal ReverieVR release is expected to include the verified shareware
archive.

A release build without the archive fails unless the builder intentionally sets:

`-PexcludeDoomShareware`

That escape hatch exists for commercial/licensing variants and other deliberate
no-content distributions.

### Commercial-distribution boundary

The original shareware terms restrict charging or receiving consideration for
the shareware without appropriate permission.

Therefore a ReverieVR distribution that is sold, bundled into a paid product,
or otherwise creates uncertainty about consideration for the included DOOM
shareware must not simply assume the free-shareware redistribution grant still
covers that distribution.

For such a release, ReverieVR must either:

1. obtain appropriate permission from the current rights holder; or
2. build with `-PexcludeDoomShareware`; or
3. substitute content with clearly compatible redistribution rights, such as a
   separately reviewed free-content option.

This is a release gate, not a runtime toggle.

### Runtime module

If the archive is present, ReverieVR verifies its SHA-256 again at runtime and
registers a built-in module:

- display name: `DOOM Shareware v1.9`
- module id: `bundled-doom-shareware-1.9`
- binding profile: `dos-doom-shareware`

The current archive remains the original installer distribution. ReverieVR
creates a separate app-private runtime working directory on the end user's
device, safely expands the verified outer ZIP there, and generates only
Reverie-owned launch metadata/configuration alongside the extracted installer.

The generated runtime uses DOSBox Pure's own `DOS.YML` `run_path` /
`run_input` mechanism to start a small bootstrap batch and supply the original
DEICE install prompts. The batch then invokes the generated
`DOOMS_19.EXE -d`, installs a Reverie-generated `DEFAULT.CFG`, and launches
`DOOM.EXE`. Once `DOOMS/DOOM.EXE` exists, launch metadata is rewritten to
start the game directly and installer input automation is removed.

The distributed `doom19s.zip` itself is never rewritten. The generated runtime
tree is derived local state and is not the authoritative redistributed payload.

For the first bootstrap configuration, Sound Blaster SFX target DOSBox's
standard SB16 base 0x220 / IRQ 7 / DMA 1 configuration. Music is deliberately
disabled in the generated config until the separate music/audio validation pass;
this avoids forcing the user through the historical SETUP.EXE menu merely to
reach the first playable frame.

## Rejected alternatives

### Bundle a registered/full DOOM IWAD

Rejected. Commercial/registered DOOM content is not covered by the shareware
redistribution path.

### Treat DOOM1.WAD as GPL because the engine source is GPL

Rejected. Engine source licensing and game-data licensing are separate.

### Repack the installed shareware files into a Reverie-specific archive

Rejected as the default distribution path because preserving the original
archive byte-for-byte gives a much clearer provenance and redistribution
boundary.

### Download DOOM silently after installation

Rejected as the normal default. ReverieVR should remain useful offline and a
release that claims to include a default test game should actually contain it.

A later user-initiated re-download/repair action may still be useful.

## Consequences

ReverieVR gets a recognizable, low-resolution DOS reference title by default
without making DOOM a proprietary hidden dependency.

The release process gains a third-party-content verification step.

Commercialization decisions must explicitly revisit the bundled-shareware gate
instead of accidentally carrying old shareware terms into a paid distribution.
