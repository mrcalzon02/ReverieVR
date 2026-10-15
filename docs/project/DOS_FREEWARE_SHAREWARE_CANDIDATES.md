# Limited-control DOS games: distribution and install status

ReverieVR is a single phone-resident APK. Do not show a DOS module as
installed/playable until its actual archive, direct launch and input binding
are supplied. Downloadable != freely redistributable; freeware != open source.

| Game | Current state | Control fit and source |
|---|---|---|
| DOOM Shareware v1.9 | Already implemented as included shareware for free builds | Head yaw mouse, pad directions, click/App mouse buttons, volume rocker for weapons; original verified doom19s.zip, ADR-0014 |
| Ken's Labyrinth v2.1 | Integrated source/bundling path for free builds; **real release and phone test still pending** | Arrows on pad; click Ctrl/fire; minus/App Space/use; volume +/- Enter/Escape. https://advsys.net/ken/klab.htm |
| One Must Fall 2097 | Candidate only — not bundled | Joystick-compatible fighter; needs verified untouched ZIP, unattended startup and control test. Author permission discussion https://forums.unrealengine.com/t/who-owns-the-rights-to-one-must-fall-2097-and-is-it-freeware/405924 |
| Tyrian 2000 | Candidate only — not bundled | Four directions and fire; freeware game-data terms and original DOS ZIP digest still need checking. https://github.com/opentyrian/opentyrian/wiki/FAQ |
| Raptor: Call of the Shadows (episode 1) | Candidate only — not bundled | Direction/aim plus primary/special fire; original Apogee shareware rights, archive hash, and direct-play startup not yet established. No full-game data. |
| Jetpack 1.5 | **Source integration + packaging added; signed APK and headset launch acceptance pending** | Adept author-hosted `jetpak15.zip`, 494745 bytes, SHA-256 `03f4979149bdeeb43091517e0f33f92b14cc2e5d7531e0c524749c3383fc9d4c`; pad movement, click held Alt thrust/jump, minus/App Ctrl phase. |
| Squarez Deluxe | **Source integration + packaging added; signed APK and headset launch acceptance pending** | Shares the same original Jetpack ZIP, but uses its own `SQUAREZ.EXE` launch, binding profile, and saves; pad arrows, click Ctrl rotate, minus/App Alt place. |
| Bio Menace | Candidate only — not bundled | Directions, fire, jump; historical Apogee freeware release; still need verified original ZIP and redistribution scope. |

No false playable shelf entries and no downloading installers on the user's
phone during play. Home/O is reserved for VR Quick Menu, with separate
minus/App and volume +/− rocker controls. In head-as-mouse profiles, head
orientation provides mouse XY while the touchpad can emit four digital keys.

## Library capacity and integration contract (2026-10-14)

The DOS library is a dynamically enumerated app-private module repository,
not a three-game roster. **The menu is now a vertically scrollable list,
not a page-tab system.** Five selectable game rows remain visible in a
fixed viewport, with Import and Back fixed below, plus a proportional scroll
indicator. Daydream touchpad vertical swipes advance the list by individual
rows while a finger moves; swipes do not also fire NAV commands on release.
Up/Down navigation and a USB/Bluetooth mouse wheel scroll the shelf as
fallbacks. The O/Home quick menu and in-game digital key bindings are
unchanged. `DosLibraryScroll` is shared by drawing, enabled hit targets
and selection; pure JVM tests cover 0, 1, 3, 5, 7, 51, 100 and 301
games plus integer boundary behavior. Library persistence tests populate
51 separate installed-game directories, reopen the repository and verify
independent bindings after a profile change. Controller-swipe tests verify
that library swipes don't accidentally trigger navigation at finger lift,
while other menus keep normal swipe navigation. Real Galaxy S9 + Daydream
shelf readability, scroll comfort, and selection remain a device gate.

Bundled DOS content is a separate question from shelf capacity. Legacy
DOOM retains its proven split-installer preparation. Standard complete ZIP
games use `BundledDosArchiveCatalog` descriptors: unique module ID, title,
verified unchanged archive path and SHA-256, executable path/name, extraction
bounds, profile, release packaging flag, and local installation marker.
`BundledDosContentInstaller` iterates packaged descriptors, continues
installing other titles when one fails with an explicit error log, and
registers a game **only after verified extraction and executable discovery**.
It preserves each game's independent saved binding metadata when damaged
content must be reinstalled. Adding another title still requires verified
original rights, its release fetch/packaging setup, declared digest, and
appropriate control preset; merely adding a title to this candidate document
does not create a playable game.

The existing Android importer accepts game files up to **8 GiB each**, but
there is no promise of limitless phone storage. Bundled ZIP extraction has
per-title limits, and the phone needs enough usable space for both the
unchanged original archive and extracted runtime/save data. The signed
ReverieVR phone-test APK was **94,235,418 bytes** on the last checked release
(`phone-test-184-1`), before these capacity-source changes; projected
future APK growth must be measured, not assumed. A noncommercial freeware
archive can be excluded from different distribution variants without
fabricating a download-on-play requirement. Offline local games remain the
default; no laptop or external app is required.
