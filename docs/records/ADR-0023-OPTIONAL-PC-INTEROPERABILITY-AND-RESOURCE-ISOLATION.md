# ADR-0023 — Optional PC Interoperability, Isolation and Low-Latency Media/Input

**Status:** Accepted product/architecture decision; optional PC-native transport beyond scrcpy remains planned, not implemented  
**Date:** 2026-10-11  
**Authority:** `docs/project/INSTRUCTIONS.md`, `README.md` privacy contract, `docs/project/PHONE_SCREEN_PANEL.md`, and ADR-0009/ADR-0010

## Context

A computer attached to a phone running ReverieVR is an **unmanaged, independently busy machine**. It may be in a voice call (Discord, TeamSpeak, etc.), playing music, transcoding or processing video, moving large amounts of data, downloading files, otherwise saturated, or doing nothing. These tasks are neither ReverieVR's business nor a reason to block VR. The phone is the sole VR computer, installed as one APK.

The user's interoperability priority is **low-latency control and explicitly selected screen sharing**, not a remotely executed VR environment, distributed compute platform, remote PC administrator, USB hub emulator, or general-purpose PC activity monitor.

## Decision

1. **No workload coupling.** Never enumerate host processes/windows for workload analytics, examine what other PC software is doing, read microphone/call audio, track CPU/GPU/storage use as an application-management service, suspend other programs, reserve host resources, or require a long-running privileged desktop agent. A transport-specific observation that a connection is slow/unresponsive is permissible for user-visible connection health; it is not permission to scan or profile the computer.
2. **Single APK is authoritative.** The Android APK owns Cardboard/stereo/pose rendering, VR shell, local native games, emulator execution when admitted, input-action interpretation, saves, privacy/recovery and gaze dwell. External connectivity is always removable during a session.
3. **Reuse standards first.** Prefer existing scrcpy/ADB only for user-started phone-to-PC mirroring and laptop-controller forwarding, Android Bluetooth/USB HID for directly attached devices, and ordinary user-approved local IP networking where a separate bridge is actually needed. Do not invent a heavy proprietary streaming stack to duplicate scrcpy. USB and Wi-Fi/hotspot are alternative transports, not distinct feature implementations.
4. **Only activated capabilities send traffic.** Optional PC-mediated control transmits the chosen device's required button/axis/pose states and minimal session/sequence/health framing. No unrequested audio, screenshots, desktop recordings, file trees, telemetry, diagnostics, media scans, or process information. Local networking requires explicit peer authorization and should authenticate/limit the peer's permitted capabilities. Discovery is bounded and inactive by default.
5. **Screen-sharing directions remain distinct:**
   - **Phone → PC:** user-initiated external scrcpy may mirror the device through Android/ADB. This is not data uploaded or collected by the ReverieVR APK, and its connection/encoder workload is not a required runtime dependency. No covert recording.
   - **Phone screen → VR phone panel:** a future on-device MediaProjection display path remains local to the handset, only after explicit Android consent, and may not forward its pixels to a peer; no OCR, image understanding or analysis, ever.
   - **PC → phone VR display:** optional desktop/window image ingress is a separate future feature, **not yet built**. It requires explicit selection/permission on the computer, an authenticated local connection, no automatic desktop discovery, bounded and hardware-efficient decode on the phone, strict close/stop behavior and performance validation. It must never hijack the phone's stereo rendering or force a desktop screen into view.
6. **Latency isolation.** The input path must not wait for a screen frame, disc/image transfer, media transcode, file copy, network discovery, or host CPU activity. Use bounded asynchronous queues, drop superseded continuous-axis samples instead of accumulating latency, preserve ordered button up/down transitions and release held state on loss, reject stale or duplicated packets, and apply bounded timeouts. Screen sharing uses its own cancellable transfer/decoder path and may lower quality/drop frames before affecting headset motion or local games. No network operation may block the GLES/Cardboard render thread.
7. **Graceful degradation.** Host congestion, disconnection, sleep or restart only affects the optional external input/media. The phone's shell, native content, already-imported media and five-second gaze-selection fallback stay operational. Any unavailable remote live media is visibly marked unavailable. Connection retries must use capped backoff without flooding the PC or network.
8. **Minimal accessory footprint.** Where an optical drive or user-selected PC window genuinely requires desktop access, any helper is a small, explicit, unprivileged, optionally launched peripheral provider. It does not ship as a required Android APK, auto-start without user consent, modify Windows services/firewall rules without authorization, or depend on exclusive hardware ownership. Neither the presence nor behavior of third-party PC apps is assumed.

## Rejected approaches

- Treating the PC as the VR game/emulation renderer or required desktop companion;
- software pretending Android is connected to all USB devices on the PC via scrcpy;
- attempting to unlock Android 11 wireless ADB pairing through a normal Android 10 app;
- universal always-on screen capture, network desktop scraping, OCR, or automatic audio capture;
- coupling controller input to a video stream's refresh rate or network file-copy throughput;
- allocating significant PC resources by default when the user may be doing unrelated work.

## Acceptance and evidence

A future PC integration is not complete until it passes phone-only/offline startup, no-controller gaze recovery, optional accessory attach/detach, authenticated pairing, no background capture, no PC workload/process inspection, controller release on packet loss, bounded queue/stale-input checks, and concurrent heavy PC video/disk workloads. Measure real Galaxy S9 headset frame pacing and p50/p95 controller event-to-action latency with a loaded host, both wired and local Wi-Fi, plus observable reconnect behavior; do not claim latency targets met before measurements.

Keep tests and claims separated: scrcpy currently provides an *external* mirroring/HID path; a ReverieVR-native PC-to-phone screen bridge and optical drive transport are **proposals, not working features**. No second APK is required by the design.
