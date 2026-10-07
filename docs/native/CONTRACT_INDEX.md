# ReverieVR Native Documentation Contract Index

**Status:** normative maintenance contract.

**Purpose:** keep `docs/native/` actively synchronized with the implementation it describes. This directory is not historical documentation unless a section is explicitly labeled historical.

## Active-truth invariant

A normative statement in `docs/native/` must describe one of these states accurately:

- **implemented** — present in authoritative source now;
- **static accepted** — implementation/documentation checks have passed, but applicable device evidence may remain;
- **device accepted** — the relevant reference-device gate has passed;
- **provisional** — implemented or designed for reuse, but compatibility may still evolve;
- **planned** — not implemented and must never be worded as an available capability;
- **historical** — retained only as history and clearly separated from current instructions.

Known-stale normative documentation is a defect. Do not leave a statement known to be false merely because the code change is already complete.

## Same-change synchronization rule

When a mapped function, service, utility, ABI structure, constant, lifecycle rule, or reusable generated-content mechanism changes, its mapped native documentation must be reviewed and, when the documented behavior is affected, updated in the same scoped change.

For the contract-sensitive sources below, the default is stricter: if the source file changes, the mapped documentation file must also change in that commit. This creates an explicit review record even when the conclusion is that the public behavior remains compatible.

Do not use "documentation will be updated later" as a normal completion state.

## Contract map

| Area | Authoritative implementation | Normative documentation | State |
|---|---|---|---|
| Native C ABI layout, version, callbacks, constants | `app/src/main/jni/native/reverie_native_module.h` | `docs/native/API_V1.md` | Implemented ABI v1 |
| Shared native ABI-consumption helpers | `app/src/main/jni/native/reverie_native_sdk.h` | `docs/native/SDK_HELPERS.md` | Implemented provisional SDK helper layer |
| Shared native GL state guard | `app/src/main/jni/native/reverie_native_gl_state.h` | `docs/native/GL_RENDERING_STANDARD.md` | Implemented focused guard; device validation pending |
| Shared native matrix/shader core utilities | `app/src/main/jni/native/reverie_native_math.h`, `app/src/main/jni/native/reverie_native_gl_utils.h` | `docs/native/CORE_UTILITIES.md` | Implemented provisional shared utilities |
| Native host loading/save/feedback behavior | `app/src/main/jni/native/reverie_native_host.cpp` | `docs/native/API_V1.md`, `docs/native/RUNTIME_SERVICES.md` | Implemented |
| Java/native runtime bridge and module-id/input boundary | `app/src/main/java/io/github/mrcalzon02/reverievr/NativeModuleRuntime.java` | `docs/native/API_V1.md` | Implemented |
| Procedural material atlas generator | `app/src/main/jni/native/procedural_material_atlas.h`, `app/src/main/jni/native/procedural_material_atlas.cpp` | `docs/native/PROCEDURAL_CONTENT_STANDARD.md` | Implemented reference / provisional shared standard |
| Red Ledger static-room geometry baker | `app/src/main/jni/native/red_ledger_static_geometry.h`, `app/src/main/jni/native/red_ledger_static_geometry.cpp` | `docs/native/PROCEDURAL_CONTENT_STANDARD.md` | Implemented reference / provisional shared standard |
| Bounded view-relative locomotion integrator | `app/src/main/java/io/github/mrcalzon02/reverievr/BoundedViewRelativeLocomotion.java` | `docs/native/RUNTIME_SERVICES.md` | Implemented; S9 comfort proof pending |
| Native locomotion gesture re-arm gate | `app/src/main/java/io/github/mrcalzon02/reverievr/TouchpadLocomotionGate.java` | `docs/native/RUNTIME_SERVICES.md` | Implemented; S9 controller proof pending |
| Locomotion/pointer shell integration | `app/src/main/java/io/github/mrcalzon02/reverievr/VrShellRenderer.java` | `docs/native/RUNTIME_SERVICES.md` | Implemented integration; semantic-review source rather than strict whole-file co-change |
| Standard native hosted input profile | `app/src/main/java/io/github/mrcalzon02/reverievr/BuiltInBindingProfiles.java`, `app/src/main/java/io/github/mrcalzon02/reverievr/VrActivity.java` | `docs/native/RUNTIME_SERVICES.md` | Implemented integration; selected facts machine-checked rather than strict whole-file co-change |
| Cross-module engineering rules and ownership | project doctrine + runtime implementation | `docs/native/README.md` | Living platform standard |
| Native systems/method discovery and maturity classification | contract map + standardized sources + selected proven shell references | `docs/native/SYSTEMS_CATALOG.md` | Living discoverability catalog |

When a new reusable native utility becomes part of the platform standard, add it to this table and to the machine-check mapping before calling the utility standardized.

Game-private code is not automatically part of the public/native SDK merely because it appears in a reference game.

Large integration files such as `VrShellRenderer.java` are not strict whole-file co-change triggers because they contain many unrelated responsibilities. Instead, the verifier derives selected contract facts from them and semantic review is mandatory when native-runtime integration behavior changes.

Current-state project records such as the backlog and acceptance ledger must also remain compatible with these normative documents. Historical changelog entries may describe superseded states when they are clearly historical.

## Machine-verifiable contract gates

Run:

```bash
python3 scripts/verify-native-doc-sync.py
```

to verify current source/document anchors.

For ABI layout/consumption behavior, also run:

```bash
python3 scripts/verify-native-abi-contract.py
```

This compiles and executes `app/src/test/native/native_abi_contract_test.cpp` with an available host C++17 compiler. It checks the frozen descriptor, old mandatory API prefix, optional capability tail, host/input/eye prefixes, shell-locomotion capability gating, overflow-safe pointer-ray sanitization (including NaN, infinity, and truncated prefixes), and GLES major/minor admission without needing Android or GLES libraries.

When validating a completed commit against its parent, run:

```bash
python3 scripts/verify-native-doc-sync.py --base HEAD^
```

For staged work before commit, run:

```bash
python3 scripts/verify-native-doc-sync.py --staged
```

The documentation verifier has three jobs:

1. compare key implementation constants/names against factual claims in the native docs;
2. reject a mapped implementation-source change that lacks a same-change update to its mapped documentation;
3. require the living systems catalog to remain discoverable across every standardized mapped source/document plus selected proven shell-only generation references.

The ABI executable test independently exercises the header/helper compatibility rules. Both supplement review; neither can prove that every semantic or device-runtime claim is correct.

## Review rule for non-machine-checkable claims

For behavior that cannot be derived mechanically—comfort, GL-state ownership, shell recovery behavior, thermal conclusions, interaction conventions, or architecture—the implementation change must explicitly inspect the relevant handbook section.

If the behavior changed, update the wording. If device evidence invalidates a claim, downgrade or correct the claim immediately.

Measured performance/thermal statements must identify their evidence state. Unmeasured improvements remain hypotheses or implementation facts, not measured benefits.

## Removing or replacing a capability

When an implemented API/service/utility is removed or superseded:

1. update authoritative source;
2. update its normative documentation in the same scoped change;
3. remove it from current capability lists or mark it historical;
4. update the contract map if ownership/path changed;
5. update backlog/acceptance evidence where the removal changes admission state.

A deprecated capability may remain documented only when its compatibility behavior remains true.

## Documentation is part of acceptance

A native implementation unit is not complete when code alone passes. Its documentation contract must also be current.

For native SDK-facing changes, the completion evidence should include:

- implementation/source validation;
- `verify-native-doc-sync.py` result;
- `verify-native-abi-contract.py` result for ABI/helper changes;
- relevant deterministic tests;
- build/package evidence when applicable;
- device/runtime evidence when the claim requires it;
- synchronized handbook/API/standard wording.
