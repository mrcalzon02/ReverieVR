# Omega Front continental geography

The 30 × 20 continental map has 600 cells of seeded coastline, rivers, ridges, farmland and roads. Sixteen contiguous Hearthfall sectors have sector inventories, political claims and field orders. Pilgrim Road adds an authored F16 relief route at the eastern edge of Hearthfall. Its four stops share one existing sector and add three playable cells in Eastern March. The remaining 581 cells are geography.

## Pilgrim Road

The campaign opens on the continental map. Choose **Pilgrim Road** to inspect the dispatch board, or choose **Hearthfall** and march east from Hearthwell (S007) to Charter Gate (S008). From the local map, WORLD returns to the continent. With a pad alone, hold Minus to open the pause menu and select **World Map**.

At Charter Gate, dispatch a relief party with four of the campaign's field rations and two repair parts from its salvage. The party moves independently of the home detachment. All three road branches meet at East Ford. Select the ford and then either the mobile clinic (parts) or Pilgrim Camp (rations). Select the current stop again to unload. Each adjacent march advances one normal campaign day, including local consumption, production and any existing strategic deadlines. Loading, unloading and browsing take no days. After serving both destinations, return by the ford and file the manifest at the gate for six trust and ten points of Pilgrim Road relations, subject to their existing caps. Further clicks cannot duplicate cargo or credit.

Each dispatch, march, delivery and filed manifest checkpoints the game. The v10 campaign keeps the 2,048-byte slot and nested v3 tactical record. Importing v1–v9 retains the previous campaign and starts the road undispatched. The route can be reopened from the map after a restart, including when the relief party and the main detachment occupy different places.

| Stop | Continental coordinates | Role |
| --- | --- | --- |
| Charter Gate (S008) | 15, 9 | Dispatch and receive the manifest |
| East Ford | 16, 9 | Road junction |
| Mobile Clinic | 17, 9 | Receive two repair parts |
| Pilgrim Camp | 16, 10 | Receive four rations |

The map draws the existing approved terrain assets with separate eye renders. The home district retains its terrain-based march costs, diplomatic transit passes and viaduct restrictions. Ordinary Hearthfall tactical encounters still use their authored 24 × 24 battlefields. Further regions, political simulation, fog of war outside Hearthfall and multi-operation faction arcs remain open. The road is one field assignment.

## Tactical pacing correction (same development iteration)

The tactical battlefield remains 24×24 tiles, and its **simulation is turn-based**. The old quarter-second loop in `omega_front_controls.h` is removed; neither update `delta_seconds`, idle time, nor stereo rendering advances enemies, allies, ammunition, mission progress, or siege deadlines. Input queues friendly orders at any pace. The **END TURN** button at the top of the battlefield and the **END TURN / RESOLVE** command in the tactical **MODES** menu trigger exactly one `Battle::Step()` (friendly actions, enemy response, objective evaluation). Holding the button cannot accidentally resolve successive rounds. The existing `paused` checkpoint field is preserved as a legacy hold gate, and old checkpoint formats remain readable. A resolved turn is saved immediately, not after a sixteen-tick timer threshold. Tests cover indefinite idle, explicit turn advancement, held-button suppression, tactical menu dispatch, save/load consistency and ended-battle reports.

**Known limitation:** Tactical tiles still use the authored 24×24 encounter layouts; continent-derived battlefield map variation and continent-wide political simulation have not yet been built. Field orders currently cover the sixteen Hearthfall sectors and the separate three-cell extension described above.