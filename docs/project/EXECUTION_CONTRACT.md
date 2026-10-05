# ReverieVR Repository Execution Contract

**Role:** EXECUTION CONTRACT

## 1. Preflight before mutation

Establish:

- canonical repository and remote;
- current branch and full HEAD SHA;
- fresh remote `main` SHA when possible;
- dirty/staged/untracked/unpushed state when a worktree exists;
- relevant project instructions, active state, backlog target, and acceptance state;
- available toolchain/runtime/device access;
- bounded target and acceptance checks.

Conversation history is not repository evidence.

## 2. Main-only and single-writer

- Work on `main` unless explicitly changed by the user.
- No side branches or PR workflow by default.
- No force-push/history rewriting.
- No GitHub Actions without explicit authorization.
- Only one mutation path may alter authoritative source/state at a time.
- Read-only investigation may run concurrently when it cannot mutate shared state.
- Refresh/reconcile remote state before commit/push when concurrent change is possible.

## 3. DEEFM

For every material operation:

1. **INTENT** — objective, starting authority, expected outputs, acceptance checks.
2. **EXECUTE** — perform the scoped operation.
3. **OBSERVE** — capture actual tool/build/runtime/Git output.
4. **VERIFY** — independently inspect destination state.
5. **CLAIM** — state only what the evidence proves.

Use exact states: drafted, worktree edit, staged, committed, pushed/remote, device-validated, released.

## 4. Editing discipline

- Inspect before editing.
- Prefer minimal coherent edits.
- Do not overwrite concurrent/unrelated work.
- Repair authoritative source, not generated/package output.
- Do not create parallel implementations to evade the existing architecture.
- Do not weaken validators/tests/gates to obtain a pass.
- Do not silently swallow failures.
- Large-file edits require exact targeting and post-edit integrity checks.

## 5. Dependency/framework discipline

Before adding a framework or library, record:

- purpose;
- exact version/source;
- license;
- minimum Android/API implications;
- architecture/ABI implications;
- offline/build-time requirements;
- binary size;
- runtime/performance cost;
- whether it remains maintained enough for our use;
- replacement/removal path if it becomes unavailable.

Prefer the smallest dependable stack that satisfies the reference hardware. Avoid heavyweight dependencies whose main value is visual sophistication ReverieVR deliberately does not need.

No dependency may quietly reintroduce a required discontinued Google service.

## 6. Performance validation ladder

Performance is validated progressively:

1. static/code review for obviously wasteful design;
2. desktop/emulator/unit checks where applicable;
3. APK instrumentation for frame timing, render scale, memory, and thermal/performance signals available from Android;
4. reference-device short-run test;
5. reference-device sustained/thermal test;
6. experience-specific stress test.

A visually correct feature may be rejected for unacceptable sustained performance.

## 7. Input validation

Input must be expressed as actions rather than raw-button assumptions. Verify:

- head-look/recenter behavior;
- reticle selection;
- trackpad navigation;
- primary and secondary actions;
- Home behavior where accessible;
- volume-side-button behavior where accessible;
- remapping;
- graceful handling when a control is absent.

## 8. Commit discipline

Before commit:

- inspect complete scoped diff;
- run available checks;
- confirm docs/ledger match evidence;
- refresh concurrency state;
- stage only intended paths;
- inspect staged diff;
- commit with an outcome-oriented message;
- verify the commit contains the intended changes.

Do not commit known failing implementation unless an explicit checkpoint policy is authorized.

## 9. Push/remote verification

Before push/update, refresh remote `main`. If advanced, reconcile without force.

After push/update:

- independently read remote `main`;
- confirm it contains the intended commit;
- confirm no unexpected branch/ref was created;
- record remote evidence in execution state/closeout.

## 10. Genuine blockers

A blocker requires something material such as:

- missing authorization;
- unavailable protected credential/access;
- destructive ambiguity risking user work;
- unavailable authoritative source;
- irreconcilable concurrent edits;
- required hardware/runtime evidence not available and no substitute can decide the gate;
- persistent capability failure after bounded retries and fallback.

A first timeout or tool failure is not a blocker.

## 11. Closeout

Report:

- repository/branch;
- starting and ending revisions;
- changed authoritative paths;
- validation performed and results;
- commit and remote evidence;
- runtime/device validation state;
- acceptance state;
- next exact action;
- genuine blocker/resumption condition if any.
