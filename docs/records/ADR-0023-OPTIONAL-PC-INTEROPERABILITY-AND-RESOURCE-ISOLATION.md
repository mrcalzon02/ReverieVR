# ADR-0023 — Optional PC Interoperability, Isolation and Low-Latency Media/Input

**Status:** Accepted product/architecture decision; APK-bundled portable Windows bootstrap source implemented; Windows/S9 hardware proof pending  
**Date:** 2026-10-11  
**Authority:** `docs/project/INSTRUCTIONS.md`, `README.md` privacy contract, `docs/project/PHONE_SCREEN_PANEL.md`, and ADR-0009/ADR-0010

## Context

A computer attached to a phone running ReverieVR is an **unmanaged, independently busy machine**. It may be in a voice call (Discord, TeamSpeak, etc.), playing music, transcoding or processing video, moving large amounts of data, downloading files, otherwise saturated, or doing nothing. These tasks are neither ReverieVR's business nor a reason to block VR. The phone is the sole VR computer, installed as one APK.

The user's interoperability priority is **low-latency control and explicitly selected screen sharing**, not a remotely executed VR environment, distributed compute platform, remote PC administrator, USB hub emulator, or general-purpose PC activity monitor.

## Decision

1. **No workload coupling.** Never enumerate host processes/windows for workload analytics, examine what other PC software is doing, read microphone/call audio, track CPU/GPU/storage use as an application-management service, suspend other programs, reserve host resources, or require a long-running privileged desktop agent. A transport-specific observation that a connection is slow/unresponsive is permissible for user-visible connection health; it is not permission to scan or profile the computer.
2. **Single APK is authoritative.** The Android APK owns Cardboard/stereo/pose rendering, VR shell, local native games, emulator execution when admitted, input-action interpretation, saves, privacy/recovery and gaze dwell. External connectivity is always removable during a session.
3. **One APK, bundled optional host utility.** The owner explicitly authorizes a separate PC-side *link executable/script only when delivered as part of the existing ReverieVR APK* and started deliberately by the user on Windows. There must be no second required download, desktop installer/distribution, covert service or PC-hosted VR runtime. A portable Windows PowerShell/cmd bundle is now packaged as APK assets and exported through Stage A; it currently launches independently installed official scrcpy/ADB for user-selected mirroring/controller modes and read-only optical-drive discovery. Windows SMB2/SMB3 and its in-APK Android SMB client remain supported for normal files and ISO imports. USB and Wi-Fi/hotspot are transport choices, not separate software ecosystems.
4. **Only activated capabilities send traffic.** Optional PC-mediated control transmits the chosen device's required button/axis/pose states and minimal session/sequence/health framing. No unrequested audio, screenshots, desktop recordings, file trees, telemetry, diagnostics, media scans, or process information. Local networking requires explicit peer authorization and should authenticate/limit the peer's permitted capabilities. Discovery is bounded and inactive by default.
5. **Screen-sharing directions remain distinct:**
   - **Phone → PC:** user-initiated external scrcpy may mirror the device through Android/ADB. This is not data uploaded or collected by the ReverieVR APK, and its connection/encoder workload is not a required runtime dependency. No covert recording.
   - **Phone screen → VR phone panel:** a future on-device MediaProjection display path remains local to the handset, only after explicit Android consent, and may not forward its pixels to a peer; no OCR, image understanding or analysis, ever.
   - **PC → phone VR display:** optional desktop/window image ingress is a separate future feature, **not yet built**. It requires explicit selection/permission on the computer, an authenticated local connection, no automatic desktop discovery, bounded and hardware-efficient decode on the phone, strict close/stop behavior and performance validation. It must never hijack the phone's stereo rendering or force a desktop screen into view.
6. **Latency isolation.** The input path must not wait for a screen frame, disc/image transfer, media transcode, file copy, network discovery, or host CPU activity. Use bounded asynchronous queues, drop superseded continuous-axis samples instead of accumulating latency, preserve ordered button up/down transitions and release held state on loss, reject stale or duplicated packets, and apply bounded timeouts. Screen sharing uses its own cancellable transfer/decoder path and may lower quality/drop frames before affecting headset motion or local games. No network operation may block the GLES/Cardboard render thread.
7. **Graceful degradation.** Host congestion, disconnection, sleep or restart only affects the optional external input/media. The phone's shell, native content, already-imported media and five-second gaze-selection fallback stay operational. Any unavailable remote live media is visibly marked unavailable. Connection retries must use capped backoff without flooding the PC or network.
8. **Controlled host-side extension.** The portable, APK-exported Windows helper can offer user-initiated advanced capabilities when direct Windows hardware access is genuinely required. It must be separately started by the computer's owner, remain nonprivileged and inactive when not in use, and expose only explicitly selected devices/media after user-authorized pairing. Current bundled bootstrap is a scrcpy mode launcher and optical-drive inventory, **not** raw sector transfer, desktop video capture, or automatic Windows-to-Android USB-hub emulation. A helper's later read-only optical sector protocol and opt-in desktop-streaming source require separate scoped implementation and tests. Ordinary SMB shares files and cannot substitute for raw optical sectors.

## Rejected approaches

- Treating the PC as the VR game/emulation renderer or required desktop companion;
- Requiring a separately downloaded/installed PC application, privileged agent or always-on daemon. A user-launched portable helper *extracted from the one APK* is specifically authorized;
- software pretending Android is connected to all USB devices on the PC via scrcpy;
- attempting to unlock Android 11 wireless ADB pairing through a normal Android 10 app;
- universal always-on screen capture, network desktop scraping, OCR, or automatic audio capture;
- coupling controller input to a video stream's refresh rate or network file-copy throughput;
- allocating significant PC resources by default when the user may be doing unrelated work.

## Acceptance and evidence

A future PC integration is not complete until it passes phone-only/offline startup, no-controller gaze recovery, optional accessory attach/detach, authenticated pairing, no background capture, no PC workload/process inspection, controller release on packet loss, bounded queue/stale-input checks, and concurrent heavy PC video/disk workloads. Measure real Galaxy S9 headset frame pacing and p50/p95 controller event-to-action latency with a loaded host, both wired and local Wi-Fi, plus observable reconnect behavior; do not claim latency targets met before measurements.

## Next authorized development path (inside the existing APK)

1. **Retain** the existing in-VR source selector and `scripts/scrcpy-reverie.*` tools. The optional user-exported `ReverieVR-Laptop-Link.zip`, sourced entirely from the Android APK, now includes Windows scripts with user-selected scrcpy controller-only, mirrored controller, and wireless-ADB setup modes, plus optical-drive inventory. The official scrcpy tool remains an existing third-party dependency for these modes and may need independent installation. The headset requires none of it.
2. **Implement** a bounded, read-only **SMB2/SMB3 client inside ReverieVR** for an explicitly specified Windows host/share and credentials. Evaluate a compatible permissively licensed Android Java library, binary size, transitive license notice and Galaxy S9 performance before adding it. Use a background executor, cancellable directory browse, strict filename/path bounds and size/hash verification on local import. Store credentials in Android-protected storage (never docs/logs). Network discovery and Windows SMB/firewall changes are not automatic. Users may select previously permitted media via the existing Android SAF flow; SMB is an optional source, not a prerequisite for local playback.
3. **Integrate** the client into a functional in-headset `Laptop Files` browse/import path only after the connection and file transfer work; no placeholder buttons. Keep the external controller source under existing Controller Source settings. Both a Windows hotspot and existing router Wi-Fi use the same SMB client; Windows sharing/firewall must permit access on the actual connected network.
4. **Consider** an optional USB/ADB-forwarded SMB connection only after a real host-side and S9 round-trip proves loopback access to an already enabled Windows share. This may still require a user-entered `adb reverse` command and does not bypass Android 10's protected wireless debugging or enable native USB hub semantics.
5. **Screen sharing:** continue supporting user-initiated phone-to-PC mirroring via existing scrcpy. A PC-to-phone screen viewer is a separate future APK-side *client* for an existing authorized protocol/source, not permission to author a Windows streamer. If there is no compatible host source, report unsupported instead of inventing one.

## Verification

Verify the phone-only runtime without the laptop first; then perform independent heavy-PC-load tests for input-only scrcpy and optional mirroring, and source-to-phone Windows SMB browse/import with signed APK on S9. No video encoder should run for the control-only command. Check missed/reordered button releases, bounded transfer memory, 5-second gaze recovery and no privileged or auto-started PC software.

Keep tests and claims separated: scrcpy provides *external* mirroring/HID, Android SMB browsing and DOS import exist in source pending device verification, and the APK-bundled Windows helper's ZIP export is implemented in source pending signed-build and Windows-device validation. A raw optical-device transport and any PC-to-phone desktop viewer remain **planned, not working features**. No second Android APK or separately distributed desktop application is required by the design.


## 2026-10-16 scope addendum — optional SteamVR game streaming

The owner now separately authorizes an *optional SteamVR HMD endpoint*
as a PC VR game compatibility path, **not** a migration of ReverieVR
modules/home/emulators to PC. This deliberately scoped new mode uses a
Windows SteamVR driver/streamer and phone client, never a general-purpose
remote desktop/runtime. Existing core Android standalone requirements
and constraints on privacy, paired peers, no background services and
user-started desktop utilities remain in force. Driver registration,
video consent and mode entry require explicit action; phone-side
recovery remains local. The executable contract is
`docs/project/STEAMVR-LINK-IMPLEMENTATION.md`. This supersedes earlier
statements that *any* PC-hosted VR frame path is outside project scope,
but only as applied to user-requested SteamVR games.
