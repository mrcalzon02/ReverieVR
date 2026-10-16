# ReverieVR — SteamVR headset endpoint implementation

**Target:** RV-0246. **Authoritative main:** mrcalzon02/ReverieVR.
**Scope:** PC runs *SteamVR games only*. The Galaxy S9 runs existing ReverieVR
APK and acts as a tracked, stereo, low-overhead streaming endpoint. No
porting of Android native games, DOSBox, media systems or VR Home to Windows.
Standalone VR must work unchanged when the PC is absent.

**Status:** Architecture and first functional Windows prerequisite probe
committed. No registered ReverieVR SteamVR HMD, video streamer or Android
VR network client exists yet. Do not present the new probe as a working
PCVR connection. No Galaxy S9, Windows host, Android build or actual
SteamVR driver-load proof has yet been performed for this program.

## System boundary and data paths

```
 Windows 10/11 x64 PC                       Galaxy S9 / Daydream View
 Steam + SteamVR + games                    ReverieVR Android APK
       |                                          ^
 OpenVR headset driver                           | latest head pose
   (IServerTrackedDeviceProvider) <== CONTROL ===| Daydream head IMU
       |                        <== INPUT ========| controller buttons/pose
 SteamVR compositor frames                      |
       |                                          |
 compositor-owned texture handoff                |
       v                                          v
 hardware video encoder === VIDEO CHANNEL ===> MediaCodec decoder
                                            -> stereo compositor
                                            -> Cardboard viewer correction
                                            -> 60 Hz phone display

 USB: IP over USB tethering/compatible network interface, not raw HDMI
 Wi-Fi: trusted 5 GHz private LAN; preferably PC wired to router
 SteamVR audio -> explicit independent audio stream -> phone headphones
 LOCAL HOME / DOS / NATIVE GAMES REMAIN ON THE PHONE
```

The phone always owns real-time head sensing, local Cardboard viewer
calibration, controller ingestion, VR global Back/Home, the diagnostic HUD,
recovery on disconnect, and local content. Do not ship the entire Windows
ReverieVR runtime: the PC executes only SteamVR and the streaming driver.

## Interfaces and driver choice (do not bluff optical support)

Windows plugin: SteamVR's OpenVR driver interface, implementing
`HmdDriverFactory`, `IServerTrackedDeviceProvider`,
`ITrackedDeviceServerDriver` for a 3DoF HMD, and
`IVRDriverInput` only for real available controller actions.
`TrackedDevicePoseUpdated()` receives legitimate orientation from the S9.
Report tracking capabilities honestly: no natural 6DoF head translation,
no tracked hand/controller position, no fake spatial motions.

SteamVR compositor handoff has **two candidates**, to be proved against
the exact SDK/runtime and the Cardboard warp pipeline:

A. `IVRVirtualDisplay` obtains the final composed/warped backbuffer and
   presents asynchronous video to a virtual display. Requires a supported
   DisplayRedirect path/manifest plus HMD and careful timing. SteamVR may
   already have applied viewer distortion. Rendering through Cardboard's
   warp again without adjusting the client would DOUBLE-CORRECT the image.
B. `IVRDriverDirectModeComponent` retrieves eye textures before the final
   warp (as demonstrated by ALVR Windows). Send **unwarped stereo** and
   perform Cardboard viewer correction/late rotational compensation on
   phone exactly once. More work (D3D11 shared-texture integration) but
   may better support head-tracked latency management.

The first composited stereo checkerboard proof decides the actual path.
Never hardcode a bogus distortion mesh or assert that a standard SteamVR
presented frame is automatically suitable for the existing Video player.
Use Valve OpenVR official samples and ALVR (MIT) as implementation references,
not direct binary substitutes. Pin versions and document licenses before
depending on upstream libraries.

## PC software and install/uninstall

1. User installs/updates Steam and **SteamVR (Steam app 250820)** on
   Windows x64, with graphics drivers supporting a hardware H.264
   encoder; verify hardware capability, do not assume NVENC, AMF or QSV.
2. From ReverieVR Stage A, user manually **Exports Laptop Link ZIP** or
   uses its explicit temporary USB/Wi-Fi share. Transfer/extract and
   launch `Start-ReverieLink.cmd`. No Windows installer/service/admin
   privilege is presumed. Existing scrcpy operations are unaffected.
3. **Implemented now:** menu entry 5 runs `SteamVrPreflight.ps1`.
   It reads `%LOCALAPPDATA%\openvr\openvr.vrpaths` (with older
   filename compatibility), resolves SteamVR runtime, and locates
   `bin\win64\vrpathreg.exe` and `vrstartup.exe`. It lists registered
   external drivers, reports incomplete installations, changes nothing,
   and explicitly says ReverieVR headset streaming is not yet installed.
4. **Future driver build gate:** MSVC x64, CMake, pinned OpenVR SDK headers,
   driver DLL, `driver.vrdrivermanifest`, resource/input-profile JSON,
   portable Windows streamer component, explicit version and SHA-256
   manifest. Build/locally verify the *exact* packed binaries before
   adding them as allowlisted assets of the same single Android APK.
   Current 128 KiB text-asset export limits MUST be revised through a
   scoped audited packaging design; no oversized binary may be silently
   truncated or downloaded from a mutable third-party URL.
5. **Future user install action:** an explicit opt-in in the portable host
   utility verifies the packaged DLL and calls the SteamVR-supplied
   `vrpathreg.exe adddriver <absolute driver folder>` only after
   showing the exact target. Do not modify unrelated driver paths or
   override an existing headset. SteamVR may need restarting *with user
   approval*, never an unprompted process kill. Start host, then SteamVR.
6. **Future uninstall:** stop the host, call the same SteamVR tool
   `vrpathreg.exe removedriver <exact registered folder>`, confirm
   readback, and remove only ReverieVR-owned files after user request.
   No global SteamVR config reset, force flags, firewall broad allow,
   hidden Windows service or external VR dependency for local use.
7. User selects **SteamVR link** on phone only after real connection
   code exists; no inert menu button in the meantime. Explicit pairing,
   stereo lens calibration, seated recenter and session selection. EXIT,
   Quick Menu and five-second gaze recovery remain local.

## Authentication and lifecycle

No opportunistic LAN scanning, publicly exposed video ports or automatic
connection to arbitrary hosts. First connection uses deliberate phone
and Windows confirmation with high-entropy one-time secret/QR and
authenticated session establishment (TLS with certificate identity
validation or a reviewed authenticated-key exchange). A six-digit
code alone is not a sufficient network authentication mechanism.
Store only per-user device identity/consent; never log secrets in
diagnostics or publish a bearer URL. Limit the host to receiving
VR controls and sending user-enabled VR media/health. No PC process/file
inspection, administrator rights, hidden screenshot or desktop capture.

Connection states: OFF -> HOST_READY -> PAIRING -> NEGOTIATING ->
STREAMING -> RECOVERING -> OFF. Reconnect must renegotiate generation
and discard old packets. A stale/disconnected host never receives inputs
from a new session. On a dropped link: mute streamed audio, release
held buttons on both ends, stop rendering stale dynamic poses, present
a safe local reconnect/exit surface, and offer return to local VR Home.
The controller and headset recovery path must not depend on SteamVR.

## Protocol and transport contract (version 1 proposal)

Control and status: authenticated, length-bounded, versioned reliable
channel. Pose updates: nonblocking latest-wins channel with monotonically
increasing sequence, session ID, timestamp and normalized finite head
quaternion. Controller presses/releases: ordered, acknowledged/replayed
only as appropriate; never drop a release, never duplicate a press.
Video: separate bounded channel for framed hardware-compressed frames
(PC -> Android), frame ID, presentation prediction, eye layout, pose ID,
codec configuration and resynchronization/keyframe flag. No bulk desktop
file access and no unbounded queues. Overload strategy: discard late
superseded video, never accumulate a growing latency queue; prioritize
head-tracking/control data over video transfer.

Start with a controlled 5 GHz LAN and straightforward authenticated
transport, prove with synthetic source before claiming real SteamVR.
USB path via Android USB tethering/IP routing where available; legacy
authorized ADB port forwarding is an optional development route, not
a mandatory dependency. Do not assume an Android phone exposes an
HDMI receiver or that a USB cable alone carries video. One codec,
H.264 low-latency, is baseline; HEVC and 90/120 Hz are not initial
S9 commitments. Separate audio negotiation and test A/V drift.

Make monotonic clock offset/uncertainty and pose-to-video association
explicit. The driver should not advertise 6DoF, fake positional tracking
or pretend that Bluetooth/Steam Controller input can provide tracked
hands. A Steam Controller connected directly to PC is a legitimate
gamepad/input source but not a tracked SteamVR motion controller;
Daydream orientation/button controls are 3DoF. Target seated/cockpit
VR apps first; unsupported titles must report incompatibility.

## Client video and lens pipeline

On Android, use hardware `MediaCodec` decode to an actual
`Surface` / `SurfaceTexture` / GLES texture without repeated CPU
pixel-buffer copies. Split left/right views as negotiated, maintain
correct viewer calibration, IPD/eye alignment and the 60 Hz local
Cardboard tracking/presentation loop. Ensure optical distortion is
applied **exactly once**. Local late rotational reprojection/timewarp
is an explicit later acceptance gate: it must use the pose/time
associated with the decoded frame and newest head pose; displaying
old video with an unadjusted new pose is not a correct implementation.

Keep decoded queue depth 1–2 frames initially, make nonblocking frame
selection, reuse last accepted decoded texture only when timing is
appropriate, and log true network/decode/submit intervals separately.
Do not call a 30 FPS streamed-video cap '60 Hz head-tracked reprojection'
unless a real reprojection pass exists.

## Implementation stages and gates

**S0 — Working installer prerequisite (source committed):** SteamVR
preflight is an actual read-only Laptop Link function; bundle whitelist
and JUnit source updated to include it. Needs Android asset/JUnit build
and real Windows SteamVR path checks.

**S1 — Paired tracking/control link:** explicit host/phone pairing, real
authenticated version negotiation, head orientation transport, timestamp
and seq validation, disconnect/release-all, host echo diagnostics.
The PC utility must show received live pose/packet age; no simulated
pose presented as working headset tracking. Test USB and 5 GHz Wi-Fi.

**S2 — Stream transport and stereo test pattern:** host-side synthetic
stereo pattern/H.264 encoder -> MediaCodec hardware decode -> exact
left/right in-eye video with Cardboard viewer calibration and eye labels.
Prove frame order, long-duration stability, no double distortion,
no extra full-frame CPU copies; establish 30/60 cadence measurements.

**S3 — Real SteamVR HMD driver:** build x64 OpenVR plugin with actual
headset pose, HMD registration, selected compositor texture handoff,
virtual-sync timing, bounded hardware encode, consistent display
characteristics and documented reversible installation. Show an
actual SteamVR VR view delivered to the phone in both eyes.

**S4 — Controller and audio:** Daydream gesture/button profile and
optional PC-local Steam Controller mapped only to supported actions;
no fictitious tracked positions. Ordered button releases, native
SteamVR app bindings, optional headset audio stream with resync,
session-safe recenter and per-title seated input checks.

**S5 — Optimized/accepted:** video-late drop/adjust, measured reprojection
if supported, display vs decoder latency and P95/P99 network/pose
age, battery and thermal trend, adaptive bitrate/encode only with
explicit capability evidence. Continuous test Home <-> PCVR <-> Home,
phone suspend, tether/Wi-Fi unplug, driver disable, SteamVR exit,
SteamVR update, USB interference, other installed HMDs, 15-minute
and one-hour S9 runs. No device acceptance before physical proof.

## Diagnostic records and performance

Report per-stage success and explicit failure reason: SteamVR path,
driver registered/loaded, compatible encoder, pairing trust, RTT,
pose age, packets dropped/reordered, video encode milliseconds,
network travel/queue, decoder queue/decode milliseconds, app
submission cadence, whether a physical present timestamp is known,
and battery/thermal. Do not conflate Cardboard `onNewFrame` calls
with display present. The host's timing and phone's timing differ;
an end-to-end latency claim needs clock-synchronization uncertainty
or external display measurement. Preserve the existing opt-in VR
performance overlay and date the diagnostic snapshot.

## Security, dependencies and product invariants

No GitHub Actions unless asked. No destructive SteamVR repair,
no fake registration, no stubbed buttons, no independent Windows
ReverieVR game engine. One APK remains the optional host utility
distribution vehicle and a standalone phone-only runtime. Treat
SteamVR/OpenVR as user-installed host software; record pinned SDK and
permissive license prior to source integration, along with all DLL,
encoder and codec licenses. All deployment and test claims follow
INTENT -> EXECUTE -> OBSERVE -> VERIFY -> CLAIM.


## 2026-10-16 S1a transport implementation — REAL TLS handshake, not pose/VR

A functional **one-shot** PC↔phone authenticated transport test is
now source-wired for either USB or private Wi-Fi, with a fifth APK-bundled
PowerShell asset, `SteamVrLinkProbeHost.ps1`, a real Windows Laptop Link
menu option 6, Android's `SteamVrLinkProbe` and the existing Stage A
Laptop Link section's **Test SteamVR PC Link (USB / Wi-Fi)** button.
It is a testable end-to-end data flow, **not** an attached SteamVR device.

**USB/ADB reverse:** Windows option 6 → U binds `127.0.0.1:38726`
on the PC only; connect and authorize USB debugging; run on the PC:
`adb reverse tcp:38726 tcp:38726`. Then select USB on phone (always
`127.0.0.1`). ADB reverse points the phone's loopback TCP connection
to the host; it is not HDMI or a mandatory PCVR runtime. Remove the
temporary mapping with `adb reverse --remove tcp:38726` if desired.

**Wi-Fi/private IP:** Windows option 6 → W binds `0.0.0.0:38726`
*only with the user's explicit consent and for at most 180 seconds*.
The Galaxy S9 and PC must share a trusted LAN (PC Ethernet + Wi-Fi
is fine); the phone enters a private IPv4 address from PC `ipconfig`.
No public or hostname endpoints, no broadcast/auto discovery.
Windows firewall may need a deliberately constrained private-network
exception; the tool does **not** change firewall configuration.
**USB tethering without ADB** uses this same W/IP option: enable Android
USB tethering, identify the PC's tether interface IPv4, and enter it
on the phone (not the 127.0.0.1 ADB-reverse option). A cable alone is
not an automatically routed socket.

**Pairing and security:** The Windows helper generates a temporary
nonexportable server TLS certificate under the *current Windows user's*
cert store, prints its DER SHA-256 fingerprint and random 128-bit
single-use token, and removes only that generated certificate on exit.
Phone user enters the two values exactly. Android connects with TLS 1.2,
**checks the full SHA-256 certificate fingerprint against the entered
fingerprint** using a constant-time digest comparison, then transmits
the token **only inside validated TLS**. The PC returns exactly
`RVLINK/1 READY\n` when the version/token match; no other data or
controller/video/head-pose packets are sent. Maximum 5 attempts,
180 seconds of server availability, 5-second TCP connect and 7-second
read/write bounds. No pairing token, fingerprint or password is
persisted in Android preferences or written to normal logs. This is
explicit one-session pairing; there is no automatic trust-on-first-use,
silent fallback to plaintext, default PIN or insecure certificate bypass.
Manually entering 64+32 hex is acceptable for the first test only;
offline QR setup and appropriately verified per-device trust persistence
are future convenience work.

**What S1a actually proves after successful hardware testing:** a PC
TCP listener was reachable from the phone on the requested link,
Windows server TLS terminated, Android matched the out-of-band fingerprint,
the one-time token was accepted and a bounded READY response arrived.
It does *not* prove head pose, graphics/video decoding, SteamVR driver
loading, time synchronization, or latency suitability.

**Remaining S1b functional implementation:** keep the authenticated
session alive after handshake; use separate ordered reliable control
and latest-pose channels; connect real Cardboard orientation and bound
Daydream controller actions to the pose/sequence serialization; provide
Windows live-pose readback; handle reconnect, all-buttons-up,
session generation, timeout and automatic return to local shell;
never send unbounded pose queues. S2–S5 are unchanged.

**Build/device gate:** Windows PowerShell 5.1 on actual PC must validate
the self-signed certificate provision/cleanup and TLS accept under U/W;
Android build and JUnit tests for validation/packaging; USB/ADB reverse,
Wi-Fi and USB tether IP tests on S9; expired/wrong fingerprint/token,
bad IP, blocked port, Windows firewall prompts, cancelled dialog,
backgrounding and no-token logging; ensure SteamVR and games are
unaffected. No Windows/Android/device handshake success has been
claimed from source-only GitHub edits.
