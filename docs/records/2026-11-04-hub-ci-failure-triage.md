# 2026-11-04 — ReverieVR hub / Breakwater CI failure triage

## Intent and evidence rule
The user requested classification of repeat failures as obsolete assumptions,
legitimate source failures, or infrastructure regressions; repair the responsible
source without suppressing real tests. This record is **source-stage evidence**,
not Galaxy S9/Daydream headset acceptance.

## Findings and targeted fixes

1. **Invalid hub workflow YAML; no jobs generated.** The Android emulator
   `script: |` ended early because four lines (comment, PNG readback
   requirement and two verified-pack assertions) were indented at ten
   spaces instead of twelve. Runs
   [41952679331](https://github.com/mrcalzon02/ReverieVR/actions/runs/41952679331)
   and multiple earlier runs failed before any job started. Commit
   [7415449](https://github.com/mrcalzon02/ReverieVR/commit/74154499ccf04ffe142872b366e499086ebf4d5a)
   repaired only block-scalar indentation, retaining the full asset checks.
   The subsequent hub run successfully started `verify-installed-hub`.

2. **Namespace error in C++ regression source.** Hub run
   [41953486715](https://github.com/mrcalzon02/ReverieVR/actions/runs/41953486715)
   compiled sources and exposed calls to `EnvironmentFor`, `BeachHeight`
   and `RearGroundHeight` without the declared
   `reverie::breakwater` namespace. Commit
   [de4870b](https://github.com/mrcalzon02/ReverieVR/commit/de4870b99bc8f6511deab36dd1dbced5231792a2)
   qualified the calls; the test then compiled and ran.

3. **Obsolete beach/bunker geometric expectations.** Newly integrated
   BeachHeight rear blending meets the existing surrounding
   RearGroundHeight (~-1.12 m), so the former -0.86 m rear shoulder and
   -1.10 m minimum-mesh-height assumptions no longer describe the
   permitted continuous landscape. The earlier bunker triangle-count
   minimum also assumed triangles now represented by genuine boxes and
   textured cladding. Commits
   [d640f61](https://github.com/mrcalzon02/ReverieVR/commit/d640f6157f793dfab5cc11e241a5f10422cd60bd),
   [d930c45](https://github.com/mrcalzon02/ReverieVR/commit/d930c4518e5974e8a35ee65deb8a1c31cb9cae97)
   and [000e475](https://github.com/mrcalzon02/ReverieVR/commit/000e47584855f9755f352e63bad7acce611cf44f)
   replaced those assumptions with four-location raised-forward-shoulder
   checks, exact 0.00001 m joined-edge checks, a 0.02 m near-edge
   continuity check, a bounded -1.13 m floor, and a >=50 solid box /
   >=6 triangle bunker-geometry minimum. Gameplay, material/RGB, and
   count assertions remain active. The all-integrated-game functional
   suite [41954767178](https://github.com/mrcalzon02/ReverieVR/actions/runs/41954767178)
   subsequently **passed**, including the Breakwater job.

4. **CI starvation under active source updates.** Prior hub concurrency
   used `cancel-in-progress: true`, cancelling a compiling Android job
   (e.g. [41954249441](https://github.com/mrcalzon02/ReverieVR/actions/runs/41954249441))
   before emulator test evidence could be collected.
   Commit [d25760a](https://github.com/mrcalzon02/ReverieVR/commit/d25760a27c9be5e6c5363b5bb0021f98a0870699)
   retained the existing one-workflow concurrency group but switched to
   `cancel-in-progress: false`. Active runs can complete; GitHub may
   coalesce queued newer revisions. No new workflow was created.

5. **Independent companion and release history.** The Breakwater
   companion [41954301851](https://github.com/mrcalzon02/ReverieVR/actions/runs/41954301851)
   passed both actual Android ABI/package and signing-identity jobs.
   Earlier phone-test runs failed to publish with
   `HTTP 403: Resource not accessible by integration` despite successful
   Android signing; this is a genuine publish-stage error, not a game
   compile failure. A later `phone-test-1858-1` prerelease was
   successfully published with both signed APKs and all required
   supporting assets, so permanent release inability is **not established**.

## Observed Android 10 emulator acceptance and remaining hardware gate
The hub emulator run
[41954301895](https://github.com/mrcalzon02/ReverieVR/actions/runs/41954301895)
**PASSED** on source `a26917005cb54c3ed099afc775915cd3c79ea31b`.
Every step of `verify-installed-hub` completed successfully,
including actual Android APK construction, installed-app instrumentation
(`OK (13 tests)`), 15 independent registered games producing verified
512x512 Android GLES2 captures, native menu capture CSV cross-check,
Omega Front travel/battle/save-resume and 16/16 Breakwater materials.
The runtime logs confirmed the five prescribed Breakwater BWM1 models
were read through verified asset paths. Artifact
[`reverie-hub-game-acquisition-41954301895`](https://github.com/mrcalzon02/ReverieVR/actions/runs/41954301895/artifacts/13381639775)
is retained, 157,192,590 bytes.

A newer hub run
[41954767199](https://github.com/mrcalzon02/ReverieVR/actions/runs/41954767199)
was queued to verify the later source-only regression changes and
non-cancelling workflow policy. Recheck its final state before claiming
the exact latest main revision has Android emulator proof.

**Still unverified:** physical Galaxy S9 + Daydream input/optics,
controller shake-to-headset-forward behavior, beach visual integration
and sustained thermal performance. Those gates require device evidence.
