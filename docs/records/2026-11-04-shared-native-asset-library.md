# ADR — Immutable cross-game asset sources inside ReverieVR (2026-11-04)

**Intent:** The same original textures and curated models should be reused deliberately between native games without copies in the one-APK distribution and without fragile per-repository download dependencies.

**Decision:** Move six Breakwater 64px authored coastal textures and three generic converted BWM1 props under `native/shared/v1/` with the exact original Git blobs; keep only the owner-specific boat/APC files private. Breakwater's native authored-atlas and prop lookup now consume shared paths, retaining old scoped paths as *lookup fallbacks* for earlier verified packs, not duplicate APK assets. Omega Front's tactical concrete and industrial-steel slots reuse the same shared textures; remove its replaced concrete/steel PNGs (14,834 bytes total uncompressed). `reverie_native_shared_asset_catalog.h` grants only exact known paths to explicitly named built-in native modules. The existing host asset reader and GLES2 shader paths remain unchanged. Ship a source manifest and deterministic verification script for cross-repo import.

**Provenance:** six owner-directed generated materials; three compact BWM1 derivatives from the previously integrated owner-authorized Beckenburg/PSX Mega Pack. This does not declare purchased originals freely redistributable. Asset formats/rights must be vetted by each new consumer. No unrelated source archives or large model libraries added.

**Boundary:** one APK contains one shared copy; there is no OS-wide install-once cache across independent Android packages, no remote fetch at launch, no new module ABI, no extra game control, and no claim of measured on-device battery/render benefit. Source refs in other repos are version pins, not runtime delivery services.

**Validation gate:** shared 9-file hash/format validator, Breakwater exact PNG/BWM parser tests, native scope allowlist tests, Omega existing texture loading/CI, Android 10 emulator, APK package audit and Galaxy S9 stereo/thermal proof. Existing nonshared BWM1 owner boat/APC remains private. The result is source-integrated only until actual suite and headset evidence.
