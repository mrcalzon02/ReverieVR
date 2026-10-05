# ADR-0004 — Persistent Platform Shell and Global Status HUD

**Status:** Accepted  
**Date:** 2026-10-04

## Context

ReverieVR is intended to host more than one VR scene: a 3D home, local video player, diagnostics/settings surfaces, and small VR games or experiences.

If each experience owns its own global controls, recovery path, battery/status presentation, or platform settings, behavior will drift and failures can strand the user inside a module. The project therefore needs a durable boundary between the ReverieVR platform shell and the content running inside it.

Stage A already persists user-facing preferences for a future VR battery HUD, look-up-triggered reveal behavior, and numeric battery percentages. Those controls need an explicit Stage B contract so they are not merely decorative setup switches.

## Decision

ReverieVR Stage B is a **persistent platform shell**.

The shell remains authoritative around the home, media player, and hosted games/modules. It owns:

- return-to-home and safe-exit behavior;
- the route back to Stage A recovery/setup;
- global settings access;
- global input-action policy;
- platform diagnostics/status access;
- shell-global handset/controller power status;
- pause/resume/exit boundaries for hosted modules.

Hosted experiences may render their own content and local UI, but they consume platform services instead of replacing the shell's global responsibilities.

### Global power/status HUD

The reference presentation contains two compact percentage/progress bars in the upper-right portion of the user's VR view:

1. handset battery percentage;
2. currently bound controller battery percentage.

These indicators are a shell capability and therefore remain available regardless of which ReverieVR-hosted experience is active.

Telemetry must be honest. If the selected controller protocol/runtime cannot provide battery state, the controller indicator reports an unavailable/unknown state instead of fabricating a percentage.

### Optional gaze-adaptive presentation

A user-selectable quality-of-life mode may reduce persistent visual clutter by retracting the status HUD during ordinary forward viewing and bringing it down into comfortable view when a deliberate look-up gesture is detected.

Head pitch is the baseline input for this behavior. Controller orientation may also be considered if the selected controller stack exposes it reliably. Exact thresholds, hysteresis, animation, and comfort timing are implementation details and require device testing.

The adaptive behavior is **not mandatory**. The Stage A preference already exists and Stage B must honor it. When disabled, a predictable persistent/manual status presentation remains available.

This opt-out rule applies generally to automatic quality-of-life behaviors that materially move, hide, reveal, recenter, or otherwise alter platform UI/interaction.

## Consequences

- The video player and games are hosted content, not separate platform silos.
- Shell-global UI can be implemented once and remain coherent across experiences.
- Recovery/settings access cannot depend on a module implementing it correctly.
- Modules must not silently bypass global input/settings/recovery contracts.
- Battery reporting becomes a controller-stack research requirement.
- The HUD must be cheap enough to satisfy the performance-first doctrine.
- Gaze-adaptive presentation requires comfort tuning and must not flap rapidly near its threshold.
- Immersive modules may request reduced noncritical HUD visibility only through explicit reversible shell policy; they do not own the recovery path.

## Affected targets

- RV-0002 — controller/runtime selection must consider battery telemetry capability.
- RV-0100 — persistent shell/home architecture.
- RV-0102 — shell-owned input-action abstraction.
- RV-0104 — persistent settings and safe reset.
- RV-0106 — shell-global power/status HUD.
- RV-0107 — optional gaze-adaptive status behavior and QoL toggles.
- RV-0400 through RV-0402 — module lifecycle/API and shell-contract enforcement.
