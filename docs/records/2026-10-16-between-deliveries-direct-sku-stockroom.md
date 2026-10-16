# Between Deliveries — 24 original SKU stockroom picker (2026-10-16)

**Acceptance state:** source-integrated, **not** Android/Daydream accepted. This implements original catalog *interaction and authored faction-readable display geometry*, **not** extraction or runtime rendering of the imported high-resolution Unreal textures/models.

## Why this exists

The previous stockroom required cycling `NEXT` up to 24 times to reach a requested supply product; that made the recovered six-supplier/24-SKU catalog unpleasant and hard to navigate in seated VR. The original shop table and full GDD define six distinct supplier groups and four purchasing categories per supplier. The new interface presents that exact shape rather than flattening everything into a long hidden cycle.

## Actual changes

- New pure C++17 `red_ledger_stock_layout.h` establishes six supplier **columns** (101–606), four product **rows** (draft, bottled, spirits, snacks), a stable rectangular selection surface per product, and the exact underlying `Trade` offer index for all 24 items. Works with the current 18-branded-drink / six-snack / one-house-tap arrangement.
- `red_ledger_signage.h` bakes 24 actual world-space SKU cards into the existing static signage VBO on GL context creation. Each card is compactly labeled (e.g. `303 SPIRIT`), tinted by faction, and visually distinct from the current inventory details. It does **not** claim to be the original high-resolution product artwork. The selection grid is reused for hit testing and rendering to prevent mismatches; no per-frame pixel upload or 24 extra glyph draws.
- `red_ledger_module.cpp` ray-picks all 24 original SKU cells, tracks the hovered offer, outlines the active card, and immediately selects its actual live wholesale SKU when clicked. The existing `BUY` target retains the real transactional wallet, standing-based pricing, available-stock and supply-interruption gates. The full original product name, cost, inventory count and owning faction remain in the current status display. `NEXT` remains as an accessibility/backup control.
- `Trade::SelectOffer(int)` verifies a real, non-house SKU and only mutates a changed selection. It never purchases as a side effect.
- Added `red_ledger_stock_layout_test.cpp` to `scripts/verify-red-ledger.sh`; expanded the signage test to prove room-1's SKU signage is actually baked rather than only defined in a table.

## Verification performed and remaining

- Remote current-source inspected before editing; immutable code blobs created with minimal source replacements and kept separate until atomic `main` commit.
- Independently counted existing baked signage glyph runs: old Bar **4,080**, old Stock **1,866**, Living **2,178**, new Stock **13,698** vertices, below the existing **24,000**-vertex room test limit; the stockroom's extra 11,832 vertices constitute physical SKU cards, not per-frame labels.
- Ran a standalone equivalence check of the cell geometry and offer-mapping math in **g++ and clang++**, both passing (all 24 unique IDs, no overlapping cards, within 4.25-meter ray reach, room extents respected). Exact integrated project host test/script, native module compilation, Android packaging, stereo headset usability and runtime GL draw are **not yet executed in this pass**. Do not upgrade the acceptance ledger to static/device accepted on this basis.
- The Drive upload only exposes Unreal `.uasset` source containers for these meshes/textures. Original **1024px+ branded pixels and original 3D bottles/casks are not in this commit**. Preserve masters and rights-clear before replacing the temporary labeled cards with verified portable assets. See `docs/project/BETWEEN_DELIVERIES_ASSET_RECOVERY.md`.

## Next gates

Export actual original mesh/texture data through compatible UE 5.5/5.7 toolchains or recover author source GLB/PNG, verify per-family redistribution entitlement, import platform-friendly runtime variants **without overwriting high-resolution masters**, bind original product model/icon identities to these 24 cells, and run `scripts/verify-red-ledger.sh`, Android build and Galaxy S9 seated two-eye controller/readability/thermal checks. The renderer must be able to show the correct source asset or explicitly identify its fallback, never silently substitute package thumbnails as an alleged high-res import.
