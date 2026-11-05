# Sitka PSX Material Drafts — Shared VR Library

**Status:** `draft/curation`. Not a shipping material pack; no runtime asset-catalog entry, APK inclusion, game renderer binding, or scene implementation is implied.

## Sources

- **Pack:** `shared-assets/staging/sitka_psx_materials_draft_v1.zip`
- **Source:** original AI-generated 7 × 4 material contact sheet, then 28 separate label-free 128 × 128 PNG crops.
- **Google Drive source archive:** https://drive.google.com/file/d/1aNy5hu4nK4Gyy8xKd75pFDgPmIL4GO8_/view
- **Original uncut contact sheet:** https://drive.google.com/file/d/1kk8p07sxnAM1cwLTYcsKUrlgigfIT0lq/view
- **Drive source folder:** https://drive.google.com/drive/folders/1V5-OnZ8im7j7u9o1w9DQk3ZnHCyb8Loi
- **Archive SHA-256:** `d7ec000a76615bcada6bf0d7c1356f485e897096bad06861d84dc7c91cea923b`
- **Original sheet SHA-256:** `4648e3266a1181e7f95152a9f112442e7cbe6dbe967c9d3ac61c8f8b0fffc2b5`
- **Git binary blob:** `6b063eb3e087d644fb7c94bc65bf0445d4e309a0`
- **Provenance:** generated in the Sitka/Daydream texture development conversation, 2026-11-04. Not copied from a Google Map, city photograph, or third-party game.

## Scope

28 separated swatches: 4 wood/siding surfaces, pilings, net/fence/railing, 2 concrete surfaces, asphalt and markings, gravel, coastal rocks, 4 metal structures/finishes, crate/pallet, 3 water surfaces, shore-break wave, beach sand, seaweed, and distant snowy rock. Exact per-file names and SHA-256 hashes live in `manifest.json` inside the ZIP.

The material sheets were concept proposals. The PNG swatches are **not guaranteed seamless**: validate edge matching, alpha and transparency needs, UV tiling, padding, and color before use. The `water_ocean_*` and `shore_break_wave` tiles are not substitutes for Sitka's real 3D ocean or animated water renderer; `mountain_snow_far` is not a substitute for real terrain. Favor small 64–128 px texture variants, nearest/bilinear sampling per asset needs, and existing shared VR catalog approvals.

## Usage and promotion

1. Fetch the ZIP and verify the SHA-256 against this record.
2. Check the per-file hashes in `manifest.json`. Do not treat crop names as source geometry.
3. Assess individual tiles for seams, intended orientation, alpha, and content fit. Fix and re-hash approved selections.
4. Promote **only approved individual textures** to the native shared material catalog; add explicit consumer scopes and run GLES2/Android and 30 FPS scene tests.
5. Never silently replace existing `native/shared/v1/materials/coastal/` production assets or duplicate the entire pack into each APK.

There is deliberately no change to a shipping material catalog or Android asset bundle in this staging commit.
