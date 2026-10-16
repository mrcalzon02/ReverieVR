# Controller Layouts and Unified Rebinding Contract

**Status:** requirements + incremental source progress; not a claim of
complete working per-controller layouts or user-facing arbitrary remapping.
**Authority:** ReverieVR Stage B and native-host input ownership.

## Device identity before configuration

ReverieVR chooses a single primary controller for platform interaction.
The user must be able to select the actual attached controller in VR,
not just a USB/BT technology category. Android's runtime device ID must
never be used for long-term identity. Persist an Android HID descriptor,
or a stronger independently verified pairing identity when supported.
If two devices expose indistinguishable descriptors, do not pretend to
identify them; display the ambiguity and require a safe correction path.

**Current implementation (2026-10-11 source):** generic Android gamepads
with unique descriptors are individually selectable; the existing source
panel cycles them. Explicit removal returns to Auto for recovery.
Daydream BLE, original Steam Controller and scrcpy have existing
source-family arbitration; per-instance enumeration for multi-device
composite BLE/HID families still needs driver-specific evidence.

Auto means a documented deterministic fallback, **not** "all controllers
simultaneously operate the same menu". A connected secondary may remain
available for isolated guest multiplayer only under explicit ownership.
Source switching must release held virtual keys/buttons/axes and preserve
safe Stage A recovery.

## Visual physical-layout families

All graphics use fixed-seed, cached low-poly silhouettes, orthographic
control labels and a small palette in ReverieVR's early-PlayStation style.
No high-resolution texture requirement or copyrighted branded model
payload. Faceplates are selectable/manual when vendor metadata is
ambiguous; do not infer functional button identity from cosmetics.

| Layout | Physical labels and arrangement | Android report validation |
|---|---|---|
| Generic | A/B/X/Y or 1/2/3/4, D-pad, L/R shoulders, L/R triggers, two sticks, select/start/system as exposed | Never assume uncommon keys/axes exist |
| Xbox 360 style | A (lower), B (right), X (left), Y (upper), LB/RB, LT/RT, LS/RS, Back/Start/Guide | Verify vendor/product, trigger rest position, wired/2.4GHz transport and Android keycode behavior |
| PlayStation DualShock style | Cross (lower), Circle (right), Square (left), Triangle (upper), L1/R1, L2/R2, L3/R3, Share/Options/PS or older Select/Start | Verify actual DualShock generation and Android driver mapping; do not equate Cross with keyboard X or assume Circle is Back |
| Daydream | Touchpad direction/press, App/Menu, Home, volume rocker, tracked pose as actually reported | Physical Daydream events and recovery stay separate |
| Steam Controller 2015 | Two touchpads, thumbstick, face buttons, triggers, shoulders, rear paddles and optional gyro | Distinguish actual HID reports from Steam keyboard/mouse emulation and from Steam platform software |

The first three layout families are required minimum renderable diagrams.
Never treat controller recognition as proof every listed physical control
actually produces a usable Android event. Unsupported parts are visually
marked, accompanied by a readable label, and cannot be rebound as if
they emitted data.

## Three-stage input pipeline

1. **Physical device and raw controls:** a selected provider, stable device
   identity and raw key/axis/pose reporting. Android gamepad A/B/X/Y,
   shoulders/triggers, stick clicks, D-pad and left/right trigger axes have
   started receiving independent `BindingInput` values. Different Android
   drivers still require a calibration/report capture pass.
2. **ReverieVR logical actions:** Select/Confirm, Back/Cancel, Menu/Recovery,
   Recenter, Navigate, Point, Scroll, Move, Turn, Look, Fire, Alt Fire,
   Reload, Interact, Inventory, Pause, and UI typing/editing. The action
   vocabulary must be versioned; shell-critical actions cannot be confused
   with guest gameplay. Action prompts show **both** the logical action
   and chosen controller's physical glyph/label.
3. **Context-specific outputs:** implemented destination types include
   virtual keyboard, mouse buttons and movement, joystick buttons 0–15,
   joystick X/Y, plus existing shell actions. Expand output kinds only after
   host/native ABI can actually consume them. A claim that "any button can
   output anything" is restricted to **implemented, authorized output
   types**; it does not mean injected Android OS actions or inaccessible
   physical sensors.

Resolution order must be explicit: OS/system safety > persistent shell
emergency recovery > active modal capture > user controller overrides >
specific native/DOS module overrides > layout defaults. The actual
runtime must resolve effective bindings once per input event, not emit
both the old and new mapping. Chords and long-holds require deliberate,
per-control support and clear progress indication, with configurable
one-hand alternatives. Provide neutral-axis calibration, deadzones,
inversion and sensitivity with rest-pose detection; never fake triggers
from a button model that Android does not report.

## Native games and host contracts

A native game declares **actions it uses**, required vs optional
inputs, default action bindings and rebind-safe alternative actions.
One shared host resolver produces virtual buttons/axes and pointer
state; games do not inspect manufacturer/vendor ID or hardcode keys.
Game-specific defaults overlay (do not destructively replace) user
controller accessibility preferences. Per-game overrides persist with
a versioned game ID; deleting a game must not delete global profiles.

The current native-standard profile sends Daydream Select to joystick
0, rocker inputs to 1/2, and selected Android gamepad physical buttons
to 3–12 plus left-stick X/Y; these are defaults, not universal action
meaning. A game that needs named Fire/Reload/Interact must define
semantics in a future versioned action manifest, rather than assuming
joystick 3 universally means Reload. Existing ABI v1 has no such action
manifest yet: **planned**, not implemented.

## In-headset binding editor

Required layout: in-world physical controller schematic with focusable
button/axis targets, an adjacent current-command list, and an observable
live input monitor. Choose device → choose global/native/DOS/game scope →
choose physical button → choose output type → choose output command →
preview/test → commit or Cancel. All keyboard, physical gamepad,
Daydream and gaze navigation paths must offer equivalent editing
capabilities where hardware permits.

Support duplicate-binding conflict explanations, clear/unbind, swap,
reset one control/reset profile, import/export only after validated
format/bounds, and explicit rescue path. Never let a malformed mapping,
missing primary device, motor impairment or single wrong press remove
the final ability to open Menu, return to Stage A or reset bindings.
No inert controls or saved-only toggles: every visible option must
perform actual routed behavior and provide visible/audio success or
failure feedback.

The editor and controller diagrams must be color-independent: labels,
shapes, focus rails, explicit disabled markings and text scale/contrast
survive protan/deutan/tritan/grayscale conditions. See
[Accessibility and Comfort](ACCESSIBILITY_AND_COMFORT.md).

## Acceptance matrix

- Generic USB pad + generic Bluetooth pad together: select either as
  primary; second cannot move shell; swap releases held input.
- Xbox 360 wired/wireless controller: capture every reported key/axis;
  verify triggers, D-pad, sticks, Start/Back and consistent logical glyphs.
- Real DualShock-generation controller: capture Cross/Circle/Square/
  Triangle; check Options/Share/PS, L2/R2 and stick-click translation.
- Daydream + Android gamepad; original Steam HID + gamepad; scrcpy UHID
  + physical controller; USB hub with multiple controllers: verify exact
  ownership, hot-unplug and the fallback path without Location gating.
- Each first-party native game: default controls, bound alternative,
  rebinding in quick menu, input-owner transitions, correct actions
  after resume/relaunch and no hidden game-specific Android code.
- One-hand and limited-motion access: complete controller navigation,
  recover missing input, remap without timing or precise aim.
- No physical multi-controller/S9 and no accessibility-human-factors
  acceptance may be marked complete on source/JVM alone.


## DOS viewpoint and menu focus correction (2026-10-15)

The standard DOS and headset-view mouse profiles keep head yaw/pitch deltas
as **continuous guest mouse movement**, independent of touchpad click.
The Daydream touchpad's four holdable directions remain DOS guest arrow
keys (including Up/Down in game menus), also independent of click.
Passing the gaze cursor over the in-room DOS shell controls may intercept
Select, but must not suppress movement axes or arrow-key holds.
Opening the actual Quick Menu or DOS binding overlay still releases and
blocks guest input to prevent accidental gameplay. Home is still the
universal Quick Menu, the touchpad click is left mouse and the physical
App/minus button is right mouse where the active DOS profile allows it.
The keyboard/menu behavior of a particular DOS game still depends on
that game's own input support; Galaxy S9 verification remains required.


### DOS overlay menu pad navigation (2026-10-15)

When the DOS Quick Menu or DOS binding panels are actually open, the
Daydream touchpad's upper and lower directional zones navigate their
buttons without requiring click-and-drag or a virtual mouse. A held
direction repeats after 350 ms and then every 170 ms; center release
re-arms navigation. Touchpad click still activates the selected menu
command. Leaving the overlay restores the unchanged DOS game bindings,
including held guest arrow keys. Home remains shell-owned.

## DOS library reach and touch locomotion (2026-10-15)

The DOS library is a seated VR destination, not a fixed 3-meter menu:
its main panel sits 2.05 m ahead with matching ray collision and stereo
geometry. Windowed DOS options use a closer right-side rail with the same
actual hit and rendered planes. The library supports bounded headset-view
relative motion (0.65 m lateral / 0.45 m forward-back) from touch-and-hold
without click; release/rearm prevents stale touch between modes. Vertical
swipes still scroll the DOS list. DOS gameplay retains holdable guest arrow
keys instead of shell locomotion. These are source changes pending physical
Galaxy S9 acceptance, not hardware proof.

## DOS floor keyboard activation (2026-10-15 source)

The desk's 51 hit-tested keycaps remain visibly drawn in the DOS library
and windowed DOS gameplay. Library keys search installed title names and
jump the scrollable list to the first match; CLEAR SEARCH removes the query.
Windowed DOS guest keys instead send real virtual-key edges, including
Enter, Tab, Backspace, Delete, arrows, printable characters and Escape;
SHIFT/CAPS/SYMS modify keyboard labels and emitted guest key modifiers.
This uses the existing guest input bus and intercepts Select only over an
actual desk-key or DOS rail target; ordinary touch-only held DOS arrows and
head-look mouse movement continue without any pressed mouse requirement.
Fullscreen DOS intentionally excludes room geometry and floor keys;
Home/Quick Menu retains recovery and modal guest-input suspension.
Handset stereo/input and actual guest typing remain acceptance gates.

## DOS no-click regression (2026-10-15 source)

The `VrInputRouterDosNoClickTest` exercises the actual Daydream packet
routing through a real DOS FPS binding engine and virtual input bus:
nonclicked touch holds guest Up/Down and releases on lift; head yaw/pitch
continue driving mouse deltas without mouse-button activation. This is a
source regression contract; the user's physical report still requires a
new signed APK on the Galaxy S9 plus live INPUT_NORMALIZED/BINDING_OUTPUT
trace if the handset differs from the tested source path.

### DOS library pad focus without mouse click (2026-10-15)

Stationary 180-ms holds in the upper/lower Daydream pad zones move
DOS-library selection focus directly, repeating at 350/170 ms. A swipe
moving 18 or more raw pad units becomes scrolling and never fires the
held-zone navigation for the remainder of that touch. Selecting a focused
game uses an ordinary touchpad click; no click is needed merely to
move between rows. Focus automatically scrolls when moving past the
first/last visible game, and normal gaze selection returns after a swipe
or menu selection. This routing is limited to the DOS library: running
DOS games still receive independent held guest arrow/WASD keys and
head-driven mouse axes, while opened DOS settings retain their own
existing directional navigation. The phone/headset usability acceptance
remains open until tested on the Galaxy S9.
