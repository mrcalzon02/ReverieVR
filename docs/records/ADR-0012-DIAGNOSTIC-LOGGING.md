# ADR-0012 — Standard and Development Diagnostic Logging

**Status:** Accepted as implementation baseline  
**Date:** 2026-10-05

## Context

ReverieVR is being developed on older mobile VR hardware where failures may be
specific to Android lifecycle, Bluetooth/controller state, Cardboard rendering,
native emulation, file import, thermal behavior, or interactions between those
systems.

A single always-verbose logging mode would waste storage and distort normal
performance. A single quiet log would be insufficient when reproducing subtle
controller, renderer, or DOS-host failures.

The user also wants eventual automated error submission, potentially through
GitHub, but does not want that problem to block current development.

## Decision

ReverieVR has two logging modes.

### Standard

Standard is the default.

It records only events that are useful in normal operation:

- application/process milestones;
- Stage A / Stage B transitions;
- module/import/load/unload milestones;
- controller connection/disconnection state;
- updater/install outcomes;
- recovery actions;
- warnings, incidents and errors;
- uncaught fatal exceptions.

Standard logging is intended to remain enabled during ordinary use.

### Development

Development is explicit and diagnostic-heavy.

In addition to all Standard records, it may record:

- raw controller snapshots;
- Android gamepad keys and axes;
- standard HID keyboard/mouse events;
- normalized Reverie input signals;
- binding transforms and virtual outputs;
- head yaw/pitch deltas;
- every VR frame's diagnostic head/render/HUD state;
- DOS/libretro frontend callbacks and frame/audio timing once that layer is
  active;
- resolution/mode changes;
- module and renderer lifecycle details.

Development logging is allowed to block when its bounded in-memory queue fills.
It prioritizes complete diagnostic capture over frame pacing.

Therefore:

**A Development-logging run is not valid performance, frame-pacing, battery, or
thermal acceptance evidence.**

### Storage and rotation

Logs remain in app-private storage.

Current rotation targets:

- Standard: 1 MiB active file + up to 4 rotated archives;
- Development: 16 MiB active file + up to 4 rotated archives.

The queue is bounded so high-volume logging cannot consume unlimited RAM.

### Crash capture

ReverieVR installs an uncaught-exception handler that writes a fatal record
before delegating to Android's previous/default crash handler.

This captures the failure locally. It does not suppress Android crash handling
or pretend the application recovered.

### Submission/export policy

There is currently **no automatic upload**.

Stage A exposes an explicit manual diagnostic export. The user chooses the
destination through Android's document UI and ReverieVR writes a ZIP containing:

- a small environment/build manifest;
- Standard logs;
- Development logs and rotated archives if present.

Development logs may include device names, local filenames, imported module
names, controller/input state and timing. The UI warns the user to review the
bundle before sharing it.

Secure GitHub issue submission is now governed by
`ADR-0017-SECURE-DIAGNOSTIC-ISSUE-INTAKE.md`.

The submission path must:

- be explicit opt-in;
- never embed a personal GitHub token, GitHub App private key, upload-broker
  credential, or repository secret in the APK;
- show what data is being submitted;
- support redaction;
- preserve the manual-export path;
- fail safely without losing the local diagnostic bundle;
- keep raw diagnostic bundles out of the public issue body and public
  repository unless the user explicitly chooses otherwise.

## Consequences

Normal users get low-noise local history.

Development sessions can be extremely verbose without changing the semantics of
the normal runtime.

Crash evidence exists even before any network reporting system exists.

The project can later add GitHub issue submission without coupling diagnostic
capture to an external service.
