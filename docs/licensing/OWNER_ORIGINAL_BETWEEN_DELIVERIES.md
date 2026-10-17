# Owner-authored Between Deliveries assets admitted to ReverieVR

The project owner confirmed directly on 2026-10-16 that the **Between
Deliveries** Unreal source assets are theirs and explicitly authorized
extraction, adaptation and inclusion in ReverieVR / Red Ledger. They are
**not** automatically sublicensed as MIT, CC-BY, CC0, or public-domain works.
License identifier: `LicenseRef-Owner-Original-Assets`.

## Source and fidelity

Original Unreal UE5.7 packages are retained byte-for-byte under
`art/source/between_deliveries/Content` with their source object names and
Git blob digests in `SOURCE_ASSETS.json`. A compatible UE5.7 editor, not
embedded package preview JPEGs, exported the actual 24 associated 300×300
RGBA PNG icons and `DT_ShopItems.csv`. The exported files' SHA-256 digests
are recorded individually in `docs/licensing/ASSET_REGISTRY.json` and in
the private original `export_manifest.json`.

Original music, sound effects, and the 24 original FBX meshes/1024px texture
PNGs remain master exports in the owner's Drive export folder. Importing
these into the native runtime is separate future work; this record does not
claim all exported sound or geometry was added to the APK.

## Runtime use

The 24 original RGBA icons are packaged under
`app/src/main/assets/native/red-ledger/icons`. Red Ledger's native
stockroom uses the optional, read-only host APK asset service to upload
them as 300×300 GPU textures when creating its GLES2 context and draws them
on the *existing physical selectable product cards*. Original icon
1 belongs to the first faction's draft, 2 to bottled beer, 3 to spirits,
4 to snacks; the 6×4 stockroom layout maps source slot
`faction * 4 + product_group` back to these original table slots. The
product label remains visible below each picture and controller targets
remain unchanged. The original exported shop CSV is packaged for
reconciliation without replacing the existing native trade simulation.

Native source integration and Git readback can be verified remotely,
but an actual Android APK build and two-eye Galaxy S9 headset run must
be confirmed separately. Missing or malformed PNGs leave the text-only
card usable rather than silently substituting editor thumbnails.

## Real SoundWave integration (2026-10-16)

Six selected **unchanged** Unreal Editor-exported PCM 16-bit stereo WAV files are now Android raw resources: Single_click, Hollow_metal, Coins, Book_flip, Door_close, Drawer. Source manifest digests and per-file REUSE/ASSET_REGISTRY licensing records trace each file to the owner's exports. Native Red Ledger emits six distinct ordered cues (IDs 22–27) only for completed interactions. The Android shell has a reusable SoundPool bank (maximum four concurrent streams) that preloads these effects once and routes cue IDs without per-frame audio synthesis, preserving existing Breakwater/other native cue mappings. Full music masters, long dialogue/music, and remaining 21 original short effects are not indiscriminately added to the APK. Android compilation and Galaxy S9 audio acceptance still require runtime verification.
