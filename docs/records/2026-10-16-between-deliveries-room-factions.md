# Between Deliveries — three-room and faction service implementation (2026-10-16)

## Evidence state
**Source integrated on main. Host executable tests and Android/Daydream acceptance not run in this pass.** This record describes source-level behavior, not a tested APK. Retain the Development-only admission gate until physical validation.

## Room topology and usable controls
The existing Bar now has **STOCK** and **ROOM** doors. Both require empty hands and no outstanding payment; selecting one changes the guest room without rerouting shell Home/Quick Menu ownership. From the stockroom **TO BAR** returns to service. From the living quarters **TO BAR** and **TO STOCK** provide two exits. All three rooms have separately baked static geometry and room-specific rendered letter signs: 11 static cubes per room, one static-room draw per eye, with context-time generation and a single room-scoped signage draw. Stock shelves, cases and casks and the living bed, pillow, writing desk and ledger are intentional early-PlayStation-style procedural art. The old bar-facing supplier/ledger papers are no longer rendered as inactive duplicate controls.

The stockroom contains existing beer/cup crate purchasing stations plus a **NEXT** selector and **BUY** action for the extended catalog. The live text board identifies the selected product, cost, unit count, stock on hand, cash and supply interruption state. These buttons perform real stock purchases through Simulation::SpendSpecialStock and the market inventory; buying a crate still uses the original supply API.

The living-room **LEDGER** toggles an in-world financial summary showing day/cash/debt/reputation, faction favor, and the previous day's sales, paid debt and lost patrons. **SLEEP** uses the existing Simulation::CloseDay: it is unavailable until every patron is handled, payment is collected and no cup is held. This preserves debt settlement, inspection, protection, persistence and the deterministic day schedule. Rooms do not remove shell emergency recovery.

## Products and faction service
The game-private `red_ledger_trade.h` implements four deterministic patron factions mapped to the existing four types: Rail Union / rail worker, District Office / night clerk, Border Militia / militia veteran and Workshop Guild / mechanic.

Eight drinks: House Lager, Rail Stout, Clerk Pils, Militia Rye, Workshop Ale, Frontier Rum, Night Vodka and Orchard Cider. Six snacks: Pickled Egg, Sausage Roll, Rye Bread, Salt Nuts, Smoked Fish and Potato Pie. Existing house beer is still bought in eight-unit crates; the seven additional drinks and six snacks have individual three-unit wholesale listings with finite stock. Each patron has a favorite drink and snack and accepts alternatives. Preferred or faction-aligned products yield higher payment and favor, and favorite products increase reputation. Drink service still follows pick-up > pour > serve > collect. Snack selection is optional; it is decremented only on a successful atomic sale, not on a discarded or rejected service.

Bar **DRINK** and **SNACK** buttons cycle available products, and the current patron's faction, requested drink/snack, selected products and basic cash/beer/cup quantities appear as *actual in-world text geometry* in the scene. The text board is regenerated only after a gameplay state change and uploaded through an existing GLES2 draw pathway, not every eye or every frame.

## Save compatibility
The host slot name remains `state-v2.bin`. A new save stores the unmodified original 160-byte Simulation V2 segment followed by a 100-byte checked Trade V1 segment (stock, favor, choices, selected offer and room), written as one 260-byte host operation. Loading a 160-byte original save migrates to the new default trade inventory. Loading both segments uses temporary state and commits only after both decode, retaining the opening state if invalid. The engine's original 160-byte serialization and V2 validation are retained; only the extended path permits bounded premium outstanding payments.

## Source and test inventory
- `app/src/main/jni/native/red_ledger_module.cpp`: native room transitions, gameplay targets, visuals, live text and composite slot
- `red_ledger_trade.h`: faction/offer logic, pricing, checked persistence, bounded stock and favor
- `red_ledger_simulation.h/.cpp`: special pours, wholesale debit, premium/payment integration
- `red_ledger_static_geometry.h/.cpp`: three fixed room batches
- `red_ledger_signage.h`: true glyphs, room-scoped signs, dynamic order/status board
- `app/src/test/native/red_ledger_trade_test.cpp` plus static geometry, signage, simulation and mug tests
- `scripts/verify-red-ledger.sh`: host GCC/Clang deterministic regression runner; **not executed during this pass**

No committed identifiable Between Deliveries-specific .obj/.gltf/.png furniture/NPC/pub asset collection was found in the inspected ReverieVR `main` tree. Do not claim imported graphical assets or a finished art pipeline.

## Required gates before acceptance
Run the host regression runner under GCC and Clang, the native doc-sync verifier and a full Android/NDK build. On the Galaxy S9, inspect both eyes, actual spatial lettering legibility, ray alignment for all doors/buttons, seated comfort, late-day bed action, original 160-byte save migration, 260-byte transactional save/update durability and stock/favor math, and 15+ minutes of frame/thermal data. Confirm all native modules still launch and return safely under Development, including shell Quick Menu and Home.
