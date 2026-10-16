# RV-0242 — DOS comfort: persistent world-screen placement

The existing two DOS presentation modes serve different needs. Immersive
fullscreen is a head-locked 2D image, centered within the 70% per-eye
optical safe area, and must not inherit world-space translations. Windowed
DOS is a 3D world-space quad with a separate right-side options panel;
the user needs to tune its perceived size, distance and height without
distorting pixels or changing DOSBox guest input/simulation.

The immutable DosScreenPlacement model clamps invalid/NaN values to defaults,
bounds size 0.75–1.20, viewing distance 2.20–4.20 meters and height
−0.48–+0.48 meters. Controls step by 0.05, 0.15m and 0.08m. Reset returns
the original 0.80m half-height, 3.20m distance and zero height. Game-specific
values use the validated DOS module ID as a preference key; only the launched
module's placement is read. DOS/VR framerate caps, head pose, Cardboard
projection, raw eye viewport, guest texture and pointer UV are unchanged.

The render quad retains displayAspect by computing width from scaled height,
rather than independently limiting only width. Very wide/large screens shift
left enough to leave the DOS options rail at right unobstructed; actual
reticle/eye visibility must be verified in the headset. The existing DOS
menu gains SCREEN POSITION, and a compact same-rail adjustment view contains
eight direct actions without a new tab. Opening from active DOS pauses the
guest. Back returns to DOS overlay, then returns to play via the original
resume route. The universal shell Quick Menu retains Home/Exit/Bindings
and preserves headset-only fallback. Attempting an adjustment at its hard
bound reports the standard UI rejection rather than silently pretending to
change the screen.

Pure Java placement source compiled and standalone smoke test passed.
JUnit test source for reset and limits committed. No Android build, install,
signed package or device acceptance is claimed. Validate 4:3 and 5:4 DOS
games, wide modes, both Cardboard eyes, mouse look accuracy, options rail
selection, controller disconnect, pause/resume and per-game persistence.
Further video-cinema and native flat-screen placement remains separate
implementation work, not covered by this DOS-only change.
