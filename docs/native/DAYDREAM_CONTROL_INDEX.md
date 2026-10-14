# Daydream control index and game binding vocabulary

**Reference hardware:** original Daydream View controller + Galaxy S9. The controller is tracked in **three rotational degrees of freedom** and reports orientation, angular rate, linear acceleration, touchpad contact coordinates, touchpad physical click, Menu, Home and volume up/down. There is no positional tracking, separate trigger button, dedicated analog stick, pressure sensor or inherent double-tap/long-press switch. The following semantic gestures are inferred in software and must be reference-device tested for false positives.

| Available physical input | Recognized normalized signal | Default shell behavior | DOS / Doom default | Native games / classic default |
|---|---|---|---|---|
| Touchpad click down/up | SELECT | Click current menu target | Mouse left; in bundled Doom Ctrl/fire | Guest joystick 0 / activate |
| Short press/release | DAYDREAM_TAP (after 350ms double-tap window) | Ghost log only | No additional guest action (avoids double fire) | Joystick 13 secondary event |
| Two short taps | DAYDREAM_DOUBLE_TAP | Ghost log only | No default action | Joystick 14 secondary event |
| Click held at least 650ms | DAYDREAM_PRESS_HOLD (held until release) | Original Select remains active | Normal Select remains held; no added action | Joystick 15 held secondary event |
| Capacitive pad touch horizontal | TOUCHPAD_X / TOUCHPAD_REL_X | Cursor/scroll/nav context | Left/right arrows or mouse by profile | Classic joystick X |
| Capacitive pad touch vertical | TOUCHPAD_Y / TOUCHPAD_REL_Y | Cursor/scroll/nav context | Up/down arrows or mouse; bundled Doom up/down movement | Classic joystick Y |
| Deliberate pad swipe release | NAV_LEFT/RIGHT/UP/DOWN | Shell navigation | No guest action beyond existing axis mappings | Shell navigation only |
| App/Menu button | MENU | Universal Quick Menu/recovery | Reserved, never stolen by guest | Reserved |
| Home button | RECENTER | Headset/controller recenter | Reserved | Reserved |
| Volume +/- rocker | VOLUME_UP / VOLUME_DOWN | Platform according to mode | Enter/Escape in DOS profiles | Guest joystick 1/2 |
| Controller orientation quaternion | Aim ray/CONTROLLER_YAW/PITCH_DELTA | Controller pointer, 3D twin | Controller-mouse profile only | Native aim/controls if consumed |
| Accelerometer impulse above ~2.5g | DAYDREAM_SHAKE | Diagnostic ghost event; existing recenter safety unchanged | Unmapped (avoids surprise fire) | Unmapped until explicit game opt-in |
| Head yaw relative movement | HEAD_YAW_DELTA | VR look/aim | Doom turning uses guest mouse X | Native camera/aim according to module |
| Head pitch relative movement | HEAD_PITCH_DELTA | VR look/aim | Selected DOS mouse profile | Native camera/aim according to module |
| Absolute head inclination | HEAD_PITCH_UP_AXIS | Normal VR look | Bundled Doom look up = Up/Forward, look down = Down/Back past ~12° with ~8.5° release; original Doom has no vertical freelook | Unmapped unless module explicitly supports |
| Head extremely upward/downward | HEAD_LOOK_EXTREME_UP/DOWN | Detected and logged; Quick Menu recovery remains deliberate | No automatic guest action | Unmapped |
| Sharp head yaw impulse | HEAD_SHARP_LEFT/RIGHT | Detected and logged, no automatic recenter | No automatic guest action | Unmapped |

The **ghost-grey control feed** is an eye-locked, middle-left, six-line rolling readout. It shows **recognized events**, not raw 60Hz pose packets. Lines have no background, fade over 12 seconds, and are not clickable. It has independent GLES texture/geometry from battery power arcs.

Actions remain distinct from detection. Guest mappings must not silently turn system Menu/Home into fire/exit controls; accidental shake and extreme-look actions are observation-only until a game opts in. The primary SELECT callback is immediate, so no delayed gesture classification can strand menu activation. Gesture meanings and thresholds are documented engineering defaults until reference-controller testing verifies reliability.
