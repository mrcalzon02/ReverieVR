# Omega Front — Square Icon Art Contract

Status: visual production constraint; faction identities remain provisional until creator approval.

## Canonical reference
Use the original Omega Front symbol sheet (gold-framed, dark steel-blue science-fiction symbols) as the global UI style guide. Use approved F06 Lost Ninth, F07 Ashen Crown, and F08 Copper Steppe portrait sheets only as visual references for their *own* faction. Do not reuse subjects or names from another faction.

## Export requirements (strict)
- Every gameplay icon is a **true 1:1 square**, 256 × 256 pixels, independently extractable.
- A twenty-icon atlas uses exactly five columns and four rows (1280 × 1024 pixels). The atlas may have neutral gutters only when the manifest defines their exact coordinates; the canonical atlas has no gutters.
- One centered unit, vehicle, building, or symbol per tile. Square frame on all four edges.
- No decorative faction title strips, banners above the grid, number strips or label bars inside individual icons. **Do not use rectangular preview cards as icons.**
- UI text (unit name, rank, status, cost) is rendered by the game outside the image; never baked into the icon.
- Foreground silhouettes remain readable at 64 × 64 and 128 × 128. Avoid duplicate poses, ambiguous weapons, and indistinguishable classes.
- Retain the original gold-bronze corner frame, deep navy/charcoal background, warm rim light and PSX-era painterly military tone.
- Do not invent named factions. Only draw roles sourced from `OMEGA_FRONT_FACTIONS.md` or visibly mark added roles as proposed.
- Each sheet gets a faction ID, individual named PNGs, source artwork, manifest, contact sheet, and visual QA (tile geometry, content identity, duplicates).
- Concept posters and rectangular promotional art belong under promotional/mockup assets and **must never be cataloged or wired as tactical icons**.

## Candidate faction chronology
F06 The Lost Ninth: Roman infantry, standards, siege crews and road engineers.
F07 Ashen Crown: feudal levy soldiers, knights, banner marshals, trebuchet carts, wall masons.
F08 Copper Steppe Confederation: mounted scouts, chariots, bronze shields, slingers, herders and mobile camps.
F09 Brassline Republic: early-industrial trench riflemen, barrage crews, riveted armored railcars, sappers and gas-mask corps.

F10 The Farwake: pearl-white/cyan distant-future refugee precision infantry, narrow triangular drones, compact specialist and medical equipment; 20 square painted portraits staged for menu use.

## Acceptance check
1. Confirm every exported gameplay asset satisfies `width == height == 256`.
2. Confirm each source has consistent framing and no in-image caption strips.
3. Confirm unit identity, era, equipment and faction against the design dossier.
4. Confirm a preview shows 5 × 4 *square* icons, not 20 horizontal cards.
5. Record whether a sheet is painterly art, source-based vector fallback, or rejected image concept; never claim that vector fallbacks are completed painterly portraits.
6. Commit/push only after reading the remote state and verifying the resulting tree contains the expected assets.

This contract deliberately does not claim that the prior local image bundles were uploaded to GitHub.
