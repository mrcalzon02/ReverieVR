# ADR-0011 — Embedded DOS Runtime and Virtual Input Routing

**Status:** Accepted as implementation baseline  
**Date:** 2026-10-05

## Context

ReverieVR is intended to host simple retro games locally rather than depend on
dead Daydream services. DOS games are a particularly good fit for the
performance-first hardware target, but a useful DOS layer cannot be hard-coded
to one title.

The same Daydream controller input may need to become:

- a keyboard key in one game;
- a mouse button in another;
- a joystick button in a third.

Head orientation may become relative mouse movement for a first-person title,
while touchpad or gamepad axes may become an absolute mouse cursor for a
point-and-click title.

The user also wants ordinary Bluetooth/USB keyboards and mice, including
devices presented through scrcpy, to work as direct guest input.

## Decision

### DOS runtime

DOSBox Pure is the primary embedded DOS runtime target.

The first pinned baseline is:

- tag: `1.0-preview6`;
- commit: `a4a0bab7f8931433588f2fcad9045c85b277373d`;
- license: GPLv2 or later.

The project owner has explicitly accepted GPLv2/GPLv2+ as viable.

DOSBox Pure remains an identifiable third-party component. ReverieVR will host
the libretro core rather than launch a separate Android DOSBox application and
attempt global Android input injection.

### Generic DOS modules

A DOS module is content + launch/runtime metadata + a binding profile.

ReverieVR accepts DOSBox Pure's advertised content families rather than a
Doom-specific package format:

- ZIP / DOSZ;
- EXE / COM / BAT;
- ISO / CHD / CUE;
- IMG / IMA / VHD / JRC;
- M3U / M3U8;
- CONF.

Imported files are copied into app-private storage so the native core receives
a stable full filesystem path.

A single executable can only be sufficient when the program is actually
self-contained. Multi-file games must bring their dependent data, preferably as
ZIP/DOSZ; directory-tree import is a follow-on requirement.

### Input layers

ReverieVR keeps three separate concepts:

1. **raw source input**
   - Daydream/phone-controller state;
   - Android gamepad axes/buttons;
   - head orientation deltas;
   - standard Android keyboard/mouse events.

2. **configurable profile bindings**
   - source signal;
   - transform/deadzone/threshold/sensitivity;
   - target virtual keyboard/mouse/joystick output.

3. **virtual guest devices**
   - held keyboard keys;
   - mouse buttons;
   - relative mouse deltas;
   - absolute pointer position;
   - wheel motion;
   - joystick axes/buttons.

The virtual-device state is what the DOS/libretro frontend will poll.

### Standard keyboard and mouse

Standard HID is passthrough by default rather than being forced through a
Daydream-style controller mapping.

Bluetooth/USB keyboards and mice therefore target the virtual guest keyboard
and mouse directly.

scrcpy input is supported when Android exposes it as key/mouse events. UHID is
the preferred scrcpy mode for DOS because it presents physical HID semantics
and relative mouse behavior.

### Overlay and recovery

Guest content must never be allowed to consume every possible host control.

The hosted-game overlay will reserve at least one host-owned escape path that
cannot be rebound solely into guest output. Opening that overlay pauses or
captures guest input, exposes binding/profile controls, and can terminate the
DOS session back to ReverieVR Home.

Profile changes, controller disconnect, Activity pause and input-device removal
must release all virtual held inputs to prevent stuck keys/buttons.

## Consequences

- Doom can use head yaw as relative mouse movement while another module uses
  touchpad position as an absolute mouse cursor.
- A real keyboard/mouse can coexist with Daydream-derived virtual inputs.
- The binding layer is emulator-independent enough to support later hosted
  runtimes.
- DOSBox Pure source/license/build provenance becomes part of the release
  acceptance gate.
- Game-content legality remains separate from emulator licensing. ReverieVR
  will not bundle commercial game data merely because it can execute it.
