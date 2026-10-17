# Between Deliveries — Original Unreal DataTable recovery and inventory transaction repair

**Observed 2026-10-16. Source table verified; device/build not accepted.** Authoritative source: the private Drive file `Between deliveries/Content/Data_tables/DT_ShopItems.uasset`, 17,513 bytes, SHA-256 `06440a1dafa96234140a585185ec3d5159d4e3fd59a8695a5121e8ba3824e62c`. It uses Unreal Engine 5.7. We did **not** publish or edit the original `.uasset`.

## Genuine original product data, recovered without Unreal Editor

A bounded Python decoder `scripts/unreal/recover_shop_table.py` parses **159 Unreal name entries**, **105 imported-object records**, and **25 tagged DataTable entries**. Each Unreal row carries original `ItemName`, `Supplier_ID`, `BuyPrice`, `SalePrice`, `Item_mesh`, `ItemCategory`, and `Image` fields. The decoder resolves negative `FPackageIndex` references through the import table, including the parent package path; all **24 real rows have distinct StaticMesh and Texture2D references**. The 25th `test` row is explicitly excluded. We verified the exact 24 `/Game/Models/Props/Booze/.../StaticMeshes/... .Object` references and `/Game/texture/ICONS/icon_01_300_v2_rt_final` through `icon_24_300_v2_rt_final`, not only the filenames' apparent ordering.

Original source prices, per faction in **draft / bottled / spirits / snacks** order, are:

| Supplier | Buy prices | Original sale prices |
|---|---|---|
| 101 Western Concordant | 8 / 10 / 32 / 6 | 12 / 15 / 50 / 10 |
| 202 UED | 7 / 9 / 30 / 5 | 12 / 15 / 50 / 10 |
| 303 Barforsograd union | 9 / 11 / 33 / 7 | 12 / 15 / 50 / 10 |
| 404 Salihorsk | 8 / 10 / 34 / 6 | 12 / 15 / 50 / 10 |
| 505 Mozoli | 10 / 12 / 35 / 8 | 12 / 15 / 50 / 10 |
| 606 Mazr | 9 / 11 / 36 / 7 | 12 / 15 / 50 / 10 |

The original DataTable identifies whiskey, vodka and rum uniformly as `Beverages`; it does not declare the actual glass/pour yield of a wholesale purchase. The retail prices apply to *original game goods*, **not automatically to one serving in the current pub simulation**. The GDD naming has some minor spelling differences (e.g. Mazar vs Mazr); both names are preserved in the machine-readable catalog. One original item has a leading space in its source text; do not silently treat it as part of the intended display name.

The verified mappings, original product names/prices/categories, and status fields live in `docs/project/BETWEEN_DELIVERIES_UNREAL_CATALOG.json`. `model_binding_verified` and `icon_binding_verified` mean **metadata relationships only**. The original 1K/4K texture pixels, mesh geometry, user rights, Android runtime import and rendered result are still unverified.

To rerun the check privately:

```sh
python scripts/unreal/recover_shop_table.py /private/DT_ShopItems.uasset /private/shop-metadata.json
```

The decoder requires the exact source SHA-256 and package layout. It rejects modified/truncated inputs rather than guessing object offsets for other Unreal files. It does not import binary assets or require UE Editor. On the original download it recovered all 24 rows, and corruption/truncation/short-input tests were exercised locally, with expected rejection.

## Fix wholesale quantities and money safety (pre-1.0)

The original temporary native economy purchased **three servings for every product**. That is economically defective for the 30–36-unit original spirits purchases because ordinary bar receipts could not repay the stock cost. We introduced explicit *provisional* per-purchase lot yields (not assertions about the original Unreal model geometry): **8 draft pours, 6 bottled servings, 12 spirit pours, or 6 snack servings**. Source buy prices remain ×100 in current simulation cents. Original shop sale prices are now preserved separately in metadata for future pricing/balance rules.

Previously `StockBuy` deducted cash with `SpendSpecialStock` and then tried `AcceptStock`; if a stock-capacity check failed the money could already have been charged. `Trade::CanAcceptStock` now checks the actual lot size against the 100,000-unit serializer cap; the native purchase path stages the cash debit on a scratch `Simulation` and commits only on actual successful inventory acceptance. The stockroom BUY ray-target availability uses the same capacity check. This fixes the root cause without untrusted bypasses.

Updated C++ trade/faction regression sources assert 8/6/12/6 lot sizes, max capacity/no overflow across all 24 SKUs, serializing near-cap values, unchanged cash on failed purchases, and new lot counts. **No full local C++17/NDK/APK/device compilation was executed in this pass.** The source decoder was executed on the real downloaded data, and native source/manifest consistency was statically verified before GitHub commit.

## Boundaries and follow-on

Metadata extraction is done; actual full-resolution art still requires compatible UE 5.5/5.7 asset export, license clearance and Android integration. The existing `scripts/unreal/export_between_deliveries.py` remains the mesh/texture extraction path, but it should use the now-verified per-row paths to validate correct exports. Preserve high-resolution masters privately; any lower-quality mobile copy must be derived, not a replacement. Do not run GitHub Actions and do not conflate static source acceptance with a functioning Daydream build.
