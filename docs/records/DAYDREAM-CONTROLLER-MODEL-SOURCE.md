# Daydream controller model source

Status: vendored runtime visual with preserved upstream provenance.

## Original source

Model: **Daydream Vr Controller**  
Author: **rje**  
Sketchfab model ID: `c1944c64e06544babc90e9d0aa953551`  
Source: https://sketchfab.com/3d-models/daydream-vr-controller-c1944c64e06544babc90e9d0aa953551

The original Sketchfab listing describes the model as downloadable, low-poly,
and licensed under Creative Commons Attribution (CC BY).

## Vendored runtime derivative

ReverieVR now ships the low-poly OBJ derivative from:

https://github.com/TechnoBuddhist/VR-Controller-Daydream

The derivative's README states that it was created from rje's Sketchfab model,
splits the controller controls into separate objects, retextures the model, and
permits use when the README is retained and rje is attributed. The derivative
repository also carries a GPL-3.0 repository license. ReverieVR therefore keeps
both the upstream README and the upstream GPL-3.0 license beside the model
instead of silently discarding either provenance statement.

Vendored files live under:

`app/src/main/assets/models/daydream/`

The runtime geometry is `vr_controller_daydream.obj`. The current lightweight
GLES renderer uses the authored object/material separation for the body,
touchpad, Home/App controls, and volume controls. The referenced upstream
texture is not required by ReverieVR's current material-color renderer.

## Runtime contract

The physical Daydream controller remains a 3DoF orientation-tracked device. The
visual model therefore uses the real controller quaternion while its virtual
hand origin remains a bounded shell-owned anchor. The anchor follows the
headset's small bounded inertial body offset so the controller does not appear
to be left behind when the user leans, but this is not controller 6DoF
position tracking.

The visible model and pointer ray share one anchor/orientation transform. The
ray emitter is offset toward the front of the rendered controller instead of
originating at the model center.

No positional/6DoF controller tracking may be inferred or advertised.

## Failure behavior

The vendored Daydream mesh is the normal controller visual. The previous
procedural controller geometry is retained only as a runtime fallback if the
asset cannot be loaded or parsed. A visual-asset failure must never disable
controller input, raycasting, or recovery controls.

## Attribution files

The packaged asset directory retains:

- `UPSTREAM-README.md`;
- `UPSTREAM-LICENSE-GPL-3.0.txt`;
- `THIRD_PARTY.md`;
- the OBJ and MTL source files.

Any future replacement with the original Sketchfab download must preserve rje
attribution and re-evaluate the derivative-specific GPL/README obligations
rather than deleting notices opportunistically.
