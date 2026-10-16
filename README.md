<p align="center">
  <img src="docs/assets/reverievr-logo.jpg" alt="ReverieVR — A Daydream Revival Project" width="704">
</p>

# ReverieVR

ReverieVR is an independent, offline-first Android VR platform intended to restore useful life to Google Daydream-era phone/headset hardware without depending on Google's discontinued Daydream services.

The initial reference target is a Samsung Galaxy S9 used with a Daydream View headset and controller. The architecture should remain portable enough to support other compatible Android phones and simple 3DoF viewers where practical.

## Public source and third-party licensing

ReverieVR's independently authored software is now distributed under the
**[MIT License](LICENSE)**. Verified original project art is eligible for
**CC BY 4.0** with explicit per-file metadata; **CC0** is reserved for works
whose rights holders deliberately dedicate them. The assembled APK is not
blanket MIT: Cardboard, DOSBox Pure, Daydream art, music, icons, and sample
game content retain their own license conditions.
Review [public distribution and licenses](docs/project/PUBLIC_DISTRIBUTION_AND_LICENSES.md)
and the [approved dependency baseline](docs/dependencies/BASELINE.json)
before redistributing, modifying, monetizing or publishing a derivative APK.
See [the SPDX/REUSE asset records](docs/licensing/ASSET_REGISTRY.json)
for existing copyright, attribution, and unresolved rights information.

**All eleven original tracks are now included in every ReverieVR APK
variant.** The project owner explicitly confirms ownership and commercial
distribution rights for these MP3s (recorded in
[Owner Original Music](docs/licensing/OWNER_ORIGINAL_MUSIC.md)).
They are staged from the [single Git source library](music/source/README.md)
after exact SHA-256 validation, without duplicating audio commits.
The full twelve-track Native Music Library is offline and local.
Starry Cereal, Sky Lobby Drift and Glass Lobby Drift rotate as lobby
music; the existing Breakwater Battery native game plays Mars des Souvenirs.
The Last Bugle Call (Ministry of Intelligence) and The Last Note Hangs
Unresolved (bar project) are mapped to their intended future native module
IDs, without pretending those games already ship. Six other songs are
available in the general library. These songs add **66.3 MB** to the base
APK, intentionally, on every build variant. Commercial use of the songs
as part of ReverieVR is authorized; the music itself is **not** MIT,
Creative Commons, CC0 or public domain.

The offline **Native Music Library** is available from VR Home, with
browse/previous/next/pause controls. It combines the original Starry
Cereal resource and eleven owner-authorized original recordings.
The broader [Jovian-PostPunkRevival Suno profile](https://suno.com/@jovianpostpunkrevival7375)
is not scraped, mirrored or streamed. Other songs from that public
profile are **not** packaged unless separately imported with the
appropriate rights and verified bytes. Neither the player nor the app needs a Suno connection.
Music retains a separate commercial-use license, not MIT or CC BY.

The default current tree excludes Valve's noncommercial CAD-derived controller
mesh; Steam HID hardware remains usable with the independent primitive 3D
fallback. Earlier public commits and releases are not automatically erased by
this forward change.

## VR Home atmospheric environments\n\nThe default offline APK source includes three low-poly environments: a Birch Living Room with recessed, mesh-cut outdoor window views and passing birds; a forest with layered panoramic sky, hills, rabbits/deer and flying birds; and a beach with coastal sky, headland, crabs and gull silhouettes. Home uses original procedurally synthesized environment-specific PCM ambience through the existing Android audio lifecycle, without downloading any sounds on the phone. See [atmosphere integration, CC0 recording candidates and acceptance requirements](docs/project/HOME_ENVIRONMENT_ATMOSPHERE.md). The five linked external CC0 recordings are **not bundled**; Android APK/device validation of the new implementation remains pending.\n\n## Status

**Pre-alpha / foundation bootstrap.**

The first product path is:

1. launch into a conventional 2D touchscreen setup screen;
2. pair/sync and verify a controller source — physical Daydream BLE is the reference path, with paired-phone Daydream-emulator compatibility and ordinary Android gamepads as alternate inputs;
3. transition into the stereoscopic ReverieVR home space;
4. operate normal platform functions from inside the headset using head-look plus the controller.

Stage A is for controller/device readiness, VR entry, updates, diagnostics, and emergency recovery. Media selection, DOS import/library management, 3D models, display preferences, and environment controls belong to Stage B. VR-triggered imports use the Android document picker via a return-to-phone handoff; import stays available after the DOS library is populated.

The VR home is the shell-owned control plane for settings, recovery, media, and later hosted experiences. A first draft local-media path now exists: Android document-picker selection, persisted local read access, flat virtual-screen playback, and mono equirectangular 360° playback. That path is source-complete but remains unaccepted until it builds and is exercised on the Galaxy S9 + Daydream View.

## Project authority

Development is governed by:

- [Project Instructions](docs/project/INSTRUCTIONS.md)
- [Controller Layouts and Unified Rebinding Contract](docs/project/CONTROLLER_LAYOUTS_AND_BINDINGS.md)
- [XR Accessibility and Comfort Acceptance Standard](docs/project/ACCESSIBILITY_AND_COMFORT.md)
- [Repository Execution Contract](docs/project/EXECUTION_CONTRACT.md)
- [APK Construction Order](docs/project/APK_BUILD_ORDER.md)
- [Backlog](docs/project/BACKLOG.md)
- [Execution State](docs/project/EXECUTION_STATE.md)
- [Acceptance Ledger](docs/project/ACCEPTANCE_LEDGER.md)
- [Architecture/Decision Records](docs/records/README.md)
- [Native Game Developer Handbook](docs/native/README.md)
- [Classic Native Games implementation notes](docs/project/CLASSIC_NATIVE_GAMES.md)
- [Breakwater sound-source README/register](docs/project/BREAKWATER_SOUND_SOURCES.md) — linked royalty-free candidates, licensing and SFX intake status
- [AI Agent Entry Point](AGENTS.md)

See [CHANGELOG.md](CHANGELOG.md) for verified project changes.

## Screen privacy, local data, and outbound connections

**Strict no-screen-analysis rule:** ReverieVR must **never** use OCR, text
recognition, visual scanning, computer vision, AI image analysis, object
recognition, screenshot interpretation, or any other programmatic inspection
of a user's phone-screen contents. The optional in-headset Phone Screen is
a **display-only local mirror**, not a scanner or data extractor. Captured
pixels must never be sent to a server, diagnostic bundle, controller peer,
or remote analytics service. This is a permanent project design constraint,
including for future native modules and plugins.

**The user's data remains on the phone.** ReverieVR locally stores
preferences, controller bindings, diagnostics/logs, selected media references,
and imported content/game saves as necessary to operate; this is *local*
storage, not cloud synchronization. We must never describe local storage as
absent. There is no screen-content upload, background screenshot scanning,
advertising telemetry, or unrestricted remote phone access.

**Allowed transfers:** controller input/pose information may be exchanged
with an explicitly connected controller companion or compatible host;
diagnostic packages may leave the phone **only following an explicit,
user-reviewed Send/Submit action**, never because a fault occurred.
Manual ZIP export stays on the handset until the user shares it. Automatic
diagnostic collection/transmission without per-submission user approval is
**not authorized**. The separate optional update-check/download feature
contacts GitHub to request release metadata and APK packages and thus
necessarily exposes ordinary request metadata (such as the app version
in User-Agent and the device's network address); it does **not** upload
media or screen content. Device Bluetooth/ADB/scrcpy traffic initiated by
the user or other apps is separate from ReverieVR's own diagnostics.

**Android screen-capture permission is not streaming to a ReverieVR server.**
Android itself requires MediaProjection consent when an app captures the
display—even if its only destination is a local in-headset texture.
The current Phone Screen is an experimental 3D housing: it does *not*
yet render live Android pixels, so opening it must **not** request that
unnecessary consent. A future functional display bridge must ask through
the genuine Android dialog at capture time and must not attempt to bypass
OS permission or secure-content restrictions. The in-VR **X/Close** control
returns to the normal Home menu and releases any capture resources.

Read the [Phone Screen privacy and lifecycle specification](docs/project/PHONE_SCREEN_PANEL.md)
before changing screen capture or remote-input behavior.

## Core principles

**Offline-first:** core VR operation must not require discontinued Google servers.

**Recoverable boot:** touchscreen setup exists outside VR so controller pairing, recovery, diagnostics, and re-entry remain possible even when the VR control path is broken.

**Performance-first:** steady frame pacing, low latency, and thermal sustainability outrank graphical fidelity. ReverieVR intentionally targets a restrained retro visual budget rather than trying to turn an older phone into a modern desktop VR GPU.

**Capability honesty:** unsupported behavior is reported as unsupported; no fake-success stubs, no-op compatibility layers, or hidden server dependencies.

**Shell-owned platform UX:** the Stage B ReverieVR shell remains the platform control plane around the home, media player, and hosted games/modules. Global status, recovery, settings, and navigation behavior belong to the shell rather than being reimplemented independently by every experience.

**Configurable quality of life:** convenience behavior that automatically moves, hides, reveals, recenters, or changes persistent UI must be locally configurable. A conservative predictable behavior remains available when automation is disabled.

## Legal and provenance boundary

Do not commit or redistribute proprietary Google binaries, applications, SDK payloads, firmware, signing material, credentials, private keys, copyrighted media/assets, or other third-party material without redistribution rights. Reverse-engineering notes, independently written compatibility code, public specifications, hashes, metadata, and user-supplied local test artifacts must remain clearly separated according to provenance and license.
