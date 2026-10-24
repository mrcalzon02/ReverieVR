# Between Deliveries — manifest reconciliation (2026-10-24)

Read the two existing original export manifests directly, without modifying Drive. This checkpoint supersedes any implication that folder counts alone establish binary integrity.

- `export_manifest.json`: 73 records, 73 reported validated, **zero recorded failures**; 24 PNG icons, 24 FBX meshes, 24 PNG textures and one shop CSV. Sum of declared exported sizes: **58,380,604 bytes**.
- `audio_manifest.json`: 40 WAV records, **one recorded failure**: `People’s_Front_Rising` rejected as an unsafe SoundWave name. Sum of declared exported sizes: **520,149,148 bytes**. This failure is not resolved merely by a later exporter-code fix.
- Both manifest status fields explicitly warn that native runtime integration remains unverified.

These counts are from the manifests, not independent byte-by-byte download verification. The Drive export root and immediate `icons`, `products`, `source_tables`, `audio` children were listed separately; no source files were changed.

## Production gate

Run a non-destructive recursive listing and verify each listed export's actual bytes and SHA-256 against the manifest. Investigate the single failed music export via the corrected exporter without re-exporting or replacing unrelated material. Only then select incremental native imports and run host/Android tests. Preserve master PNGs and WAVs outside size-constrained APK packaging; use curated mobile derivatives and documented licensing. No Unreal `.uasset` should be treated as a portable Android runtime asset.

See `docs/project/BETWEEN_DELIVERIES_FULL_SCOPE.md` and `docs/project/BETWEEN_DELIVERIES_ASSET_RECOVERY.md` for the gameplay and existing asset-bridge authorities.
