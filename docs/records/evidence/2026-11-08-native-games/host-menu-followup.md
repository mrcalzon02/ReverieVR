# Native host menu/lifecycle follow-up

Read-only follow-up, 2026-11-08. Inspected checkout `8a5196aa97a73cdae1393e27529bd51914fc1b0f`; the two requested Java files are unchanged from audit baseline `12d4018c90e56a3767b8ffc58a2e5394440caec1`. Original classic report, JSON and test evidence remain unchanged. No new broad suite, production edits or workflow runs were performed. Findings below distinguish source-traced behavior from previously executed module lifecycle failures.

## Direct answers

**Quick Menu does not stop native simulation updates.** `VrShellRenderer.java:2090–2137` explicitly calls `nativeModuleRuntime.update(POINTER_NONE, null, null)` while the menu is visible. `NativeModuleRuntime.java:383–407` still computes elapsed time and reads the existing joystick axes/buttons; `447–468` forwards them to JNI. `reverie_native_host.cpp:1105–1153` invokes the module's `update` without a paused-state guard. `POINTER_NONE` clears queued sounds afterward (`1154–1161`); it does not freeze time or zero button/axis state.

For the classics this continues actual play: Pong/Snake/Chutes call `Step` regardless of pointer availability (`classic_games_module.cpp:1730–1749`); Breakout does likewise (`1363–1372`); board CPU results can be polled/started (`1638–1654`). Thus an active Snake can collide and a Pong/Breakout ball can continue while the user reads settings. This is **source-confirmed behavior**, not a newly executed Android menu test. It is a material menu/playability gap if Quick Menu is expected to provide a safe pause.

**Home destroys the active module.** The complete source route is:

| Step | Evidence |
|---|---|
| Quick Menu Home requests native stop after releasing GLES resources | `VrShellRenderer.java:5760–5787` |
| Activity immediately invokes runtime stop and ends the hosted input profile | `VrActivity.java:2078–2091` |
| Runtime clears the handle and calls native Pause then native Destroy | `NativeModuleRuntime.java:527–542` |
| JNI destroy calls module destroy and closes the library | `reverie_native_host.cpp:954–959`, `614–631` |
| Home's “RESUME NATIVE” calls ordinary native playback/start | `VrShellRenderer.java:7815–7840`, `7891–7904`; `VrActivity.java:2029–2045` |
| Start always stops any old session and creates a new instance | `NativeModuleRuntime.java:180–187`, `238–287` |

The earlier actual-module probes already established that all eight classics reset on this Destroy/Create boundary and never read/write a save. Consequently, **Home → RESUME NATIVE loses the current classic match**, not merely a hypothetical future process-kill case. The shell's label checks recorded module identity/availability, not whether a resumable match exists (`VrShellRenderer.java:7758–7763`). Android Back outside Quick Menu also follows native stop → Home (`2191–2204`); Close/Back inside the menu only dismisses it (`5647–5649`, `5790–5797`, `6542–6551`).

## Additional host-input defect exposed by the trace

**A held Daydream button or touchpad axis can remain latched when its release occurs inside Quick Menu.** This is a concrete static data-flow finding; it has not received a device replay in this bounded follow-up.

1. The native profile binds SELECT to joystick button 0 and the classic profile maps touchpad X/Y to joystick axes (`BuiltInBindingProfiles.java:560–568`, `638–649`). Those values persist in `VirtualInputBus` until explicitly replaced or released (`VirtualInputBus.java:60–67`, `105–112`, `218–236`).
2. On ordinary menu entry, `VrActivity.onInputAction(MENU)` resets only `StandardHidInputRouter` (`1105–1112`). That reset releases tracked keyboard keys/mouse buttons, not the native joystick binding state (`StandardHidInputRouter.java:213–224`). Renderer menu entry calls the full guest-input release callback only for DOS, not native modules (`VrShellRenderer.java:1155–1179`).
3. While Quick Menu is open, `VrActivity.onBindingDigital` and `onBindingAxis` return without applying events (`1178–1205`), including digital releases and zero-axis updates. `isHostedInputSuppressed` is true for the open menu (`VrShellRenderer.java:1641–1649`).
4. The Daydream router still updates its previous physical click/touch state (`VrInputRouter.java:306–311`), emits digital changes only on edges (`599–611`), and emits zero touch axes only on the touching→not-touching transition (`251–271`). A release delivered while suppressed can therefore be discarded permanently until a new press/release or touch/lift sequence.
5. Menu Close changes UI flags only; it does not release/re-arm native bindings (`VrShellRenderer.java:5790–5797`). Meanwhile the menu's native Update continues reading the latched bus values as described above.

A minimal regression to add during repair is: start native game, hold Select or a nonzero pad direction, open Menu while held, release while Menu remains open, close Menu without re-pressing; assert zero guest buttons/axes throughout the menu and on return. Repeat with headset-recovery menu entry: that directly calls `toggleOrientationMenu` (`VrShellRenderer.java:1973–1987`) and bypasses even the ordinary Activity MENU keyboard/mouse reset. The repair should centralize guest release on every menu-entry path, stop simulation for intended pause semantics, and re-arm cleanly at return; releasing only the pointer is insufficient.

## Suspend/resume and reconnect protections that do exist

- **Android Activity pause** is different from Quick Menu: `VrActivity.java:314–343` calls binding `releaseAll`, resets HID, calls native Pause and pauses `CardboardView`. On Activity resume it resumes the native runtime (`273–306`), which resets `lastUpdateNanos` before the native Resume callback (`NativeModuleRuntime.java:344–357`). This avoids a large catch-up delta on the intended lifecycle path. Device scheduling/process-death recovery was not re-tested here.
- **Controller disconnect/source switch** has explicit input release and edge reset (`VrActivity.java:925–938`). `ControllerManager.java:100`, `163`, `214–216` arms `ControllerResumeGate`; `244–254` suppresses packets until accepted. The gate requires all click/touch/Menu/Home/volume controls to be neutral (`ControllerResumeGate.java:14–23`). That protection is wired to provider connection/source changes; the reviewed menu open/close paths do not invoke it.
- **Home teardown** ends the hosted profile (`VrActivity.java:2085–2090`). Restoring the prior profile calls `engine.setProfile`, which releases all virtual input (`InputBindingManager.java:75–82`; `InputBindingEngine.java:33–40`, `261–267`). Home does not have the same retained-input problem as merely closing the menu, but it does trigger the classic save-loss defect.

## Incidental specific game control mismatch found in the traced call site

While verifying the classic continuation path, the existing Snake direction UI exposed a separate, source-only mismatch: UP and DOWN buttons are drawn at `y=+1.02/-1.02`, height `.36` (`classic_games_module.cpp:1015–1021`), but pointer direction changes require `hy>1.38` or `hy<-1.38` (`1740–1744`). The visual button centers and their full background extents are outside those vertical activation thresholds. Clicking the visible UP/DOWN center therefore does not select the labeled direction through this handler; the generic primary action still starts the snake. Horizontal pad controls remain implemented. This was not added to the immutable original report and was not broadened into a new test suite; preserve it as a targeted interaction defect to reproduce with one real-module ray click at each visible button center.

## Evidence limit and next acceptance

These routes were traced through production Java/JNI/native source; no Android/headset interaction was executed for this follow-up. The eight classic Destroy/Create failures remain runtime-backed by the prior logs. Do not promote menu input release, safe pause or Home resume to passed acceptance based on initial launch screenshots. The next bounded verification should cover the held-input sequence, ten seconds of open-menu gameplay invariance, and Home→Resume exact match restoration after the identified defects are repaired.
