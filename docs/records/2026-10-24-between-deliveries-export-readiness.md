# Between Deliveries — export readiness checkpoint (2026-10-24)

This is a read-only Google Drive inventory checkpoint, **not** evidence of APK integration or complete upload integrity. Authority: `docs/project/BETWEEN_DELIVERIES_FULL_SCOPE.md` and `docs/project/BETWEEN_DELIVERIES_ASSET_RECOVERY.md`.

The source folder `Between deliveries` exposes `Content` and `BetweenDeliveries_UnrealExports`. The export folder contains `icons`, `products`, `source_tables`, `audio`, `audio_manifest.json` (21,324 bytes), and `export_manifest.json` (33,830 bytes). Direct folder listing confirms **24 PNG icons**, six supplier directories (`101`, `202`, `303`, `404`, `505`, `606`), one `DT_ShopItems.csv` (6,337 bytes), and separate `audio/ui` and `audio/music` directories. These are metadata listings only; nested file bytes, completeness, SHA-256, and present-day audio export totals were **not** independently revalidated in this checkpoint.

Existing recovery documentation records 73 successful primary exports and 40 WAV audio exports with one filename failure at that earlier point. Reconcile the actual current manifests and nested files before claiming the missing track is fixed. Never infer complete upload merely from a folder appearing in Drive.

## Next bounded production slice

1. Read both manifests and enumerate every nested export. Validate filename, byte length, expected category count, and SHA-256 against each source where available; classify missing/mismatched assets as pending, not deleted.
2. Compare exports to `docs/licensing/ASSET_REGISTRY.json` and the current packaged native assets; exclude Fab/Megascans or any other third-party materials without license clearance.
3. Preserve original 1024px product maps, 300px icons and 4096px wall masters. Produce separate mobile derivatives with documented resolution/compression decisions; never substitute Unreal embedded 256px previews for masters.
4. Extend the existing 101draft FBX/GLES2 pilot incrementally to other source models, using the established loader and actual Android performance constraints. Do not commit `.uasset` packages as runnable Android resources.
5. Verify a native build, model visibility, icon legibility, audio routing and controls on the target Galaxy S9 before calling the asset set integrated or shipped.

No GitHub Actions requested. No Google Drive files were modified during this checkpoint.
