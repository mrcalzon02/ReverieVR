# RV-0243/0244 — Thermal notices and disconnected-controller safety

## Scope and honesty

Power and heat warnings are informational, not GPU thermal measurement or
automatic throttle. Native PowerManager status codes, when Android supports
them, are prioritized; Android battery percentage and battery-thermistor
EXTRA_TEMPERATURE are independently available on the phone.
Battery thermistor values must never be presented as CPU/GPU temperature.
The code uses 42°C and 45°C battery-sensor thresholds as conservative,
noncertified heuristics for an informational warning, not safe-operating
certificates. Unknown sensors are ignored rather than guessed. Battery
charge <=15% warns to save. Android MODERATE/SEVERE/CRITICAL advise cooling
or leaving VR. These thresholds require sustained device evaluation.

User can opt out with Quick Menu → Settings → BAT / HEAT WARNINGS.
The separate texture is only 512x72, shown identically in both eyes
near the lower safe region, hidden over the universal Quick Menu,
and its level is sampled at most once every second. Texture uploads
happen only on warning changes. No background worker, network,
forced pause, modal interruption, frame-rate cap, or GLES blocking call
was added.

ControllerManager gates post-disconnection provider snapshots to prevent
a late 'down' after VirtualInputBus releaseAll. VrActivity resets prior
gesture/menu/touch/button edges and DOS mouse-pose accumulators on
provider connection loss and generic Android input-device removal.
The existing 'look upward deliberately for five seconds' controllerless
recovery path remains, avoiding an unsolicited menu over DOS play.

## Acceptance

Verify:
- when a warning is unavailable no fake temperature appears;
- battery level 15/16 and temperature thresholds 42/45°C, Android thermal
  moderate/severe/critical, precedence and opt-out persistence;
- warning has binocular readable text and does not cover critical DOS HUD,
  Quick Menu focus, or controller reticle;
- 15+ minute sustained Galaxy S9 test to compare thermal/battery readings,
  with warnings enabled and disabled;
- BLE unplug/power-off mid-touchpad-press; Android HID removal mid-stick
  or held key; no phantom input on reconnect; no control reactivation
  until deliberate new input; DOS guest state and UI recovery continue;
- signed APK Gradle/JUnit and physical-device tests before acceptance.

Pure policy code locally compiled and standalone smoke executed. Source
readback verified. Real Android APK build, JUnit execution, thermal
threshold verification, hardware input reconciliation and headset visuals
are still pending. Broader thermal-quality fallback, controller policy
across each native game and per-module pause-on-loss belong to separate
measured work; these were not implemented in this slice.
