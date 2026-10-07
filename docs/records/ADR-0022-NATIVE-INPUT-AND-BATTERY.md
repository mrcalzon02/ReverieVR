# ADR-0022 — Native Input Re-arm, Gentle Recenter, and Live Battery

**Status:** Implemented; Galaxy S9 validation pending
**Date:** 2026-10-06

## Touchpad movement input integrity

The first-party native locomotion path consumes one immutable controller
snapshot per renderer frame. The movement gate starts disarmed and requires
a complete finger release before accepting touch. Scene transitions,
Quick Menu visibility, touchpad clicks, stale BLE packets and controller
disconnection disarm it. No already-held finger can move the camera as
the module or UI context changes. This gate affects native locomotion
only; other game bindings and Select behavior remain unchanged.

## Controller recenter

A deliberate shake requires two sharp, substantially opposing acceleration
impulses separated by 65–350 ms, with a three-second cooldown. While the
touchpad is in use the detector resets to avoid unintended recentering.
The recenter action no longer hard-resets controller position, velocity,
body-anchor smoothing or tracked orientation. Instead it enables a
three-second exponential positional return to neutral; accelerometer
movement remains live. This is intentionally a gentle, non-snap action.
Follow-up phone-test #74 implemented the translucent headset-relative
neutral-target controller, procedural spring, and generated audio cue.
Stereo/audio/gesture comfort still requires Galaxy S9 physical proof;
see `CONTROLLER-GHOST-RECENTER-CONTRACT.md`.

## Battery telemetry

The already registered ACTION_BATTERY_CHANGED receiver now reads
EXTRA_LEVEL and EXTRA_SCALE, rejects invalid values and updates the
renderer only on valid changes. The BatteryManager capacity query on
startup/resume remains a fallback. Unchanged percentage readings do not
dirty the HUD texture. One-minute performance logs now see the latest
delivered battery level, but percentage is quantized and is not a
high-precision energy meter; on-device discharge/thermal tests remain
necessary. Pure JVM conversion and behavior tests cover the changes.
