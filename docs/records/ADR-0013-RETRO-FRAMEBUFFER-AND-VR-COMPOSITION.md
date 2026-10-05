# ADR-0013 — Retro Source Resolution and VR Composition

**Status:** Accepted as implementation baseline  
**Date:** 2026-10-05

## Context

ReverieVR must display authentic low-resolution guest software inside a modern
stereoscopic mobile VR compositor.

These are two different resolution domains:

1. the **guest/source framebuffer** produced by DOS or another retro runtime;
2. the **per-eye VR render target** selected by Cardboard/Android for the actual
   phone/viewer optical path.

Conflating them causes incorrect pixel aspect, distorted retro output, or
unnecessary VR rendering cost.

## Decision

### Guest/source modes are first-class

The display mapper explicitly recognizes important retro source modes including:

- 320x200 VGA / Mode 13h;
- 320x240 square-pixel 4:3;
- 640x350 EGA;
- 640x400 VGA;
- 640x480 VGA;
- 720x400 DOS/VGA text;
- 800x600;
- 1024x768;
- 1280x1024;
- 160x144 Game Boy;
- 240x160 Game Boy Advance.

Unknown modes remain valid and default to their native pixel aspect.

### Pixel dimensions and intended display aspect are separate

A framebuffer's numeric width/height do not always describe the intended
physical display shape.

Examples:

- 320x200 has a 1.6:1 raw pixel grid but classic PC presentation commonly
  expects a 4:3 display shape;
- 640x400 likewise maps to an intended 4:3 display shape;
- 720x400 text output is treated as a 4:3-class display mode;
- 320x240 and 640x480 are square-pixel 4:3 modes;
- 1280x1024 is a square-pixel 5:4 source mode.

ReverieVR therefore stores:

- source width;
- source height;
- intended display aspect;
- whether the source is square-pixel.

The virtual screen geometry uses the intended display aspect, not blindly
`width / height`.

### Integer scaling is a source-texture concern

When useful, retro content should support nearest-neighbor/integer scaling
inside an intermediate texture or virtual-screen surface.

The shell UI/HUD remains independent and can stay high-resolution/readable even
when the guest framebuffer is extremely low resolution.

This permits:

- pixel-stable guest graphics;
- readable native VR menus/overlays;
- no requirement to render the entire VR scene at the guest game's resolution.

### VR eye buffers are runtime/viewer-driven

ReverieVR does **not** hard-code 640x720, 800x900, 1280x1024 or another fixed
per-eye target merely because those shapes have been used by other VR systems.

It also does not assume a universal 1.4x lens-distortion supersampling factor.

Cardboard owns the eye viewport, projection, distortion mesh and actual
phone/viewer geometry. ReverieVR may later expose measured render-scale/quality
controls, but those operate in the VR composition domain and are validated on
the Galaxy S9 reference device.

The pipeline is:

`guest framebuffer -> aspect/pixel correction -> texture -> virtual screen or
module surface -> Cardboard per-eye composition/distortion`

## Consequences

Doom's 320x200 output can look like the 4:3 display it expected while the VR
shell remains rendered at an appropriate headset resolution.

Text-heavy modes can receive their own scaling/filter choices without making
the entire world-space UI pixelated.

Future CRT/shader effects can be optional post-processing on the guest surface,
not mandatory distortion baked into the VR eye buffer.

Performance tuning remains evidence-based on the reference phone rather than
derived from a generic headset supersampling rule.
