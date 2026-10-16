# ReverieVR Laptop Link — single-APK portable Windows bootstrap

**Status (2026-10-12):** first functional handoff implemented in Android source:
the existing ReverieVR APK includes the portable Windows utility scripts,
Stage A's **Export Laptop Link ZIP** action, and **Share Laptop Link over
USB / Wi-Fi (5 min)**. The latter advertises a one-use, 160-bit random,
five-minute URL from an in-app temporary HTTP server. It supports private
hotspot/Wi-Fi IP access and wired, authorized ADB port forwarding; the
user explicitly downloads, extracts and starts the helper on Windows.
No standalone Windows download, service or separate Android package is
required. Full signed Android build, Galaxy S9 Wi-Fi/ADB round-trip and
Windows execution still require acceptance.
It does **not** yet expose raw optical-disc sectors or PC desktop pixels.

## Owner's architecture

- One ReverieVR APK runs stereo VR, games, local emulation hosts, media, shell,
  controller routing and head-tracked gaze recovery, without a PC.
- A laptop may be arbitrarily busy with Discord/TeamSpeak, music,
  transcoding, downloads or other unrelated operations. Nothing in
  ReverieVR enumerates or interferes with those processes.
- The owner **authorizes a tiny optional desktop-side link helper only when
  it is distributed as content from the single ReverieVR APK**. Do not
  create a separate required app distribution/download or Windows installer.
  No Windows autorun, elevated installation, background service or
  privilege-bypass is implied or allowed.
- Windows must explicitly save and run the helper. A normal Android APK
  has no authorization to execute laptop code over USB, MTP or a hotspot
  connection. Subsequent automatic reconnection may be developed only
  once a user-launched, authenticated host program is running.

## Current functional export flow

1. Open **ReverieVR Stage A → Laptop Link → Export Laptop Link ZIP**.
2. Android Create Document prompts for the ZIP destination; save to a
   location you can transfer to the laptop, for example Downloads.
3. Copy `ReverieVR-Laptop-Link.zip` via USB file transfer, Windows
   File Explorer, existing local-network share, or another user-approved
   transfer method. The app does not silently push executable scripts to
   other computers.
4. Extract it, review contents, and deliberately run
   `ReverieLink/Start-ReverieLink.cmd` on Windows.
5. The portable PowerShell 5.1 menu offers:
   - control-only gamepad forwarding: installed
     `scrcpy --no-video --no-audio --gamepad=uhid`;
   - regular phone-to-PC mirror + controller:
     `scrcpy --stay-awake --no-audio --gamepad=uhid`;
   - existing scrcpy Wi-Fi setup (older Android requires initial USB/ADB
     authorization): `scrcpy --tcpip --no-audio --gamepad=uhid`;
   - read-only enumeration of local optical drives and instructions to
     share lawful disc *files/images* via Windows SMB;
   - Quit.
6. If scrcpy is not installed, the menu asks the user to locate the
   official `scrcpy.exe`; it does not install software or fetch unknown
   binaries silently. An input-only session is separate from screen
   mirroring; neither is required for running the headset.

The ZIP contains only three allowlisted script/readme assets:
`README.txt`, `Start-ReverieLink.cmd`, `ReverieLink.ps1`. The APK
produces a bounded ZIP on demand from its own bundled assets, without a
second APK. It writes via the Android system document URI, off the UI
thread, and attempts to remove partial output on failure. Export is
explicit and does not upload desktop or phone data.

## Direct laptop pull from the installed phone APK

An additional Stage A button now starts a tiny temporary download endpoint.
It serves **only** the existing bounded, three-file ZIP from packaged APK
assets; it does not browse the phone, expose user media, forward inputs,
execute code, or automatically install anything on Windows.

- **Wi-Fi / laptop hotspot:** put laptop and phone on the same **trusted
  private** local network; tap **Share Laptop Link over USB / Wi-Fi (5 min)**,
  then open the displayed unique `http://PHONE_IPV4:PORT/reverie-link/TOKEN`
  address in the laptop's browser. There is no Wi-Fi scan or internet
  dependency. The displayed SHA-256 can be compared with
  `Get-FileHash .\\ReverieVR-Laptop-Link.zip -Algorithm SHA256`
  before extracting or running executable scripts.
- **USB / pre-authorized ADB:** connect/authorize USB debugging; the phone
  displays `adb forward tcp:PORT tcp:PORT`. Run this command from the PC
  with Android platform-tools installed (usually alongside scrcpy), then
  browse the phone-displayed `http://127.0.0.1:PORT/reverie-link/TOKEN`
  URL on that same computer. The forwarding direction is **PC → Android**.
  Do not use `adb reverse` for this endpoint.
- **Security:** transport is normal HTTP, **not TLS**. The URL is a
  cryptographically random one-time bearer secret; only a trusted
  isolated network or USB/ADB forwarding is appropriate. Public Wi-Fi
  and untrusted intermediaries are unsuitable for distributing executable
  scripts. The server closes after one completed transfer, five minutes,
  twenty-four invalid requests, Activity backgrounding, or explicit Stop.
  The URL is never written to ReverieVR diagnostics/logs. It does not
  install or launch the helper by itself. The user must verify/copy,
  extract and explicitly run the portable Windows launcher.
- **Recovery:** the original Android Create Document ZIP export stays
  available even if neither TCP/IP nor ADB debugging is working. Nothing
  in phone-only VR depends on this endpoint. It is active only while
  Stage A is visible with the share dialog open.

The pure-JVM `LaptopLinkDownloadServerTest` exercises one-time HTTP ZIP
delivery, token rejection and explicit cancellation; headset/Windows
proof remains separate.

## SteamVR PC-headset endpoint (new, SteamVR only)

Windows Laptop Link first added a fourth allowlisted plain-text file,
`SteamVrPreflight.ps1`. Its existing manual launcher offers menu option
5: it **reads** OpenVR per-user paths, verifies the SteamVR runtime and
Valve `vrpathreg.exe`, and lists registered external drivers. This is a
real prerequisite check, not a fake SteamVR headset or working video
stream. The previous 3-file historical milestone was superseded by four members at S0. At S1a, the export now contains five members including SteamVrLinkProbeHost.ps1; the JVM bundle test expects all five.

User has explicitly chosen *SteamVR games on the PC* instead of moving
ReverieVR's Home/DOS/native-game runtime to Windows. The future Windows
SteamVR HMD driver and video host will be optional components delivered
from a single APK export, installed with explicit consent, and reversible.
The independent S1–S5 implementation/install/protocol spec is at
`docs/project/STEAMVR-LINK-IMPLEMENTATION.md`.

### USB / Wi-Fi SteamVR host connection test (S1a)

The **Test SteamVR PC Link (USB / Wi-Fi)** button in Stage A performs a
real connection attempt. Windows Laptop Link option 6 runs a one-session
TLS 1.2 listener. USB choice binds loopback and requires the explicit
`adb reverse tcp:38726 tcp:38726` command from the authorized PC;
Wi-Fi/USB tether choice binds port 38726 to a user-consented private
LAN interface (Windows firewall may need a limited manual exception).
Windows prints a new SHA-256 certificate fingerprint and 32-character
hex token; enter both on the Galaxy S9. Android pins certificate bytes,
sends the token through TLS, and only reports success on bounded READY.
The temporary Windows certificate is removed when the listener stops;
no Windows driver/SteamVR registration or streaming is performed here.
No runtime handshake has yet been verified on Windows or S9.

## Next functional slices

**A. Real optical-disc data:** inspect Windows' read-only optical device
interfaces and identify an implementation capable of CD raw sector
access when required by PS1 BIN/CUE and PS2 optical formats. A normal
SMB share of a drive is not equivalent to sector-mode emulation or a
BIOS-authorized game dump. Host transport must authenticate its phone
peer, allow only user-selected media, refuse writes, provide bounded
reads and timeouts, and avoid root/admin or arbitrary filesystem access.
Prefer a compact signed portable host executable built and carried in the
APK only once code and legal requirements are verified.

**B. Seamless delivery:** source-implemented temporary, consented
download endpoint for an already authorized USB/ADB forwarding connection
or trusted private Wi-Fi/hotspot. Retain Create Document ZIP export as
universal fallback. Device acceptance must prove actual network transfer
and uninterrupted Windows archive extraction; no silent Windows execution
or firewall modification is allowed.

**C. PC-to-phone screen panel:** separate from scrcpy's phone-to-PC mirror
and the local Android MediaProjection panel. A Windows source must be
explicitly started and consented; phone-side reception uses low-overhead
bounded decoder work. Screen content may never be OCR'd or analyzed, and
the existing no-phone-pixels-upload privacy boundary remains intact.

## Verification

- JVM `LaptopLinkBundleTest` checks only the three fixed export members
  and rejects missing/oversize bundle resources.
- Android build must prove the three packaged assets, the new Stage A
  button and its URI export callback compile. Android document picker
  success/failure and ZIP extraction on real phone require device proof.
- On Windows, verify all menu modes, missing scrcpy, power-shell script
  policy diagnostics and optical-drive absence/presence without admin
  installation. Existing official scrcpy UHID may fail on older Android;
  never claim universal Galaxy S9 support without an S9 test.
- Keep controller input, screen streaming and future optical transfers
  independently cancellable. Laptop loss must return to local Daydream/HID
  or five-second gaze selection. Measure headset frame pacing and latency
  separately from host processing load.

See `docs/project/PHONE_SCREEN_PANEL.md`, `docs/project/LAPTOP_SMB_IMPORT.md`
and `docs/records/ADR-0023-OPTIONAL-PC-INTEROPERABILITY-AND-RESOURCE-ISOLATION.md`.
