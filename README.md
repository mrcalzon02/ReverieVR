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
See [the SPDX/REUSE and asset registry](docs/licensing/ASSET_REGISTRY.json)
and each Release's `DIST-LICENSE-MANIFEST.json` for actual files,
copyright/attribution records, and unresolved rights.

An additional [eleven-track source music inventory](music/source/README.md)
records new owner-supplied MP3s and intended uses for the bar project,
Beachhead-style game, Ministry of Intelligence, and alternate lobby tracks.
**Their binary upload to GitHub is pending** and they are not yet packaged
or playable in the current APK. Source-only candidates are not automatically
licensed as MIT, CC BY or CC0.

The offline **Native Music Library** is available from VR Home, with
browse/previous/next/pause controls. The catalog is sourced from the owner's
[Jovian-PostPunkRevival Suno profile](https://suno.com/@jovianpostpunkrevival7375)
and accepts only owner-authorized, official paid-plan audio downloads
(see [Suno music provenance](docs/licensing/SUNO_AUDIO_PROVENANCE.md)).
Currently Starry Cereal is packaged; the other public-profile songs are **not
yet in the APK**. Neither the player nor the app needs a Suno connection.
Music retains a separate commercial-use license, not MIT or CC BY.

The default current tree excludes Valve's noncommercial CAD-derived controller
mesh; Steam HID hardware remains usable with the independent primitive 3D
fallback. Earlier public commits and releases are not automatically erased by
this forward change.

## Status

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
