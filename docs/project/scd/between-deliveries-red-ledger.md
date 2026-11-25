# Between Deliveries: The Red Ledger — completion guide

**Project ID:** `between-deliveries-red-ledger` • game • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
November 16: four patrons served, accounts reconcile, 135-day debt payoff replay; full property/district/monthly story remains open.

## Authority and entry points
- `docs/project/BETWEEN_DELIVERIES_FULL_SCOPE.md`
- `docs/project/BETWEEN_DELIVERIES_CORE_SERVICE_DESIGN.md`
- `docs/project/BETWEEN_DELIVERIES_ASSET_RECOVERY.md`
- Production anchor: `app/src/main/jni/native/red_ledger_module.cpp`
- Production anchor: `app/src/main/jni/native/red_ledger_simulation.cpp`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-red-ledger.sh`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-featured-persistence.sh`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/audit/probes/red_ledger_module_probe.cpp`

## Ordered gates
### G01 — Complete real bar economy and upgrade functions

**Implementation:** Implement documented stock/storage/perishability, customer affinity, supplier alternatives, day calendar, equipment/renovations and usable room controls.

**Does it work?** Run fresh operating days with scarcity, alternative suppliers and changing demand; every paid upgrade changes real behavior. Balance accounts exactly; interruption and write retry preserve transactions once.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Complete property, private rooms and district commerce

**Implementation:** Implement authored upper floor/private activities, district routes, commerce and required original patron/room/product assets. Maintain source/license inventory.

**Does it work?** Every included room/location can be entered and used; required patrons/products are installed and rendered rather than primitive fallbacks; purchases and access obey state.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Implement monthly faction story and all intended endings

**Implementation:** Follow full-scope named cast, monthly prerequisites, neutrality pressure, property decisions and six faction-dependent Month Five endings. Resolve ambiguity with source canon, not invention.

**Does it work?** Legitimate fresh campaign replays reach each specified ending, with logged prerequisite/consequence chain and preserved calendar/relationships. No debug jumps or invented neutral ending.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G04 — Validate finished hosted campaign

**Implementation:** Exercise full workday and month transition through Android, real audio, seated readability, save corruption and controller recovery.

**Does it work?** APK and S9 prove service/payment/wash/upgrade and story progression to ending; no lost payment, unreachable action or persistent deadlock.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
