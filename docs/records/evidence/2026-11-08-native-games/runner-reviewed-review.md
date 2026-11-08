# Patched checkpoint runner: bounded regression re-review

**All three requested regression checks passed.** The harness exited 0. This is a follow-up to the earlier read-only review; the original acceptance evidence and original review have not been altered. No production source was edited by the reviewer.

Reviewed/tested runner SHA-256: `ac28f2a3a71a0902160665db0b52f3893c476e4cdfa29a2f6987ef92e6c31fad`.

The exact source is preserved in `checkpoint_runner.reviewed.py`. `reviewed-source.json` records the working commit and invocation. `commands-and-results.jsonl` records every actual runner argument array, result, signal and relevant hash; `harness.log` contains the complete transcript; per-case manifests/checkpoints/raw logs are in their named subdirectories. The full regression driver is `run_regressions.py`.

## Executed command

```text
python3 /workspace/scratch/d76b27d6a834/audit-work/runner-acceptance-reviewed/run_regressions.py
```

Individual actual invocations use the resolved Python interpreter and the production script, with separate manifests/output directories. They are preserved as argument arrays in `commands-and-results.jsonl`; no shell interpretation is involved in the tested runner calls.

## Results

| Required check | Observed result | Evidence |
|---|---|---|
| Corrupt unselected completed log | Initial A/B run passed. After intentionally corrupting only B's scratch log, `--resume --only a` exited **2** with `Completed evidence missing or changed: b`. Neither case reran and the checkpoint bytes were unchanged. | `01-corrupt-unselected/resume-rejected.log`; original B bytes kept in `b-original.log` |
| Parent SIGKILL while child lives | Runner exited **-9**, child continued. Ordinary resume and resume with `--recover-interrupted` both exited **2** while the live child retained the output lock. No second attempt or execution was created. | `02-parent-sigkill/blocked-live.log`, `blocked-live-explicit.log`, worker counter and command ledger |
| Explicit cleanup/recovery | The harness explicitly killed and reaped only the recorded fixture group. Plain resume still exited **2** for unclean termination. `--resume --recover-interrupted` then exited **0**; original attempt is interrupted, second passed; execution counter is exactly `xx`. Both attempt log hashes match. | `02-parent-sigkill/blocked-no-ack.log`, `recovered.log`, `evidence/checkpoint.json` |
| Timeout with TERM-ignoring descendant | The group leader used default TERM handling; its descendant installed SIG_IGN for TERM. Runner timeout exited **1**, recorded `timed_out`, and killed the descendant without a harness kill. | `03-descendant-timeout/run.log`, checkpoint and descendant readiness record |
| SIGTERM with TERM-ignoring descendant | The same process structure received SIGTERM at the runner. Runner exited **143**, recorded `interrupted`, and killed the descendant without a harness kill. | `03-descendant-sigterm/run.log`, checkpoint and readiness record |

The test harness became a Linux child subreaper solely so it could reap intentionally orphaned/dead fixture processes. In the two descendant-cleanup tests, **the runner sent the terminating signals**; the harness only reaped already dead children. No fixture processes remained after cleanup. The SIGKILL test deliberately uses external cleanup, which is the contract represented by the explicit recovery flag.

The deliberately corrupted B log remains corrupted as test evidence, with its original bytes archived separately. Its mismatch is expected and is not a damaged production audit log. All other attempt hashes checked by the harness match.

## Supported claim after the patch

For these local diagnostic commands, the runner now:

- Verifies stored hashed attempt logs before execution filtering; filtered resume cannot quietly overlook the exercised unselected-log corruption.
- Preserves output-directory exclusion while the launched command retains its inherited lock, even after the runner itself is killed.
- Fails closed on an unclean recorded running attempt; recovery requires explicit acknowledgment after cleanup. The command identity includes process group and boot ID.
- Stops the tested process group on timeout or handled SIGTERM even when its leader exits before a TERM-ignoring descendant.
- Retains both interrupted and completed attempts with matching log hashes, and does not relabel interruption/time-out as pass.

The earlier two reproduced/source-established blockers are closed for these exercised paths. **No remaining blocker was reproduced by the requested bounded checks.**

## Boundaries to retain in the final wording

The live-orphan rejection exercised the inherited-file-lock protection. The separate same-boot group-existence rejection is present in the reviewed source but was not independently reached while bypassing that lock. These are complementary safeguards, not two independently tested rejection branches.

The source fingerprint still covers tracked files while excluding `docs/`, `art/`, and `.github/`; external/untracked data, environment values and toolchain/library identity are not part of that cache key. Resume therefore requires unchanged relevant inputs, and a new output directory when those unrecorded inputs change. Do not describe the runner as universal input-equivalence detection.

These checks do not establish recovery from erased storage, successful remote uploads, exactly-once execution across loss before the final checkpoint commit, or supervision of commands that deliberately detach into other groups. `--recover-interrupted` is an explicit cleanup acknowledgment, not proof of arbitrary external-process cleanup. The parent's separate durable evidence archive remains necessary for workspace loss.
