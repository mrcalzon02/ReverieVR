# ReverieVR

ReverieVR is an independent, offline-first Android VR platform intended to restore useful life to Google Daydream-era phone/headset hardware without depending on Google's discontinued Daydream services.

The initial reference target is a Samsung Galaxy S9 used with a Daydream View headset and controller. The architecture should remain portable enough to support other compatible Android phones and simple 3DoF viewers where practical.

## Status

**Pre-alpha / foundation bootstrap.**

The first product path is:

1. launch into a conventional 2D touchscreen setup screen;
2. pair/sync and verify the controller;
3. transition into the stereoscopic ReverieVR home space;
4. operate normal platform functions from inside the headset using head-look plus the controller.

The VR home then provides full local settings control, performance diagnostics, module launching, and eventually video playback and small VR experiences.

## Project authority

Development is governed by:

- [Project Instructions](docs/project/INSTRUCTIONS.md)
- [Repository Execution Contract](docs/project/EXECUTION_CONTRACT.md)
- [APK Construction Order](docs/project/APK_BUILD_ORDER.md)
- [Backlog](docs/project/BACKLOG.md)
- [Execution State](docs/project/EXECUTION_STATE.md)
- [Acceptance Ledger](docs/project/ACCEPTANCE_LEDGER.md)
- [Architecture/Decision Records](docs/records/README.md)
- [AI Agent Entry Point](AGENTS.md)

See [CHANGELOG.md](CHANGELOG.md) for verified project changes.

## Core principles

**Offline-first:** core VR operation must not require discontinued Google servers.

**Recoverable boot:** touchscreen setup exists outside VR so controller pairing, recovery, diagnostics, and re-entry remain possible even when the VR control path is broken.

**Performance-first:** steady frame pacing, low latency, and thermal sustainability outrank graphical fidelity. ReverieVR intentionally targets a restrained retro visual budget rather than trying to turn an older phone into a modern desktop VR GPU.

**Capability honesty:** unsupported behavior is reported as unsupported; no fake-success stubs, no-op compatibility layers, or hidden server dependencies.

**Shell-owned platform UX:** the Stage B ReverieVR shell remains the platform control plane around the home, media player, and hosted games/modules. Global status, recovery, settings, and navigation behavior belong to the shell rather than being reimplemented independently by every experience.

**Configurable quality of life:** convenience behavior that automatically moves, hides, reveals, recenters, or changes persistent UI must be locally configurable. A conservative predictable behavior remains available when automation is disabled.

## Legal and provenance boundary

Do not commit or redistribute proprietary Google binaries, applications, SDK payloads, firmware, signing material, credentials, private keys, copyrighted media/assets, or other third-party material without redistribution rights. Reverse-engineering notes, independently written compatibility code, public specifications, hashes, metadata, and user-supplied local test artifacts must remain clearly separated according to provenance and license.
