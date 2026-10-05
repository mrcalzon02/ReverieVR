# ReverieVR Project Instructions

**Role:** INSTRUCTIONS  
**Authority:** stable project doctrine

## 1. Purpose

ReverieVR is an independent Android VR platform intended to make Daydream-era phone/headset hardware useful again for local VR applications, small games, and video playback after Google's Daydream platform was discontinued.

Reference hardware:

- Samsung Galaxy S9 handset;
- Daydream View headset;
- Daydream controller.

Additional compatible Android phones and simple 3DoF viewers are desirable when support does not compromise the reference path.

## 2. Product goals

ReverieVR should:

1. boot and operate without discontinued Google Daydream servers;
2. provide a local in-headset VR shell/launcher;
3. expose complete local settings for the virtual environment and device behavior;
4. provide seated head-look navigation with configurable physical input;
5. support local VR video playback;
6. host small self-contained VR applications/games as subprojects/modules;
7. expose a documented integration surface rather than binding every experience directly to one device;
8. remain buildable, understandable, testable, and recoverable.

## 3. Reference interaction model

The baseline seated interaction model is:

- **head orientation/head-look** for view direction and reticle pointing;
- **controller trackpad/touch area** for directional navigation, scrolling, and context-sensitive analog input;
- **primary click/select** for activation;
- **secondary/app/back control** for backing out or context actions;
- **Home** for returning/recentering according to configurable behavior;
- **two handset side volume buttons** for volume by default, with remapping only where Android/device behavior safely permits it.

Touchscreen interaction must not be required for ordinary in-headset use after launch.

Bindings must be represented by a configurable input-action layer rather than permanently hard-coded into application logic.

## 4. Performance doctrine

ReverieVR is **performance-first and retro-first**.

The reference phone is finite thermal and battery hardware. VR workloads that chase unnecessary graphical fidelity until the handset overheats, throttles, or drops frames are considered defective.

Default design expectations:

- simple low-complexity geometry;
- deliberately restrained scene complexity;
- small/appropriate texture budgets;
- baked/static lighting where practical;
- minimal transparent overdraw;
- aggressive visibility culling;
- level-of-detail systems where useful;
- object/material batching where it actually reduces work;
- minimal post-processing;
- no expensive effect merely because the framework makes it easy to enable;
- dynamic render scale or quality fallback when required to preserve frame pacing;
- thermal behavior and sustained performance tested on the reference device.

A polished Doom/early-3D-scale visual language is acceptable. Visual complexity is allowed only when measured headroom supports it.

**Stable frame pacing, sensor latency, input latency, thermal sustainability, and legibility inside the headset outrank polygon count and graphical spectacle.**

## 5. Non-goals unless explicitly promoted

The project does not initially promise:

- binary compatibility with every historical Daydream application;
- reimplementation of every Google cloud/backend service;
- redistribution of proprietary Google software/assets;
- 6DoF tracking from hardware that cannot provide it;
- support for every Android device;
- a public app store/cloud ecosystem replacement;
- modern high-end VR graphical fidelity.

Research does not silently expand product scope.

## 6. Authority hierarchy

Resolve truth in this order:

1. current explicit user instruction and granted scope;
2. platform safety, authorization, and repository permission boundaries;
3. this document;
4. `docs/project/EXECUTION_CONTRACT.md`;
5. verified repository state on `main`;
6. `docs/project/EXECUTION_STATE.md` and `docs/project/ACCEPTANCE_LEDGER.md`;
7. durable design/decision records;
8. `docs/project/APK_BUILD_ORDER.md`, `docs/project/BACKLOG.md`, and `CHANGELOG.md`;
9. conversational context, memory, prompts, or informal notes.

Lower layers cannot silently override higher verified authority.

## 7. Repository policy

- Canonical repository: `https://github.com/mrcalzon02/ReverieVR`.
- Canonical branch: `main`.
- Main-only development by default.
- No side branches, alternate implementations, replacement repositories, or PR workflow unless explicitly authorized.
- No force-push or published-history rewriting.
- No GitHub Actions unless explicitly authorized.
- Preserve unrelated user work.
- Reconcile concurrent changes before mutation.
- Prefer one coherent target or tightly coupled change set per commit.

## 8. Engineering rules

### Offline-first
Core headset/runtime operation must remain useful without an external service.

### Capability honesty
No hidden stubs, swallowed failures, no-op compatibility shims, dummy success responses, or simulated service availability. Clearly marked scaffolding is allowed only when it cannot be mistaken for working behavior and is tracked as incomplete.

### Root-cause repair
Repair authoritative source and architecture rather than generated output or layered compensating hacks.

### Reference-first abstraction
Abstract hardware interfaces where useful, but do not over-generalize before the Galaxy S9 + Daydream View path works.

### Graceful degradation
Missing optional hardware/capabilities should disable only the dependent feature, explain the limitation, and preserve unrelated functionality.

### Inspectable local configuration
Headset parameters, render settings, controls, comfort settings, and compatibility decisions must be represented by local inspectable configuration/code with provenance.

### Performance is correctness
Performance, thermal behavior, frame pacing, decoder behavior, tracking latency, and input latency are acceptance criteria.

## 9. Provenance and reverse engineering

- Keep independently authored source separate from third-party artifacts.
- Do not commit proprietary Google binaries, applications, SDK payloads, firmware, keys, credentials, copyrighted assets, or other material without redistribution rights.
- User-supplied local artifacts for compatibility research stay outside distributable source unless licensing permits inclusion.
- Record provenance for imported open-source code, specifications, calibration data, or assets.
- Prefer public documentation, observable behavior, independently written compatibility code, and reproducible fixtures.
- Preserve required attribution/licenses.

## 10. Documentation roles

- `docs/project/INSTRUCTIONS.md` — stable doctrine.
- `docs/project/EXECUTION_CONTRACT.md` — execution/Git/validation rules.
- `docs/project/APK_BUILD_ORDER.md` — ordered product construction.
- `docs/project/BACKLOG.md` — eligible and deferred work.
- `docs/project/EXECUTION_STATE.md` — current verified operational position.
- `docs/project/ACCEPTANCE_LEDGER.md` — admission state.
- `docs/records/` — architecture, investigations, decisions.
- `CHANGELOG.md` — verified completed project changes.

Do not create competing trackers for the same authority role.

## 11. Decision discipline

Recorded rulings prevent drift but are not sacred. A better approach must identify the existing ruling, present the new evidence/tradeoff, and update the authoritative records if adopted.

## 12. Development-unit rule

Every implementation unit states:

- bounded target;
- authoritative files;
- dependencies;
- expected behavior;
- deterministic checks available now;
- device/runtime validation still required;
- acceptance state reached.

Planning alone is not completion.

## 13. Project identity

The project name is **ReverieVR**. It is not an official Google, Daydream, Samsung, or Meta product. Historical names may be used factually for compatibility targets/hardware.
