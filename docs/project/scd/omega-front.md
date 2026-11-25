# Omega Front — completion guide

**Project ID:** `omega-front` • game • **Initial gate status: unverified pending current-source reconciliation**

## Evidence baseline
November 16: 16 Hearthfall sectors plus clinic/camp/gate delivery route; midtrip save identical stereo. Regional and 70-arc campaign incomplete.

## Authority and entry points
- `docs/project/OMEGA_FRONT_HEARTHWELL_PROLOGUE.md`
- `docs/project/OMEGA_FRONT_GRAND_CAMPAIGN.md`
- `docs/project/OMEGA_FRONT_FACTION_CHRONICLES.md`
- `docs/project/OMEGA_FRONT_CONTINENT.md`
- `docs/project/OMEGA_FRONT_FACTIONS.md`
- Production anchor: `app/src/main/jni/native/omega_front_module.cpp`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-omega-repairs.sh`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/verify-omega-front-playability.sh`
- Existing diagnostic/probe (read its documented arguments before running): `scripts/audit/probes/omega_hearthfall_replay.cpp`

## Ordered gates
### G01 — Finish Hearthfall operations and aftermath

**Implementation:** Wire legal action/cost explanations, mission goals, faction accords, settlement, logistics and tactical input on all existing operations.

**Does it work?** Cold start, complete each existing operation, win and lose representative battles, settle outcomes, then save/reopen strategic/tactical state and demonstrate persisted faction/logistics consequences.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G02 — Complete strategic management and supply

**Implementation:** Implement roster, equipment, research, production, convoy, planning, intelligence and history pages using actual simulation state.

**Does it work?** A player can recruit, equip, research, deliver resources, issue and cancel orders, and see costs and resulting state; no inert actions. Scarcity and access constraints affect deployment.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G03 — Expand continental operations and authored arcs

**Implementation:** Implement remaining authored arcs in chronology from grand campaign and faction documents, region by region. Keep coverage manifest mapping each required arc to actual maps/objectives/transitions.

**Does it work?** For each claimed arc, enter legitimately from preceding state, finish or fail/recover, record settlement and next unlock; overview geography alone is not playable content. All required campaign arcs must have witnesses.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

### G04 — Complete campaign and platform acceptance

**Implementation:** Run full progression routes through intended ending(s), preserve tactical queues and RNG on interruption, validate licensed content and controller readability.

**Does it work?** Installed campaign can reach all required ending paths without debug state, softlock or lost progress; sustained device and shared acceptance pass.

**Required evidence:** Production-path replay with initial/final state, actual inputs, assertions, logs and revision; capture both eyes for visual changes.

**Decision:** PASS only with independent matching evidence. NO: reproduce, repair production source, rerun this gate. BLOCKED: record exact missing capability and retry condition; preserve checkpoint and select another ready track. Do not skip forward within this track merely because a proposal was written.

## Completion
Apply the [shared SCD contract](../SUSTAINED_CONTINUOUS_DEVELOPMENT.md) and every applicable common end-to-end condition. Record revision, actual runner/logs/artifacts and source readback in the existing acceptance ledger. Maintain this guide when owner-approved scope changes.
