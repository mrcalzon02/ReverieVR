# Between Deliveries — private original Unreal export utility

**What this actually does:** `export_between_deliveries.py` is a functional Unreal Editor Python batch exporter for the 24 source product groups already discovered in `docs/project/BETWEEN_DELIVERIES_UNREAL_CATALOG.json`. It resolves the **actual** packages under `/Game/Models/Props/Booze/{product_folder}`, exports supported `Texture2D` objects as **original-size PNGs**, `StaticMesh` objects as **FBX**, and tries exporting the original `/Game/Data_tables/DT_ShopItems` as **CSV**. Every result gets SHA-256, byte count, source Unreal object, supplier/SKU, and (for PNG) actual pixel resolution in a private `export_manifest.json`. The PNG validator walks the full chunk chain, checks CRCs, requires IHDR/IDAT/IEND and rejects truncated files rather than trusting a valid-looking first 24 bytes. Missing assets and failures are explicit.

**It has not been executed in Unreal Editor**; the current environment has no Unreal installation or complete `.uproject`. Ordinary Python unit tests for safe filenames, 24-item catalog validation, PNG dimensions, bad FBX detection, and hashing have passed. The utility is *not* proof that any 4K textures or original GLB geometry were actually exported or loaded into ReverieVR.

## Original DataTable recovery WITHOUT the Unreal Editor

The user's original `DT_ShopItems.uasset` is a valid 17,513-byte UE5.7 DataTable. We recovered its **actual** tagged rows and imported UObject path references without opening Unreal. For independent reproduction on the original private file:

```sh
python scripts/unreal/recover_shop_table.py /private/DT_ShopItems.uasset /private/shop-metadata.json
```

The decoder requires SHA-256 `06440a1dafa96234140a585185ec3d5159d4e3fd59a8695a5121e8ba3824e62c` and fails closed on any different/truncated package. On the original download, it recovered **24 unique mesh+icon references**, buy/sale pricing and the discarded `test` row; it stores no source-image bytes or uploaded binary assets. **This does not export mesh geometry, 1K/4K textures, music, or prove rights.** Editor extraction still follows below.

## Requirements and use

1. In a private Windows Unreal project, restore the original `Between deliveries/Content` directory under an actual UE 5.7 `.uproject`. Original source packages include both **UE 5.5 and 5.7**; use a 5.7-capable editor and confirm imports/material dependencies. The original shop row struct and plugins may require restoration; failing packages are logged. Enable the standard **Python Editor Script Plugin**. Never modify the Google Drive originals while they are being uploaded.
2. Clone the current `ReverieVR/main` repository (so the script can read `docs/project/BETWEEN_DELIVERIES_UNREAL_CATALOG.json`). Use the canonical source file `scripts/unreal/export_between_deliveries.py` from the checkout.
3. Set `BETW_DELIVERIES_EXPORT_DIR` to a **separate private output directory**, preferably on a volume with free space for dozens of full-res files. Do **not** set it inside your public Git checkout; the script refuses that. On Windows Command Prompt:

```bat
set BETW_DELIVERIES_EXPORT_DIR=D:\BetweenDeliveries_Original_Exports
"C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe" "D:\Projects\BetweenDeliveries\BetweenDeliveries.uproject" -ExecutePythonScript="D:\Repos\ReverieVR\scripts\unreal\export_between_deliveries.py"
```

Paths shown are **examples**, not confirmed locations on the user's machine. The editor reports failures in the log; the private output's `export_manifest.json` lists all exports and their source objects. Existing target files are never overwritten. Failure to find the row struct, a model, an exporter, or original pixels is reported explicitly.

## Constraints and next integration

- The **24 original product-to-mesh and product-to-icon associations are now verified** from the original binary DataTable, not inferred: `scripts/unreal/recover_shop_table.py` decodes the source FProperty tags and resolves each reference through the Unreal import table. All 24 original buy/sale prices, categories, supplier IDs and names are in `docs/project/BETWEEN_DELIVERIES_UNREAL_CATALOG.json`. The CSV export remains useful for editable authoring and price revision, but is **no longer required to identify the original asset links**. Exported PNG/FBX still requires UE and licensing.
- This first utility intentionally **does not export `Fab/Megascans`, `OldWall`, characters, fonts, music, or arbitrary third-party packs**. Their source origin, explicit distribution rights and sometimes the mixed UE 5.5 format must be separately cleared. Exporting into a private folder is not permission to publish.
- Source PNG and FBX are **masters for conversion**, not Android APK assets. Verify color space, alpha, UV/pivots, scale and actual texture resolution. Generate mobile-sized GPU variants and eventual .glb models *from* those masters. Do not downscale or replace originals, and do not use embedded 256px JPEG package thumbnails as original artwork.
- A source may export from FBX without material links if dependent Unreal shader graph or third-party plugins are missing; those are errors to resolve before renderer integration, not to disguise with placeholder material.
- After licensing and exporter readback, import a curated subset of verified assets with original SHA-256 provenance and a tested Android asset-loading path, update `docs/licensing/ASSET_REGISTRY.json` appropriately and test native Android compilation and Galaxy S9 two-eye rendering. The current direct-selection stockroom signs are intentional **temporary usable UI**, not a claim that original art is currently rendered.

Run the ordinary pure-Python safety tests (no UE required) with:

```sh
python -m unittest scripts/unreal/test_export_between_deliveries.py -v
```

The authoritative game scope remains `docs/project/BETWEEN_DELIVERIES_FULL_SCOPE.md` and the asset-recovery gate remains `docs/project/BETWEEN_DELIVERIES_ASSET_RECOVERY.md`.


## Private Unreal SoundWave export (original music and sound effects)

The original `Content/Sounds/Music` and `Content/Sounds/Ui` packages are not audio files that Android can directly play. After restoring the full UE 5.7 project and confirming that the assets load, use the dedicated **read-only, private** SoundWave exporter:

```bat
set BETW_DELIVERIES_EXPORT_DIR=D:\BetweenDeliveries_Original_Exports
"C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe" "D:\Projects\BetweenDeliveries\BetweenDeliveries.uproject" -ExecutePythonScript="D:\Repos\ReverieVR\scripts\unreal\export_between_deliveries_audio.py"
```

The exporter enumerates actual `/Game/Sounds/Music` and `/Game/Sounds/Ui` objects, exporting only `SoundWave` instances with Unreal's built-in `SoundExporterWAV`. It never overwrites existing exports, never modifies source packages, rejects unsafe names and incomplete/unsupported PCM WAVs, and writes an atomic private `audio_manifest.json` recording the Unreal object, category, size, SHA-256, sample rate, channels, bit depth, duration, export status and failures. Audio bytes remain entirely outside Git. Missing folders, unloaded objects and failed/corrupt audio are explicit failures; a successful CPython unit test is **not** proof that Unreal Editor can load all the original files.

Run the five isolated, Unreal-free regression tests with:

```sh
python -m unittest discover -s scripts/unreal -p 'test_export_between_deliveries_audio.py' -v
```

**Distribution gate:** the original Unreal music/effects remain **unapproved for publication or APK inclusion** until their individual provenance/rights and actual editor exports are verified. Successful private extraction is not a license grant. Review each WAV, then author controlled, size-optimized Android derivatives while retaining the private lossless exports and digests. The owner-approved `The Last Note Hangs Unresolved.mp3` already has its own packaged module cue and is unaffected by this exporter. The audio export utility does not install or start a second playback system, and neither Unreal execution nor headset playback has been claimed.

## Owner authorization and original source admission (2026-10-16)

The owner has confirmed that the Between Deliveries assets are theirs and approved for inclusion. The original canonical 24 icon source packages, all seven DataTables and the Barfront/stool source samples are now tracked byte-for-byte under `art/source/between_deliveries/Content`, with per-file blob hashes in `SOURCE_ASSETS.json`. Source admission is not the same as editor conversion or game-runtime use. `export_between_deliveries.py` now also exports the 24 icon Texture2Ds as full-size 300×300 PNGs (never 256px preview thumbnails); the SoundWave exporter now records owner-confirmed rights. Once a compatible Unreal 5.7 editor performs these exports, the portable assets can be optimized for the S9 and wired into the game. No actual PNG/FBX/WAV export or headset playback has occurred here.
