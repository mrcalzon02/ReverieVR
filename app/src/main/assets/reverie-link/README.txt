ReverieVR Laptop Link - portable Windows tools
==============================================
This bundle was exported FROM THE REVERIEVR ANDROID APK. It is a small,
optional, plain-text laptop utility, not another Android app.

How to start:
1. On the Galaxy S9, use "Export Laptop Link" in ReverieVR Stage A.
2. Copy the exported ZIP to a Windows computer (for example by MTP/USB,
   a chosen file-sharing destination, or another user-controlled transfer).
3. Extract the ZIP so Start-ReverieLink.cmd and ReverieLink.ps1 are together.
4. Inspect the scripts if desired; launch Start-ReverieLink.cmd yourself.
5. Choose a mode. To forward a laptop gamepad or mirror the phone, the
   computer must have the OFFICIAL scrcpy executable available, installed
   separately or on PATH: https://github.com/Genymobile/scrcpy

Modes 1 and 2 need normal authorized ADB over USB or an already configured
trusted ADB/TCP connection. Mode 3 enables scrcpy's Wi-Fi setup flow;
older Android builds normally need initial USB debugging authorization.
UHID controller forwarding may fail on old Android devices; keep local
Daydream/HID/gaze controller recovery available.

Mode 4 enumerates optical drives ONLY when chosen. It does not extract
raw game-disc sectors, copy or stream screen pixels, read PC processes,
change Windows firewall settings, install a background service or
change laptop resources. For Windows share imports, first configure
a folder using normal Windows file sharing, then connect using the
read-only SMB client already in ReverieVR.

Only the user may initiate/run this Windows helper. There is no
silent Windows execution or USB autorun and there is no automatic
desktop-screen capture. The headset continues to work without this
utility, without a router, and without internet access.

This is the host-bootstrap milestone, not yet a raw optical-disc
transport or a PC-desktop-to-headset video streamer.
