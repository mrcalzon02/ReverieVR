# ReverieVR — Optional Emulation Modules

**Status:** Phase 1 source discovery, informed consent and source-snapshot retrieval implemented. **Not yet a playable Libretro runtime**. Updated 2026-10-11.
**Authority:** `docs/project/INSTRUCTIONS.md`, `docs/project/EXECUTION_CONTRACT.md`, `docs/native/README.md`.
**Execution target:** Samsung Galaxy S9 / Daydream, GLES2, ARM64, controller-first 3DoF seated mode.

## Single APK host boundary (governing)

Emulation is **phone-local**. The ReverieVR APK itself must own the Libretro
host/adapter, local controller routing, in-headset display, audio, local
file picker, save states, memory cards, persistent configuration and recovery.
No desktop application, scrcpy session, remote emulator process, cloud
catalog service, additional installed Android APK, or external optical drive
may be required to run an installed core with compatible local game files.
The source catalog is available offline from its curated embedded metadata;
online verification/download is only for a user-requested new source/core.

A user-approved, license-compliant compatible core may be downloaded later
into app-private module storage under the **one installed ReverieVR APK**.
This must remain optional and its source/binary compatibility, Android
dynamic-loading rules, and performance remain separately gated. Bundling
all cores, PlayStation firmware, disc images, or a desktop runtime is not
part of the baseline. Repositories are discovery/provenance sources, not
mandatory live dependencies for playing already installed content.

Laptop-attached controllers or optical drives are optional supplements.
A PC-side bridge may present read-only game images to a phone-side local
importer or forward events to the phone's own input router, but it must
never host the game's emulation as a prerequisite. Import and cache media
locally when practical; loss of a PC link must not affect other native or
offline games. The emulator handles ISO/BIN-CUE/CHD and virtual save media
according to its specific supported formats; a separate phone-wide virtual
CD drive is not a requirement.

## UX contract

From **Home → Emulator Modules**, the user sees the catalog of source projects, their target system, license and availability status. The list is a **curated allowlist**, not arbitrary code search. On opening, ReverieVR checks the corresponding public GitHub repository's identity, archive status and verified upstream license asynchronously; network failure is shown as unavailable, without silently converting a license failure into approval. Scrolling pages and exiting use the same headset Select/Back input as other Home destinations.

Selecting a catalog entry opens a separate confirmation page showing source repository, license, hardware/BIOS notes and a clear boundary: **SOURCE SNAPSHOT ONLY — not yet a playable emulator core**. YES explicitly requests a download; NO and Back cancel without network transfer. The app does not install or download an executable, ROM, BIOS or firmware automatically at startup, boot, browsing, or on unrelated program updates. All source download work occurs off the VR rendering thread; the UI displays checking, downloading, saved or failed states, with activation/error feedback.

Downloaded source is pinned to an immutable 40-character Git commit resolved from the verified official public repository after confirming its license. Play!'s upstream license is a BSD-style two-condition grant in `License.txt`, but GitHub reports `NOASSERTION` rather than a recognized SPDX expression. Play! is allowed only when that exact source license file's reviewed Git blob SHA matches at the pinned upstream commit; a changed or missing license blocks download pending fresh review. This exception does not permit arbitrary `NOASSERTION` repositories. It arrives from the allowlisted GitHub codeload host, HTTPS only, with redirect blocking and response size cap (96 MiB). The archive is saved privately as inert ZIP data, not extracted or executed, and accompanied by repository identity, license, commit, length and SHA-256 digest; local status revalidates the manifest and archive. A corrupted or interrupted transfer is not marked as saved. No new filesystem permissions are needed; Android INTERNET is already in the manifest.

## Phase 1 curated sources

| Platform | Project | Upstream | License reported by GitHub | Firmware notes |
| --- | --- | --- | --- | --- |
| Game Boy / Game Boy Color | Gambatte | https://github.com/libretro/gambatte-libretro | GPL-2.0 | Generally does not require firmware |
| Game Boy Advance | mGBA | https://github.com/libretro/mgba | MPL-2.0 | Generally supports use without external BIOS |
| NES | Nestopia UE | https://github.com/libretro/nestopia | GPL-2.0 | Generally does not require external BIOS |
| Nintendo DS | melonDS | https://github.com/libretro/melonDS | GPL-3.0 | Some modes and games require user-provided firmware |
| Original PlayStation (PS1) | PCSX ReARMed | https://github.com/libretro/pcsx_rearmed | GPL-2.0 | BIOS may be required; user must provide lawful copy |
| PlayStation 2 (PS2) | Play! | https://github.com/jpd002/Play- | BSD-style two-condition license, `License.txt` blob pinned for verification (GitHub: `NOASSERTION`) | Built-in HLE BIOS; no separate PS2 BIOS needed |

The pinned source snapshot is **not** equivalent to a compatible `arm64-v8a` Android native module. The application never represents source-package retrieval as a functional installed emulator.

## Phase 2 — installable cores and actual runtime

Libretro publishes precompiled ARM64 Android cores at `https://buildbot.libretro.com/nightly/android/latest/arm64-v8a/`. Do not load mutable upstream nightly binaries blindly: introduce a ReverieVR-vetted, versioned distribution manifest with source commit, ABI, uncompressed size, core package SHA-256, signature/provenance evidence, license notice and core-to-repository mapping. Bind the approved package digest to every download, strictly parse the ZIP (one expected ELF `.so`, no traversal or symlinks), check the ELF ABI, and use app-private non-writable finalized storage. Account for Android dynamic-code-loading restrictions and permissions, and implement safe failed-update rollback. Never run downloaded source scripts on device, require broad storage permission, request Bluetooth location, or silently install an APK.

Create the real, generic Libretro host as a **separate runtime** from the current hardwired DOSBox Pure host. An installable core must pass complete Libretro ABI negotiation, video/audio callbacks, controls for Daydream and mapped gamepads, game-file import via Android's SAF into private data, optional BIOS selection via SAF (no bundled firmware), SRAM/savestates, pause/resume, stereo head rendering, display scaling, fullscreen in-scene viewer, hot-unload, and GL/audio context restoration. Keep the `ReverieNativeModuleApiV1` ABI unchanged; use an adapter layer rather than exposing third-party raw callbacks to the VR shell.

When that host exists, replace **download source** with **INSTALL CORE? YES / NO** only for runtime-qualified, verified binaries. On installation success, the same entry becomes PLAY / REMOVE / UPDATE, with progress, disk space and source/license details. A core without an importable user game, required firmware or compatible GLES support must show a precise explanation rather than appear launchable. Installer acceptance requires a real boot of a free/public-domain homebrew game per console, never an unauthorized copyrighted ROM. Source browsing and game import remain separate capabilities.

## Capability, licensing and performance gates

Original PlayStation and PlayStation 2 emulation are target **platforms**, not promises of full-speed compatibility on an S9. Play! uses a built-in high-level BIOS and supports Android/Libretro, but its PS2 compatibility is incomplete; Libretro's Play! OpenGL renderer requires OpenGL 3.0 or GLES 3.2 or newer, and PS2 games can exceed Galaxy S9 thermal and performance budgets. The existing ReverieVR home is GLES2; actual integration must negotiate a separate compatible GL context and validate sustained rendering, input and audio before exposing a PLAY button. Performance must be evaluated per core and title (especially hardware accelerated vs software render), with sustained battery/thermal tests. Each emulator's own license, upstream credits, source disclosure requirements, and any per-core noncommercial clause must be reviewed before shipping executable binaries. GitHub public visibility **does not establish permission to redistribute binaries**. BIOS and game ROM rights are a separate issue; downloading an emulator never grants game-media rights.

## Verification

The pure-Java catalog tests verify supported platform coverage, license gating, immutable source-ref URL construction and rejection of arbitrary URLs/refs. Android/Gradle tests must compile the VR Shell's new category, Host callbacks and source repository. Native host remains unchanged. Device acceptance requires proof of menu navigation, NO avoiding downloads, YES fetching only approved repository content, offline handling, integrity failure recovery, headset controls and nonblocking frame rates. **An actual playable emulator is a separate Phase 2 acceptance gate**.
