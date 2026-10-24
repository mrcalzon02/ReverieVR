# Between Deliveries — Core Customer-Service Design (author clarification, 2026-10-24)

**Status:** Author-confirmed gameplay interpretation. This supplements the original Google Drive design; it does not supersede its narrative, supplier catalog, or campaign. Source: https://docs.google.com/document/d/1xRq-sHxbdNexYifVbIbzB4ArEc3gy5Rlvbb9ae5LCoU/edit

## Central mechanic
The player's principal decision is **which customers to serve and which alcohol/products to serve them**. The six suppliers are not merely price lists or abstract faction meters. Repeated stocking and serving choices gradually align the bar with suppliers and thereby change the kinds of patrons who come, remain, and leave. Patron selection and serving accuracy drive the economy, faction relationships, and bar identity. Do not substitute a generic housekeeping simulation.

## Patron preference, cleanliness, and conflict
Patron archetypes have alcohol/product preferences and associated cleanliness ratings. Distinguish **cleanliness expectation/tolerance** (the environmental standard a patron needs) from **cleanliness impact** (how much that patron type degrades the shared environment). Some drink preferences correlate with less tidy, more disruptive patrons; do not assume every drinker of a product behaves identically. Low-cleanliness clientele gradually lower the bar's environmental condition and can displace patrons with higher cleanliness expectations. Clientele mix also affects fight/disorder risk, which feeds into comfort, reputation, and retention. These are linked consequences of whom the player serves, not independent minigames.

## Baseline and investment
The unupgraded Red Ledger starts noticeably dirty and run-down. Paid cleanliness/comfort upgrades improve its baseline and resilience, enabling the player to attract and retain more demanding customers. Upgrades do not automatically erase ongoing patron impact, faction friction, or fights. No physical trash/spill spawning, manual sweeping, or cleaning minigame is required.

## Progression and implementation contract
1. Supplier-specific products influence customer demand and affinity.
2. Serving a customer the requested or preferred product affects satisfaction, repeat visits, payment, and gradual supplier alignment; wrong/rival service has consequences.
3. Customer archetype composition changes gradually through repeated choices, not instantly after one sale.
4. Aggregate clientele cleanliness impact and upgrades determine bar condition; patron cleanliness expectations influence arrival and return.
5. Clientele compatibility and fights affect comfort, reputation, and retention.
6. Save the relevant persistent service, alignment, clientele, and bar-condition state. Test both rough-bar and upgraded mixed-clientele trajectories.

**Balancing is not yet canon:** Exact numeric rating scales, formulas, fight probabilities, and alignment thresholds require tuning and tests; do not present provisional numbers as author-specified. The source design mentions messy patrons, cleanliness investment, and fights, while the explicit linked customer-service interpretation above was clarified by the author on 2026-10-24.
