# VR Quality-of-Life Research Baseline

**Status:** Adopted as design input  
**Date:** 2026-10-04

## Why this exists

Mature VR platforms have accumulated years of recurring friction reports. ReverieVR should not repeat avoidable mistakes merely because the reference hardware is older.

The goal is not to imitate every modern headset feature. It is to capture the **system-level conveniences, comfort controls, accessibility defaults, and recovery paths** that users repeatedly notice when they are absent.

## Strong recurring themes

### Remember preferences globally

Users repeatedly complain about setting snap/smooth turning, vignette/tunneling, subtitles, or similar defaults again in every title.

ReverieVR therefore owns a persistent user comfort/accessibility profile. Hosted modules may override only when their design genuinely requires it, and should otherwise consume platform defaults.

### Recenter must be trivial

Recenter and seated-height repair must not be buried several menus deep. A seated platform should assume the user will occasionally shift chair, posture, headset position, or launch into a module with the wrong reference frame.

Universal recenter is a shell function.

### Fixed-position use is first-class

The reference system is seated 3DoF VR. Essential shell interactions must never require standing, reaching the floor, or physically rotating beyond comfortable seated range.

### Readability beats decorative UI

Critical text needs adjustable size, strong contrast, comfortable focal distance, and a layout that does not demand constant neck movement. Color is never the only carrier of important state.

### Audio cannot be the only signal

Dialogue, warnings, and state changes need captions/subtitles or visual equivalents. Caption size, backing, and location should be adjustable.

### Input must be flexible

ReverieVR starts with one Daydream controller, but the action layer should permit:

- remapping;
- left/right-hand presentation;
- one-controller complete shell navigation;
- generous targeting tolerances;
- optional gaze/dwell selection for shell fallback;
- future generic gamepads without rewriting modules.

### Controller loss is a recoverable pause, not a catastrophe

Disconnect, low battery, gyro drift, and bad deadzones need explicit status and recovery. Modules should pause where appropriate, preserve state, and allow reconnection without dumping the user back to Android.

### Routine notifications must not occupy the center of vision

Users strongly object to modal system panels that obscure exactly what they are trying to look at. Battery, reconnect, update, and status messages should use compact peripheral treatment unless immediate confirmation is genuinely required.

### Automatic conveniences must remain optional

A behavior that is wonderful for one person can be infuriating when it triggers accidentally. Gaze reveal, automatic update checks, dwell selection, break reminders, comfort vignettes, notification animation, and similar conveniences need user controls.

### Quick access matters

The VR shell should provide a compact universal panel for frequently needed functions without leaving the current module:

- handset and controller battery;
- time/session duration;
- volume;
- brightness;
- thermal/performance warning;
- recenter;
- Home;
- Settings;
- controller status;
- pause/return;
- exit/recovery.

### Reduce motion in the interface too

Motion comfort is not only about gameplay locomotion. UI animation, large moving backgrounds, peripheral animation, zooms, and head-locked elements can also be uncomfortable.

A reduced-motion setting should simplify transitions and disable nonessential animated peripheral behavior.

### Optional real-world peek is worth investigating

Modern users value rapid access to their surroundings. The Galaxy S9 cannot provide modern stereo passthrough, but a short-lived rear-camera view may still be useful for locating a desk, drink, cable, or person.

This is research-only until tested. It must be clearly labeled as a **monocular convenience view**, never a safe room boundary, depth-accurate passthrough, or permission to walk around while wearing the headset.

## ReverieVR rule

Quality-of-life automation is **opt-out by default where practical**. Recovery-critical controls remain available even if their convenience presentation is disabled.
