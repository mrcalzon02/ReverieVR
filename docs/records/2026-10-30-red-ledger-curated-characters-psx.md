# Red Ledger — curated Characters PSX import (2026-10-30)

Status: **six real, privately converted game-use assets built and validated; NOT yet shipped or headset-verified.** This record must not be used to claim a signed or released APK.

## Source and selection

Source: [Characters PSX by Elbolilloduro](https://elbolilloduro.itch.io/characters-psx). The current license authorizes commercial and noncommercial game use without attribution but forbids re-uploading the asset as an asset collection, even after modification. Keep raw FBX files and textures in the owner's private Drive archive. Do not commit raw FBX files or converted standalone meshes to public source control or an independent asset catalog.

Owner's private Drive upload: \`Characters_psx_02/Models/Rig/{Male,Female}\` with matching \`Characters_psx_02/Textures\`. The initial older \`Characters_psx_1.2/Characters_psx_02/Models\` folder was empty; the **newly uploaded copy** contains the actual mesh data. These are distinct Drive folders; do not mistake the older empty folder for missing assets.

Only six source pairs are selected. The selected meshes use the existing Red Ledger \`owned_patrons\` enum, \`ForPatron(PatronKind)\` mapping and GLES2 RLBP renderer. Each variant resolves to \`native/red-ledger/beckenburg/patron_<role>.rlprop\`.

| Role | Source FBX and same-named PNG | Triangles | Derived bytes |
| --- | --- | ---: | ---: |
| RailWorker | Character_Male_57 | 1,616 | 325,008 |
| NightClerk | Character_34_Female | 1,378 | 296,448 |
| MilitiaVeteran | Character_Male_47 | 1,650 | 329,088 |
| Mechanic | Character_Male_34 | 1,306 | 287,808 |
| DirectorateCourier | Character_45_Female | 1,382 | 296,928 |
| SalihorskHauler | Character_Male_50 | 1,616 | 325,008 |
| **Total** | **six selected actors, no unused source files** | **8,948** | **1,861,288** |

The Directorate courier was visually selected as \`Character_45_Female\` after rejecting \`Character_42_Female\` (unsuitable clothing) during the actual mesh preview. Do not revive the older \`patron_gopnick\` or \`patron_pink_dress\` rejected static meshes.

## Conversion and verification

Run with Python 3 and Pillow:

\`\`\`sh
python3 scripts/modules/convert-characters-psx.py \
  --input /path/to/private/selected-source-pairs \
  --output /path/to/private/red-ledger-selected
python3 scripts/modules/verify-characters-psx-selection.py \
  /path/to/private/red-ledger-selected
\`\`\`

The converter deliberately reads only six named FBX+PNG pairs, expecting FBX 7.5–7.9 binary and an indexed per-polygon-vertex UV layer. It triangulates the source polygon indices, rotates X-forward/Z-up into the game's Y-up/Z-forward coordinate system, scales each character to 1.68 m, preserves the UVs, reduces textures to 256×256 RGB565, adds restrained faction-tinted coloration and outputs the GLES2 native RLBP v1 format. Result: 8,948 triangles and six geometrically distinct, textured models.

The diagnostic verifier checks the exact six-role allowlist, nonempty geometry, triangle/byte bounds, UV and vertex validity, per-file SHA-256 against the generated private selection manifest, and nonempty texture variation. It does **not** represent a Galaxy S9 runtime validation or an art quality gate for main releases. Never make an aesthetic/subjective test block a release.

### Private staging artifact

The private owner-delivered staging ZIP contains \`beckenburg/patron_*.rlprop\`, \`selection.json\`, the converter, and installation notes. It intentionally does **not** contain upstream FBX/PNG originals. The ZIP is **not** a complete installable \`.rvpack\`. The binary assets are not committed to this public repository.

## Non-destructive integration requirement

**RvPackInstaller installs a complete replacement asset revision.** Installing just the six character meshes as the next Red Ledger package could remove existing private Beckenburg room assets, smoking models or other optional assets from the active content path. For the actual signed release, merge these six selections into the existing complete private Red Ledger content staging and publish it as a new fully verified revision, or embed them with the signed combined-game APK using an appropriately private build process. Do not replace the existing complete content pack with a six-file-only pack. Preserve game saves and all unrelated native game content.

The current \`red_ledger_module.cpp\` already loads the six \`kPaths\` through \`read_asset\` into VBOs and texture objects, and draws only the active patron. Its primitive silhouette remains the failure-only fallback when a matching private asset is unavailable. No scene/interaction hitbox or economic state must change.

### Remaining functional gates

- Physically install a complete content revision or integrated signed APK; confirm actual \`6/6\` RLBP load diagnostics and visible renders on both S9 eyes.
- Verify the controller ray, customer interaction target, room navigation, content updates and save persistence are unaffected.
- Validate steady FPS, temperature and memory during sustained play.
- Animations are not represented by RLBP v1. These converted characters are **static rest poses**, despite the upstream FBX rig names. A separate skeletal animation/render path is needed for walking, sitting, gestures and interacting.

No claim of headset integration, signed APK, or game release is made here.
