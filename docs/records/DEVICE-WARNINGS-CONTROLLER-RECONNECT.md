# RV-0243/RV-0244 — Quiet thermal warnings and reconnect re-arm

## Device-side sources and labels

The user's Samsung Galaxy S9 is a finite thermal device. BAT / HEAT WARNINGS
is a persisted on/off Quick Settings control, with a compact nonmodal status
texture rendered once in each eye only for real, classified conditions.
Values are Android PowerManager thermal status (when available), battery
temperature obtained through Android BATTERY_CHANGED and phone battery
percentage. Battery degrees Celsius MUST NOT be described as CPU or GPU
temperature. No warning is inferred when a metric is unavailable. The
policy uses battery temperature thresholds of 42/45 degrees C and charge
of at most 15%, with higher OS thermal statuses prioritized.

A source-side DeviceWarningLatch is evaluated on the existing one-second
sensor sample cadence. A higher severity becomes visible immediately;
a lower severity must remain below the prior level for 10 seconds before
the old warning is replaced or removed. Disable-to-OFF immediately
resets the latch and hides the banner; re-enabling uses current sensors.
It does not change presentation refresh, native/DOS simulation clock,
or background application state, nor does it autonomously exit VR.

## Controller reconnect input policy

Existing VrActivity releaseAll actions stop stuck guest keyboard/mouse/
joystick state when a provider leaves READY or an Android input device is
removed. ControllerManager now also clears packet/button edge state and
requires a fully neutral ControllerSnapshot on every fresh connection:
touching=false; touchpadPressed=false; Menu, Home and both volume buttons
all false. Until that arrives, stale/held controller snapshots are not
forwarded to menus or games. The system does not suppress controller-free
headset pose, five-second gaze recovery, or a manually opened Quick Menu.
Returning to play requires releasing the physical input first, rather than
re-pressing held input involuntarily after reconnect.

## Testing and remaining gates

Pure helper JUnit tests added for threshold de-escalation, immediately
escalating dangerous conditions, toggled reset, monotonic timestamp
rollback, reconnection held touch/menu denial, and re-arm only after
release. Equivalent helpers with a lightweight packet fixture compiled
locally using Java 21 and a standalone smoke harness passed 17 runtime
assertions; this is **not** a signed APK, exact Android integration
compile, Gradle/JUnit acceptance or physical Galaxy S9 evidence.

Device test on S9: monitor real BATTERY_CHANGED temperature and Android
thermal status; cross 42 and 45 C only through controlled/safe conditions,
never deliberately overheat the phone. Validate warnings OFF, source
unavailable, no overlay collision, reasonable visual size and normal FPS.
Disconnect Daydream mid touch/click and mid DOS mouse key, ensure guest
release and no reconnect ghost press; confirm deliberate post-neutral
Select, gaze recovery with no controller, and Home / Exit recovery on
both eyes. No forced automatic popup or fullscreen mode alteration.
