# ReverieVR Laptop Link — single-APK portable Windows bootstrap

**Status (2026-10-12):** first functional handoff implemented in Android source:
the existing ReverieVR APK includes the portable Windows utility scripts
and Stage A's **Export Laptop Link ZIP** control. It exports a validated,
bounded ZIP through Android's system Create Document flow. The Windows
utility is human-readable PowerShell/cmd and runs only when the owner
copies, extracts and explicitly starts it on the laptop. Full signed
Android build and Galaxy S9 + Windows execution still require acceptance.
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

**B. Seamless delivery:** improve delivery beyond manual ZIP copying when
the USB/network authorization model is proven. A user-approved local
download endpoint from the phone is feasible; auto-running Windows code
or silently changing firewall/USB settings is not. Retain exported ZIP
as a universal fallback.

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
