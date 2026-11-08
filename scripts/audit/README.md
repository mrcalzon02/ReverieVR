# Native-game audit diagnostics

This directory preserves the bounded diagnostic runner and production-module
probes from the 2026-11-08 audit. It changes no game implementation or workflow.
The decision record is
`docs/records/2026-11-08-native-games-release-audit.md`; exact original commands,
results, captures and failed attempts are in the accompanying evidence archive.

## Resumable host checks

Run from the repository root with Python 3, GCC and a real JDK supporting Java
`--release 17`. The manifest is a reusable ten-case subset of the original
eleven-case host run. Historical downloaded-Android-capture verification needs
its external artifact directory and remains recorded separately in the archive.

```sh
python3 scripts/audit/checkpoint_runner.py \
  --manifest scripts/audit/host_cases.json \
  --output /tmp/reverie-native-audit
python3 scripts/audit/checkpoint_runner.py \
  --manifest scripts/audit/host_cases.json \
  --output /tmp/reverie-native-audit --resume
```

Exit 0 means every manifest case has a passing final result and its stored
hashed attempts survived verification. Exit 1 preserves actual diagnostic
failures/timeouts or incomplete work; exit 2 rejects invalid/reused/locked
evidence. Handled SIGTERM exits 143 and records interruption. A test fixture
whose successful assertion reproduces a game defect may itself exit zero;
interpret its assertions, not only its process status.

`--only CASE` restricts execution, while all stored hashed attempt logs are
still checked. `--retry-failed` explicitly reruns failures/timeouts and retains
old attempts. A source/manifest change requires a new output directory.

After an unclean kill, inspect the saved process group and stop/reap surviving
diagnostic processes. Then use `--resume --recover-interrupted`. This is an
explicit cleanup acknowledgment, not permission to launch duplicates. The
runner refuses a known live group and passes its output lock to launched
commands. Never automatically kill an unrelated process based on a reused PID.

The cache key covers tracked source excluding `docs/`, `art/` and `.github/`.
It does not authenticate external/untracked fixtures, compiler versions,
environment variables or system libraries. Keep those inputs unchanged when
resuming; otherwise use a fresh output directory and record the environment.
Commands that deliberately detach into other sessions need external supervision.
No exactly-once guarantee is made across a crash before final checkpoint commit.

Checkpoint JSON uses file fsync, atomic replacement and directory fsync.
Completed logs have SHA-256 hashes; every attempt is retained. These operations
protect local process recovery, not erased storage. Preserve bounded evidence
batches independently before risky installs/builds and after material results.

## Production-module probes

`probes/` contains audit fixtures, not alternative game implementations. They
include the actual production module sources and use ordinary update/input and
host save callbacks. Three recent-game include directives were converted from
the audit's absolute checkout path to normal include-path lookup; all other
probe bytes are unchanged. `probe-copy-manifest.json` records both hashes and
that exact transformation. Original executed versions remain in the archive.

Native dependencies are GCC C++17, pthreads, Khronos EGL/GLES headers/libraries
and zlib. Graphics probes used real Mesa with `EGL_PLATFORM=surfaceless` and
`LIBGL_ALWAYS_SOFTWARE=1`, not fake GL. Classic non-rendering probes use dead-code
elimination to omit graphics; that must not be called a render pass.

Example classic lifecycle probe (kind 0=Pong, 1=Snake, 2=Chutes, 3=Checkers,
4=Go, 5=Chess, 6=Breakout, 7=Reversi):

```sh
g++ -std=c++17 -O1 -pthread -ffunction-sections -fdata-sections \
  -Wl,--gc-sections -DCLASSIC_GAME_KIND=0 \
  -I app/src/main/jni/native scripts/audit/probes/classic_lifecycle_probe.cpp \
  -o /tmp/reverie-classic-lifecycle
/tmp/reverie-classic-lifecycle
```

At the audited production revision all eight variants exit 1 for
`FAIL_SAVE_RESTORE`, with zero save reads/writes. This is intentional failing
evidence, not a runner failure to hide. The Snake pointer probe likewise exits
1 for the visible UP/DOWN mismatches. Guess and Breakwater probes exit 1 for
their violated interaction/preservation expectations.

Example real Gumball module render/fault probe:

```sh
mkdir -p /tmp/reverie-gumball-captures
g++ -std=c++17 -O1 -Wall -Wextra -Wpedantic \
  -I app/src/main/jni/native -I scripts/audit/probes \
  scripts/audit/probes/gumball_module_probe.cpp \
  -lEGL -lGLESv2 -lz -o /tmp/reverie-gumball-probe
EGL_PLATFORM=surfaceless LIBGL_ALWAYS_SOFTWARE=1 \
  /tmp/reverie-gumball-probe /tmp/reverie-gumball-captures
```

This probe exits zero when it reproduces the expected interruption loss and
completes rendering assertions. It does not establish release readiness. The
other exact dependency/compile arrays are preserved with their original logs;
adapt only the probe/include/output paths to this directory.

## Verified recovery checks

The archive preserves an original graceful SIGTERM/resume test and a second
independent bounded review. The reviewed runner rejects a corrupted unselected
log, blocks resume while an orphan retains the lock, requires explicit cleaned
recovery after SIGKILL, and terminates a TERM-ignoring descendant on timeout and
SIGTERM. The tested runner SHA-256 is recorded with those results. Reuse of the
same-boot group guard without the inherited lock was source-reviewed, not
independently branch-tested. Physical-device and remote-upload durability are
separate evidence layers.
