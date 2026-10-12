# Laptop Files — Windows SMB2/SMB3 import inside the phone APK

**Status (2026-10-12):** Android source implementation for authenticated read-only Windows SMB share browsing and import into the existing DOS library. **Signed full Android build, Windows-share round trip, and Galaxy S9 physical acceptance remain pending.** The PlayStation/Libretro source catalog remains non-playable; importing an image does not make an unsupported emulator playable.

## How it works

No desktop ReverieVR companion application is needed. The Windows laptop uses its **built-in SMB file sharing**; the Galaxy S9 runs the new SMB client inside the existing ReverieVR APK. An ordinary Wi-Fi network, laptop hotspot, or another routed local network can connect the two. This does **not** require Android 11 wireless debugging, and it cannot turn the PC into a USB hub.

1. On Windows, explicitly share a folder containing **files**, such as DOS ZIP, executable or supported CD-image files, with an account that has at least read permission. A drive letter by itself is not a share. On a laptop hotspot, confirm Windows has enabled file sharing for the appropriate network profile and TCP port 445 is allowed on that trusted connection. Never expose SMB port 445 to the public internet.
2. On Galaxy S9, connect to the same local network or the laptop's hotspot through Android Wi-Fi Settings. In ReverieVR select **DOS Library → Import**, return to Stage A, then choose **Laptop files (Windows SMB2/SMB3)**.
3. Supply a literal Windows host name or IPv4 address (example: `192.168.137.1`), **share name** rather than a filesystem path, Windows username, and password. ReverieVR uses explicit authentication; it does not browse the whole network or request location services.
4. The Android app reads directory entries asynchronously, shows only directories and known DOS content extensions, lets the user open folders and go to a parent directory, and confirms each individual file import. The import is copied **to app-private storage on the phone**. Later DOS playback is fully phone-local, without the laptop.
5. Large files and failed sessions are bounded: a maximum of 200 supported entries per directory, a maximum of 8 GiB per imported file, 10-second SMB socket/request timeouts, interrupted-transfer cleanup, and SHA-256 calculation on the local copy. Existing DOS metadata is saved only when the copy is complete. Unsupported types remain unavailable.

## Security and resource contract

Only the explicitly named Windows share is connected, using authenticated SMB2/SMB3 on TCP 445 with **signing required**. All requests are read-only: list and open/read. No write/delete/rename or broad computer inspection. The password is not saved to preferences, source files or diagnostic logs; its in-memory copy is cleared when the import session closes. DNS host names and file names are validated to reject traversal and arbitrary UNC/path input. Standard Android Internet permission already exists; no new Bluetooth, location, Wi-Fi scan or broad filesystem permission is added. For Windows local accounts, enter the relevant account name; authentication may require adjusting account policies, which is a user/Windows configuration decision, not a background system change by ReverieVR.

A CD/DVD optical drive is **not** automatically a virtual block device on Android. Windows SMB shares directories/files, not raw optical sectors; multi-track PS1/PS2 physical discs may need independently produced, lawful image files and accompanying tracks. Current DOS-module import copies a **single file**; BIN/CUE sets requiring multiple files and separate PS1/PS2 emulator execution are independent future requirements. The user should prefer self-contained supported DOS content (e.g. DOS ZIP) for the initial acceptance test.

SMB uses `com.hierynomus:smbj:0.15.0` (Apache-2.0, Maven Central), whose known runtime dependencies include `com.hierynomus:asn-one:0.6.0` (Apache-2.0), `net.engio:mbassador:1.3.2` (MIT), `org.slf4j:slf4j-api:2.0.18` (MIT), and `org.bouncycastle:bcprov-jdk18on:1.85.2` (Bouncy Castle's permissive license). Android app minSdk remains 26; Java compile level remains 17; no native ABI-specific SMB binary is bundled. Runtime library and transitive dependency increase may meaningfully grow the unshrunk APK (especially Bouncy Castle); measure the *actual* packaged sizes rather than assuming it is lightweight. SMB is invoked only on demand off the VR/GL thread; its session is closed when done. A future dependency replacement is acceptable if size/security/device testing warrants it, but removing SMBJ requires retaining SMB2/3 interoperability and signing protection, not reverting to SMB1.

## Acceptance gates

- A signed Android build resolves/compiles SMBJ and its complete dependencies at minSdk 26, Java 17; complete JVM path tests pass.
- On actual Windows + Galaxy S9: an explicitly shared folder lists; a DOS ZIP copies with matching SHA-256, launches from the phone's library afterward with Wi-Fi disabled; file import is unaffected by absent optical drive.
- Malformed share, wrong credentials, no Windows server, interrupted transfer, traversal attempt, a 200+ entry directory, failed disk write, and mid-import disconnection produce a clear failure and no partial admitted DOS module.
- Concurrent scrcpy gamepad/mirror and busy Windows file/video operations cannot block Android stereo rendering or disable gaze fallback. Validate with the actual headset and measure latency/frame-time and APK size.
- No upload of phone-screen pixels, no PC process enumeration, no host-side code or service installation. SMB clients and emulator execution stay in one APK.

See `docs/records/ADR-0023-OPTIONAL-PC-INTEROPERABILITY-AND-RESOURCE-ISOLATION.md` and `docs/project/EXECUTION_CONTRACT.md`.
