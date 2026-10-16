# RV-0238 — Observable callback pacing, optional in-eye HUD

ReverieVR prioritizes a sustainable, even 30 FPS game rendering pace,
with responsive 60 Hz headset presentation wherever support permits.
The existing Cardboard onNewFrame callback is **not** a GPU finish or
SurfaceFlinger present event. An FPS average alone cannot prove even
frame spacing.

## Implemented

Quick Menu → Settings: PERF: ON/OFF, persisted and default OFF, placed on
the bottom row alongside existing HID and contrast actions. Uses the same
ray/gaze/select routing and avoids a separate settings screen.

When enabled, the compact binocular panel sits under the standard FPS
badge, drawn once in each restored Cardboard eye viewport using a shared
512x256 texture that updates at most once per second. It hides while
the shell Quick Menu is open. Regular FPS badge and battery HUD continue
independently; no graphics synchronization or sleep was added.

Observed metrics, all bounded to 600 callback intervals:
- Cardboard app-frame callback FPS (not physical display FPS);
- average, P95, P99, maximum and population-standard-deviation frame
  intervals in milliseconds; ring resets on shell mode changes;
- counts of observed intervals >33.333334ms and >50ms (NOT exactly
  missed VSync counts);
- per-eye GL-thread **CPU submission** P95/P99, sampled with System.nanoTime
  around Java onDrawEye, not GPU execution;
- Android battery temperature and OS thermal status if supported.

Explicitly NOT MEASURED: actual GPU draw completion, GPU utilization,
real display presentation timestamps, VSync misses and reprojection frame
reuse. A future hardware capability probe may add properly supported
nonblocking GPU disjoint-timer queries and compositor timing, but must not
fabricate metrics where unavailable.

The renderer saves one dated, bounded report into existing app-private
preferences at most every 10 seconds. All diagnostic ZIPs include the
last sampled data in `performance.txt`, regardless of verbose log setting;
no user file paths or location data are included in that separate record.
Standard minute performance logs inherit the additional stats.

## Acceptance and remaining gates

Added JVM test source for percentiles, jitter, >33/>50ms gap counts,
reset, honest unknown state, CPU-eye labels, thermal status and dated output.
In a real Galaxy S9 test, verify Quick Menu selection and press/release,
both-eye placement/legibility and phone optics, native/DOS/video mode
transitions, pause/resume, persistent on/off, and ZIP output for both
logs-included and logs-excluded exports. Compare sustained frame pacing
and battery temperature with PERF off/on; reject if instrumentation
itself causes perceptible stalls. Real Android build, tests and headset
runtime evidence remain pending.

## Performance notes

Normal callbacks do not allocate for the raw interval/eye timing rings.
Sorted interval snapshots and string allocation occur once per second
only when PERF enabled or once per 10 seconds for exported diagnostic
samples. Overlay uses existing GLES2 texture program and no new shader,
mesh or dependencies. GPU work is a few triangles and an occasional
RGBA upload while visible. No caller should equate native/DOS game
simulation FPS with Cardboard callback FPS.
