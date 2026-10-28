# Native Games gallery — 2026-10-28

Status: **source implemented; Android APK compilation, headset legibility and S9 input acceptance pending**.

The user supplied a seven-game PSX-style menu reference for the ReverieVR native game browser. The Stage B native-games shelf must use a framed, illustrated card gallery instead of the three plain-text rows previously titled "CLASSIC GAMES". The first seven featured ids correspond to Breakwater Battery, Between Deliveries / Red Ledger, Gumball Armada, Forward Detachment, Lantern Desk, Omega Front and the procedural Test Chamber. Remaining playable classics paginate afterward. Only descriptors actually returned by the native host after development-mode filtering become selectable cards; the reference never creates fake entries or bypasses admission.

Implementation: `NativeGamesGallery.java` draws original compact vector preview vignettes on the shell UI texture, with labels and hit rectangles shared exactly at 1024x768; the UI texture is generated only when dirty and is shared across both eyes. Six cards per page use page wrap-around. Download/Update remains a dedicated always-available action, routing to existing Stage A management. The shell's existing pointer focus, five-second gaze fallback, quick menu and actual native start/failure callbacks remain authoritative. Gallery previews are not game screenshots, actual game menus or proof of gameplay.

Validation needed: run Android Java compilation and existing runtime tests without adding workflows; verify launch, failure feedback, Back/Home, page controls and manager handoff, seated pointer/dwell selection, legibility and stereo comfort on a Galaxy S9. No APK, remote CI or device execution is claimed by this record.
