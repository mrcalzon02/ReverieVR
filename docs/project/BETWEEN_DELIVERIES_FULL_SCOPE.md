# Between Deliveries — Full Game Scope and Source Reconciliation

**Authority and status (2026-10-16):** This file is a fidelity map from the user's complete Google Docs design, **not** a replacement for those documents or proof that every feature exists. The canonical design source is [“Between deliveries”GAME](https://docs.google.com/document/d/1xRq-sHxbdNexYifVbIbzB4ArEc3gy5Rlvbb9ae5LCoU/edit). Supporting primary sources: [Between deliveries scripts](https://docs.google.com/document/d/1nqZcaSccx4ZCewrBCZykmxGVygvGsrMJmDN9VwdXgRo/edit) and [Between deliveries booze](https://docs.google.com/document/d/160Zvlg8n0G2d9ChURwriYiPLJN-CyNsNxnlpaX4Ong4/edit). Main design last observed modified 2026-09-06; source read in full on 2026-10-16.

**Essential boundary:** `between-deliveries-red-ledger` currently implements **the opening bar slice**, NOT the entirety of *Between Deliveries*. Continue development as one economically and narratively connected survival sim, not a standalone bar minigame. The bar is the first business and control surface through which the player gradually influences the Mininska neighborhood, relationships, rooms, supply chains, and political attention. Treat world/character progression as first-class future systems, and preserve the opening bar loop without treating its limits as intended final content.

## Premise and player

Kazoff Mikhail Andreyevich Kazoff is a displaced debtor and reluctant bar operator in the contested Barforsograd region of a fictional post-Collapse, Soviet-fragmentation setting within the document's stated IronSight world. The bar, **The Red Ledger**, is financed by a local criminal organization. It begins as one crumbling concrete room, one tap, one stool, mismatched cups, a flickering bulb, and a backroom holding a mattress and financial ledger. The protagonist is not meant to become a conquering crime boss; advancement makes them *valuable infrastructure*, attracting rivals who want to take what they built.

Win/lose is not a linear heroic ascent. Long-term gameplay must model survival, competence, economic stabilization, coercive patronage, fragile neutrality, and eventually political dispossession or forced departure. The original document explicitly gives **six faction-specific endings** at the end of Month Five, not an endless identical four-customer day loop.

## Authoritative gameplay pillars and actual scope

| Pillar | Intended scope | Current repo assessment / priority |
|---|---|---|
| Manual service and room comfort | Pour drinks, cup washing, stock use, diverse regulars, serving accuracy, mess/cleanliness, fights, comfort, reputational effects | **Partial implemented**: manual cups/tap/customer/payment, six patron archetypes (four per operating day), debt and pressure. Mess, fights, patron memory, genuine environmental comfort remain **open** |
| Supply chains and merchandise | Six suppliers with different goods, per-category costs, quality, availability, reliability, political ties, delivery interruption, faction-affiliated drinkers | **Source-integrated / hardware-unverified**: `red_ledger_trade.h` now has all six named suppliers and the 24 GDD/Unreal-shop product labels with 18 branded drinks + six snacks, on top of a separate basic house tap. Distinct original Unreal meshes/textures are NOT yet renderer-compatible or loaded |
| Purchaseable property upgrades | Five tiers across walls, floor, ceiling, bar stools, draft/bottled/spirits/snack storage, plus lighting, sound, tap capacity, cleaning and later premium stock | **Open**: small current stock/bar actions do not implement multi-tier improvements, discounted prices, comfort or capacity outcomes |
| Reputation, factions and neutrality | Six relationship tracks, customer product loyalty, supplier-specific upgrade discounts at thresholds 1–5, pressure when favoring one bloc, inspections/risks | **Partial**: six saved supplier favor values and buyer-specific drink/snack affinity now exist; the economic discounts are a provisional three-threshold wholesale rule, not the complete five-tier upgrade system, neutrality or ending decisions |
| Operating bills and coercion | Debt service, electricity interruptions, protection demands by calendar month, inspection regime, environmental condition, service quality and repair costs | **Partial**: current simulation cycles protection/inspection/supply interruption every three days; this is not the GDD's monthly protection cadence |
| Building expansion | **Midgame second floor**, private booths, card games, quiet meetings, more expensive customers, higher profit and political attention; **late-game private rooms**, premium storage, faction meetings and laundering | **Open**: current module's additional room concepts are service/stock/living; they are not the planned upstairs/private-room income layer. Do not relabel storage as an upstairs floor |
| Neighborhood economy | Growing street around bar: casino, grocery, hotels, other businesses, dockworker presence, economic stability, local reputation and external interest; business changes cause district effects | **Open**: single rendered bar with no linked outside economic state or distinct active district entities |
| Narrative and characters | Intro, initial debt/terms of protection, end-week one, Month Two Mininska recovery, Month Three Grigory→Sonia changeover, Month Four neutrality ultimatum, Month Five six endings; diegetic ledger, notes, absences, reports | **Open**: generic recurring event loop rather than story-state transitions and named, persistent cast |
| Visual execution | PSX-inspired *intentional* low-poly world: readable signage, textured props, old concrete, industrial ruin, flickering power, weather and narrative environment changes—not low-information placeholder boxes | **Partial**: 3D bar, some baked sign geometry, material atlas/mugs, with remaining prop/readability and multi-location scenes outstanding |
| Saving, portable/offline hosting | Transactional saves for inventory, debt, patron relationships, building tiers, district and narrative checkpoints; head-look/controller and shell recovery. **Before version 1.0, no legacy save migration obligation** | **Partial**: clean, integrity-checked pre-1.0 v3 save slot records six-faction stock; obsolete saves are intentionally ignored until post-1.0 compatibility policy begins |

## Six suppliers and canonical starter price matrix

The GDD describes **six**, with four purchasable product classes per supplier. The numerical source is given as $ units, NOT established to be the same denomination as current simulation cents. **Current pre-1.0 conversion:** catalog list price in design dollars ×100 to the runtime `cents` account denomination, for **one provisional category-appropriate lot** per supplier purchase (8 draft pours, 6 bottled servings, 12 spirit pours, 6 snack servings). These yields are an explicit pre-1.0 balancing policy, NOT described as those exact quantities in the source GDD. The original Unreal table also lists **sale prices** of 12/15/50/10 for the four classes, but they refer to source shop goods, NOT necessarily to each current individual bar service.

| Supplier | Draft | Bottled | Spirits | Snacks | GDD identity |
|---|---:|---:|---:|---:|---|
| Western Concordant Trade Alliance | 8 | 10 | 32 | 6 | Reliable, order-focused, expensive; inspection/legal leverage |
| UED Provisioners Integrated | 7 | 9 | 30 | 5 | Bulk, volatile quality, militarized supply |
| Barforsograd Brewers Union | 9 | 11 | 33 | 7 | Local working-class identity, regulars and dockworkers |
| Salihorsk Dry Stills | 8 | 10 | 34 | 6 | Hard liquor, coercive supply chain |
| Mozoli Brew's | 10 | 12 | 35 | 8 | Mid-tier and loyal drinkers |
| Mazr Vodka | 9 | 11 | 36 | 7 | Status and privacy, military-proletariat contacts |

The complete named starter inventory, in the GDD's four-item order per supplier, is:

- **Western:** Blue Harbor Draft; Green Harbor Lager; Harbor Reserve Whiskey; Candied plums.
- **UED:** Amber Union Ale; Union Star Lager; Union House Whiskey; Golden Fry Chips.
- **Barforsograd union:** Redline Dark Ale; Redline Spiced Rum; Redline Barrel Whiskey; Hot Ration Crisps.
- **Salihorsk:** Iron Mug Stout; Crossmark Bitter; Dockside Brown Whiskey; Salt Pretzel Bag.
- **Mozoli:** Market Amber Ale; Greenmark Bitter; Greenmark Clear Whiskey; Street Sausage Tin.
- **Mazr:** Green Draft Lager; Mazr Blue Vodka; Mazr Crystal Vodka; Beans chicken and potato.

The packaging companion doc specifies 24 distinct **sealed, branded shipping-pack forms** (four for each supplier). Future models/UI/warehouse stock should represent real cans, growlers, bottles, cartons, pouches and tins—do not display identical placeholder blocks with different labels.

**Upgrade matrix:** 6 suppliers × 8 listed upgrade categories × 5 tiers; GDD baseline tier prices $500/$1,000/$1,500/$2,000/$2,500 for each category before supplier-specific faction discounts. Threshold amounts/discount percentages are **not specified in the source**, only five levels are requested; choose/calibrate them as an explicit balancing decision rather than falsely attributing numbers to the GDD.

## Campaign beats and state

1. **Arrival / opening days:** Kazoff is given the bar by crime creditors; financial literacy and quiet competence are survival, not power. One tap, one stool, simple drinks, dated notes and ledger.
2. **End of Week One:** Grigory recognizes reliable operating behavior and explicitly warns against serving wrong faction products; first week's protection is paid, attention grows. Missed payments must produce consequences rather than just decorative dialogue.
3. **End of Month Two:** Investment and predictable local commerce revitalize the district and its businesses; **Mininska** gains or regains its name and becomes worth contesting. Casino, grocery, lodging and neighborhood activity become real economic context, not only cutscene lines.
4. **End of Month Three:** Grigory is killed/replaced by **Sonia Yelena Markovna Volkov** when his UED alignment endangers neutrality. Patron ownership, risk and dialogue change durably.
5. **End of Month Four:** Sonia reviews payments and alliances, warns that rising faction preference threatens district viability, and explicitly demands restraint.
6. **End of Month Five:** Six mutually exclusive faction-dominance finales, each a consequence of actual saved alignment/pressure/economic choices (not random or a purely cosmetic screen):

| Favored supplier | GDD ending | Outcome |
|---|---|---|
| Western Concordant | Death by Paper | Legal/property erasure and Mininska emptied |
| UED Provisioners | Appreciation | State seizure, deportation/detention |
| Barforsograd Brewers Union | Too Successful | Influence feared; Kazoff eliminated |
| Salihorsk Dry Stills | Incorporation | Criminal takeover, Kazoff removed |
| Mozoli Brew's | Accident | Dockworker celebration and catastrophic fire |
| Mazr Vodka | Family | Absorbed/relocated; Kazoff does not return |

**Design gap to resolve explicitly:** The GDD describes six faction-favored endings, but does not define a neutral seventh ending or whether neutral play escapes a bad ending. Do not invent either without noting it as a design extension. Narrative gating should respect finances and milestones but must not silently skip a required beat when a player advances early.

## Named cast and their systemic roles

Kazoff (player operator); **Grigory Ivanovich Sokolov** (casino/protection and early infrastructure); **Sonia Yelena Markovna Volkov** (post-Month-Three shadow patron enforcing balance); **Kiril Sergeyevich Antonin** (accounting and economic monitoring); **Anton Vladimirovich Petrenko** (violence control); **Vera Oksana Leonidovna Markelova** (Mozoli dockworker faction/community, possible fire); **Col. Pavel Dmitrievich Mazrin** (Mazr military ties/removal); and **Inspector Alina Viktorovna Rozen** (Western legal eviction). They have documented life histories, traits and consequences, not interchangeable generic customers.

## Active-document ambiguities: preserve; do not silently retcon

- The initial design introduces a Barforsograd bar called **The Red Ledger**; the introductory script mentions **The Red Letter Bar**. Confirm exact in-world sign and whether it is an alias/typo before permanently altering assets.
- The intro already calls the arrival neighborhood **Mininska**, while Month Two describes that name returning. This may represent nonlinear narrative framing. Establish chronological intent before a final intro implementation.
- The main text calls the world an **IronSight** setting, while adjacent project naming has since evolved. Preserve world provenance without automatically renaming canon.
- The Grigory cast entry explicitly says **“Profile assumes survival” / alternate continuity**, while the Month Three main narrative kills him. Treat survival as an alternate branch unless selected as the canonical timeline.
- There are source spelling variations **Western Concorde/Concordant** and **Barforsograd/Barsofagrod**; use primary faction naming and retain aliases during data import.
- Event cadence, price units, exact faction threshold percentages, and numerical district metrics need balancing decisions and tests, not presumed quotations from the design.

## Engineering sequence — prioritize gameplay, not obsolete pre-1.0 save formats

**BD-FULL-00 — Authority sync (this artifact):** Record complete source URLs, full-scope feature matrix, exact 6×4 product identities, six-way outcomes, source conflicts and current code gaps. Prevent the existing room's status from being described as complete gameplay. **Implemented as design/engineering record only.**

**BD-FULL-01 — Canonical economics (partial source-integrated; not device-accepted):** Six supplier identities and 24 authentic GDD/Unreal product labels now drive the stockroom's 24 purchase choices, six patron archetype preferences, prices, promotions and saved supplier favor. A separately payable house tap remains. Trade save v2 and host save slot v3 intentionally discard previous development-only saves (per user instruction); corrupted current saves still reject atomically. The original Unreal product meshes/icons are **not** imported/rendered yet; five upgrade-discount tiers and broader balancing are still open.

**BD-FULL-02 — Bar systems:** Quality/perishable goods, distinct repeat customers with supplier identity, mess/comfort, cleaning, fights and notoriety, power reliability, supplier stock interruptions and justified billing/protection cadence; maintain service action and UI legibility.

**BD-FULL-03 — Upgrade/building economy:** Real purchase/cost/capacity with all eight categories × five tiers, additional bar improvements and visual upgrade states, not an upgrade button that only increments text.

**BD-FULL-04 — Second-floor transition:** Actual distinct reachable upper floor with private booths, cards and confidential faction conversations. Separate economies (high margin/low volume; attention), distinct real room props and exit route. Do not claim unlock while only a bar-only menu exists.

**BD-FULL-05 — Neighborhood and private rooms:** Later private rooms, premium storage, faction meetings, illicit finance consequences, district businesses and economic feedback loop. The existing seated 3DoF navigation must remain usable; genuine independent spaces and enter/return paths, not invisible room labels.

**BD-FULL-06 — Month-by-month narrative:** In-world notes/scripts, named actors, event prerequisites, protection handoff Grigory→Sonia, contested district status and six real ending branches. Save durable decisions and avoid deterministic faction selection being overwritten by hard-coded calendar steps.

**BD-FULL-07 — Acceptance:** Host C++ state-machine tests, current-save corruption/rollback checks, native module and Android/NDK compilation, source distribution/rights audit, actual S9 two-eye input/audio/readability and sustained performance, and end-to-end narrative milestones before release claims. Prefer practical content over decorative empty menus.

**2026-10-16 original-product stockroom presentation pass (source-integrated, hardware unverified):** The native stockroom now offers direct head/controller-ray selection of all 24 real supplier products in a single six-column (101–606) by four-category physical board; category/faction labels and color groups are baked once into a stationary room VBO, with on-hover borders and full product name/cost/quantity on the current status board. Exact controller hit bounds and product arrays are data-linked; no 24-click cycling is required. This is a temporary readable interaction surface, **not a substitute for the original high-resolution Unreal meshes and 1K+ textures**. Original asset export and rights clearance remain open. See `docs/records/2026-10-16-between-deliveries-direct-sku-stockroom.md`.

**Original-art recovery implementation (2026-10-16):** A usable Unreal Editor Python export utility now lives in `scripts/unreal/export_between_deliveries.py`, enumerating the source product package objects for all 24 real SKUs and exporting full-quality PNG textures, FBX meshes and DataTable CSV into a separate private folder with validation/hashes. No original full-resolution pixel/model payloads have yet been exported, license-cleared or wired into Android, because the live UE project/editor is not available. Four pure-Python exporter safety tests passed. Consult `scripts/unreal/README.md` and `docs/project/BETWEEN_DELIVERIES_ASSET_RECOVERY.md` before treating any future files as game assets.

**Original economic data reconciliation (2026-10-16):** Decoded the actual UE5.7 DataTable (all 24 product rows plus excluded test). Exact vendor buy price, original sale price, category, item-to-mesh and item-to-icon links are recovered, not inferred from naming. Wholesale purchase lots were adjusted to 8/6/12/6 servings to restore viable margins while leaving original source prices in the metadata. The actual native stock purchase is now atomic across cash and inventory, with precise cap checks to prevent charging for stock that cannot be accepted. `docs/records/2026-10-16-between-deliveries-exact-unreal-shop-recovery.md` details evidence, unresolved rendering and licensing.

**2026-10-16 audible owner soundtrack:** The registered native module `between-deliveries-red-ledger` now selects `The Last Note Hangs Unresolved` from the existing owner-original, commercially authorized, locally packaged MP3 library. This corrects an obsolete `bar-project` cue-ID mismatch; the shared shell loops the assigned track while hosted, honors volume/mute/lifecycle, and restores Home music on exit. Pure-Java route smoke test passed; actual Galaxy S9 playback remains unverified. The Unreal project's 14 `Sounds/Music/*.uasset` faction/ambient tracks are a **separate** unexported collection, not yet audible native files. [Evidence](../records/2026-10-16-between-deliveries-owner-music-routing.md).

## Original Unreal artwork and economic data recovery (2026-10-16)

The author's original Unreal Engine 5.7 `Between deliveries/Content` assets are now accessible in private Google Drive. For an inspected asset inventory, checked source binary signatures/sizes, rights gates, and real full-resolution export path see [BETWEEN_DELIVERIES_ASSET_RECOVERY.md](BETWEEN_DELIVERIES_ASSET_RECOVERY.md). The 24-product original source-name, buy/sale price, category, and **VERIFIED imported mesh/icon reference mappings** are now in [BETWEEN_DELIVERIES_UNREAL_CATALOG.json](BETWEEN_DELIVERIES_UNREAL_CATALOG.json), independently reproduced by [the checked original Unreal shop-table decoder](../../scripts/unreal/recover_shop_table.py). File bindings are verified as metadata; usable Android source pixels/geometry, licensing, and actual runtime rendering remain pending. Unreal `.uasset` packages are **not** compatible runtime Android GLES textures/meshes/sounds: preserve masters, export through a compatible editor, retain appropriate licenses and only then integrate usable derived assets. No uploaded binaries or APK render have been accepted in this pass.


## Evidence / current status

At reconciliation on 2026-10-16 the native module is `app/src/main/jni/native/red_ledger_module.cpp`; its authoritative bar state is `red_ledger_simulation.{h,cpp}`, and the small secondary market/catalog is `red_ledger_trade.h`. After the six-faction integration pass the latter has `kFactions=6`, `kDrinks=19` (18 branded + one tap), `kSnacks=6`, and a checked 152-byte trading save. The `Simulation` has six patron archetypes with four arrivals per day and still uses a provisional three-day pressure cycle. A separately published phone-test build is documented in the project's acceptance ledger, **not** proof that any complete five-month game, second floor, district or six-faction economy has been built. Live source can change independently; every future slice must read back before editing.

**Scope lock:** Never report Between Deliveries as a completed single-bar game, nor quietly replace the Google Docs full five-month design with the current VR demonstrator. Reconcile new changes to this source inventory, implementation status and source URLs in the same commit.
