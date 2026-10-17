# Between Deliveries — private original Unreal export utility

**What this actually does:** `export_between_deliveries.py` is a functional Unreal Editor Python batch exporter for the 24 source product groups already discovered in `docs/project/BETWEEN_DELIVERIES_UNREAL_CATALOG.json`. It resolves the **actual** packages under `/Game/Models/Props/Booze/{product_folder}`, exports supported `Texture2D` objects as **original-size PNGs**, `StaticMesh` objects as **FBX**, and tries exporting the original `/Game/Data_tables/DT_ShopItems` as **CSV**. Every result gets SHA-256, byte count, source Unreal object, supplier/SKU, and (for PNG) actual pixel resolution in a private `export_manifest.json`. The PNG validator walks the full chunk chain, checks CRCs, requires IHDR/IDAT/IEND and rejects truncated files rather than trusting a valid-looking first 24 bytes. Missing assets and failures are explicit.

**It has not been executed in Unreal Editor**; the current environment has no Unreal installation or complete `.uproject`. Ordinary Python unit tests for safe filenames, 24-item catalog validation, PNG dimensions, bad FBX detection, and hashing have passed. The utility is *not* proof that any 4K textures or original GLB geometry were actually exported or loaded into ReverieVR.

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

- The **24 recovered product-folder associations were inferred** from original package and item names; exporting all actual assets inside each group avoids pretending a particular `StaticMesh` object is already verified. The DataTable CSV is required to establish final row-to-object and icon bindings.
- This first utility intentionally **does not export `Fab/Megascans`, `OldWall`, characters, fonts, music, or arbitrary third-party packs**. Their source origin, explicit distribution rights and sometimes the mixed UE 5.5 format must be separately cleared. Exporting into a private folder is not permission to publish.
- Source PNG and FBX are **masters for conversion**, not Android APK assets. Verify color space, alpha, UV/pivots, scale and actual texture resolution. Generate mobile-sized GPU variants and eventual .glb models *from* those masters. Do not downscale or replace originals, and do not use embedded 256px JPEG package thumbnails as original artwork.
- A source may export from FBX without material links if dependent Unreal shader graph or third-party plugins are missing; those are errors to resolve before renderer integration, not to disguise with placeholder material.
- After licensing and exporter readback, import a curated subset of verified assets with original SHA-256 provenance and a tested Android asset-loading path, update `docs/licensing/ASSET_REGISTRY.json` appropriately and test native Android compilation and Galaxy S9 two-eye rendering. The current direct-selection stockroom signs are intentional **temporary usable UI**, not a claim that original art is currently rendered.

Run the ordinary pure-Python safety tests (no UE required) with:

```sh
python -m unittest scripts/unreal/test_export_between_deliveries.py -v
```

The authoritative game scope remains `docs/project/BETWEEN_DELIVERIES_FULL_SCOPE.md` and the asset-recovery gate remains `docs/project/BETWEEN_DELIVERIES_ASSET_RECOVERY.md`.
