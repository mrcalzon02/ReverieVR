# ADR-0003 — Native Android Stage A Shell

**Status:** Accepted  
**Date:** 2026-10-04

## Context

The pre-headset setup surface must work before a VR runtime or Daydream controller stack is ready. Coupling ordinary controller setup/recovery UI to a game engine or VR framework would increase APK size, startup cost, thermal overhead, dependency risk, and recovery complexity.

## Decision

Stage A uses the Android platform directly:

- Java source;
- platform Activity and View widgets;
- XML resources/layouts;
- SharedPreferences for current local bootstrap settings;
- no AndroidX;
- no Compose;
- no game engine;
- no network permission;
- no dependency on discontinued Google Daydream services.

Bootstrap toolchain:

- Android Gradle Plugin 9.4.1;
- Gradle 9.6.1;
- JDK 17;
- compile/target SDK 36;
- minimum SDK 26.

This ADR decides only the conventional Android Stage A shell. It does not select the Stage B stereoscopic renderer or Daydream controller transport. Those remain RV-0002.

## Consequences

- Controller pairing/recovery remains usable even if the VR layer fails.
- The initial APK dependency surface is extremely small.
- Stage B must expose a clean boundary that Stage A can enter and recover from.
- UI sophistication is intentionally secondary to reliability, readability, and low overhead.
- The project can replace the Stage B implementation later without rebuilding the setup shell around a different UI framework.
