# ReverieVR XR Accessibility and Comfort Acceptance Standard

**Authority:** new requirements and test gates. **State:** planned unless an
existing source/service is specifically identified; no accessibility certification
or device acceptance is implied.

## Scope and obligation

Before extended headset use or general release, perform an equal-access and
comfort review for **Stage A**, all Stage B shell panels, Quick Menu, in-VR
physical keyboard, three home rooms, 3D viewer, video player, DOS host and
each first-party native application/game. Accessibility cannot be delegated
to individual game authors: shell defaults and recovery are mandatory, while
modules may provide compatible additional controls.

Reference: [W3C WCAG 2.2](https://www.w3.org/TR/WCAG22/) and
[W3C XR Accessibility User Requirements](https://www.w3.org/TR/xaur/).
WCAG is a *design benchmark*, not a claim of automatic WCAG conformance by
native stereo VR content. Measured text contrast target >=4.5:1 (or >=3:1
for genuinely large text), relevant control/indicator boundaries >=3:1,
and distinguishability must be assessed in headset per eye—not only on
desktop screenshots. Measure after lens distortion, chromatic aberration,
brightness shifts and UI scaling.

## Color-vision accessibility

- Never encode health, warning, friend/enemy, selectable/disabled, battery,
  targeting, cooldown, or important game information **only in color**.
  Pair with stable shapes, names, legends, line styles, hatches, patterns
  and high-luminance separation. Preserve the same meaning across all
  native modules and DOS/media overlays.
- Test normal chromatic vision and representative protanopia/protanomaly
  (red-weak), deuteranopia/deuteranomaly (green-weak),
  tritanopia/tritanomaly (blue-weak), and achromatopsia/grayscale.
  No single "color-blind mode" guarantees all these people can discern
  controls. Offer individually selected palettes and explicit monochrome/
  high-contrast presentation only after they affect real draw paths.
- Validate under the actual Daydream optical path. Simulators are useful
  *screening*, not a substitute for testing with color-vision-deficient people.
  Game team indicators cannot depend on red/green or blue/yellow hue alone.
  Video content itself need not be recolored by default, and opt-in video
  transforms must not corrupt stereo/camera metadata.

## Vision, motor, hearing and cognitive access

- Adjustable text and interaction target sizes with no clipped headings,
  low-vision/large text navigation, optional persistent focus label and
  focus/activation feedback distinct from errors. Do not require
  color interpretation or sub-second aim precision to enter recovery.
- One-handed left/right operation, alternate controller layouts, remappable
  physical inputs, sticky/hold-to-toggle options, adjustable long-press,
  dwell select and optional hands-free/gaze fallback while protecting
  deliberate selection from involuntary motion.
- Captioned dialogue and meaning-bearing sound effects; adjustable
  background/size/position, offscreen speaker/danger direction cues in
  spatial media, and suitable mono audio fallbacks. Do not rely only on
  left/right audio cues for safety or gameplay.
- Predictable focus order, comprehensible state/error descriptions,
  undo/cancel affordances, quiet or simplified UI options, and persistence
  that can be recovered from Stage A without deleting unrelated saves.

## Vestibular, fatigue and photosensitivity

- Seated/low-movement defaults and alternatives to smooth artificial
  locomotion/turning; snap turns, optional vignette, reduced or disabled UI
  animation, independent recenter and adjustable movement speed.
- Avoid uncontrolled flashes and repetitive high-contrast flicker; check
  frequency/area/luminance against accepted seizure-risk guidance. Exposure
  from third-party media cannot be fully controlled; warn when metadata or
  visual inspection suggests a risk rather than falsely certifying playback.
- Explicit opt-in session duration/break reminders, no forced automation,
  immediate pause, simple return to 2D, thermal/battery status and visible
  abort path. Do not portray rear-camera passthrough as a safety substitute.
- Test thermal drift, long-session visibility and focus fatigue on S9 with
  Daydream View; include seated operation with restricted head/neck movement.

## Acceptance protocol

For every shell/module/version record: tested hardware/controller and input
combination; implemented settings path; normal/protan/deutan/tritan/grayscale
color-state coverage; luminance contrast measurements; text scaling; seated
and one-hand workflow; caption/critical cue alternatives; motion/flash policy;
recovery from unusable settings; headset screenshots/video and real device
validation; failures and owners. Include deliberate impairment-oriented
participant feedback where possible, with consent. A code-only test or a
simulator screenshot cannot satisfy device or human-factors acceptance.

An accessibility setting may appear in a shipped UI only after it changes
actual rendering/input/audio and has a reversible fallback. New native games
declare accessibility considerations within their acceptance evidence,
not bespoke proprietary platform controls.
