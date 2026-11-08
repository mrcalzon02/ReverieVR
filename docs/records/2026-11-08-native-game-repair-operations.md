# Native game repair operations — 2026-11-08

## Intent and starting evidence

Owner instruction: begin extensive repairs and productive development after
the per-game audit. Production baseline is
`12d4018c90e56a3767b8ffc58a2e5394440caec1`; the preserved audit checkpoint is
`43a1626d40788067128915aa77deb356434ae9b3`. Root owns the authoritative local
main checkout. Three isolated local clones implement classics, Red/Gumball,
and Breakwater/Guess/Test Chamber. They do not commit, push or modify the
authoritative checkout. Root serially reviews and integrates each patch.

## Shared host repair checkpoint

- Quick Menu and headset recovery now pause native simulation. Activity
  and menu reasons are independent; only releasing the final reason resumes.
  The frame clock resets so menu time never becomes catch-up simulation.
- The production input binding engine clears held outputs and queued taps
  on focus loss, still observes releases while suppressed, and requires a
  held button/axis to release or return to neutral before gameplay resumes.
- Optional ABI-v1 `prepare_exit` is an append-only, size-gated callback.
  Ordinary Home, Back and Exit Phone retain the live instance on refusal.
  The menu offers retry, continue, or an explicit two-step Exit Anyway /
  Confirm Exit with possible progress loss. This does not promise a rollback
  of disk bytes after a post-rename error. New
  module launch also refuses to destroy an unresolved session.
- Omega Front participates in the exit check and flags encode failures.
  Unreadable prior state can be retried before deciding whether exit is safe.
  Its existing checked 2,048-byte campaign format is unchanged.
- Save success now includes file flush, atomic rename and parent-directory
  flush. A post-rename error leaves the latest live state pending for retry;
  successful re-reading alone does not prove durability. Non-regular save
  objects are rejected as I/O errors, never treated as an empty save.
- Brief primary/secondary taps completed between frames now reach one native
  update through the existing bus capture contract, then expire normally.
- The stale documentation verifier recognizes shader/log validation through
  the safe shared wrappers, additional GL restores on failure paths, the
  existing central native profile resolver, and Markdown emphasis around
  unchanged capability limits. It still requires the shared implementation,
  profile default and both GL capture sites; no publication workflow is added.

## First integrated checkpoint verification

The first integrated checkpoint passes **41 JUnit tests** against the production
binding engine/bus/profile. The real `NativeModuleRuntime` Java class passes
**27 lifecycle/dispatch assertions** using an explicitly named counting JNI double. The latter
verifies actual Java update suppression, pause composition, reset timing,
save refusal and refused replacement; it is not a native-module or Android
runtime test. It includes inter-frame taps, refused replacement focus restoration
and paused Sitka touch suppression. The production C++ host passes
**22 persistence/exit assertions**
with real POSIX I/O and explicitly injected file/directory fsync failures;
Android asset and log functions are named shims, so this is not handset proof.
The native ABI executable passes old-prefix, truncated-tail,
null callback and new-callback cases. New assertions were added to the
existing Android hub instrumentation method without changing its test count;
they are pending execution on the integrated repair revision.

The first Java compile attempt omitted two existing profile dependencies.
The first executed suite exposed a test fixture using the wrong native
secondary source; the fixture now uses the actual Volume Up binding. The
production controller layout was not changed to satisfy that test.

## First-checkpoint limitations (superseded by integrated result below)

Game-lane repairs and their fault-injection/real GLES captures are being
validated independently. Do not claim all modules repaired from this shared
checkpoint. Current Android build, installed hub, physical S9/Daydream,
thermal and save-failure-menu visual proof remain outstanding. Forced OS
destruction cannot preserve a write that storage never accepted. Existing
development-game visibility and publication gates remain unchanged.

Next: integrate proven per-game checkpoints, execute combined native and
Java diagnostics, retain capture/log hashes, then record the exact final
commit and any remote publication result.

## Integrated repair result

All sixteen registered native modules now have a checked persistence/recovery
path and participate in the optional save-aware exit callback. All eight
classics, Guess, Lantern and Forward gain persistence that was absent at the
audited baseline. Source-only Sitka receives ABI initializer compatibility
only and is not registered or claimed playable by this repair.

The concurrent Breakwater audio source at `b2365add` is preserved by merge.
The actual production Update retry race was independently reproduced and
fixed: a recovered automatic save coinciding with fresh primary/secondary
input cannot buy twice or begin a wave. A failed purchase in the current
frame also blocks a secondary launch. Audio fixtures now assert these
intentional save guards and checked 88-byte records. Merged Breakwater
audio passes 2,484 checks; a fresh actual GLES run verifies all three
stations, both-eye guides and context recreation against matched source hashes.

Red Ledger also gains bounded four-stream ownership and shutdown on pause,
focus/session loss and close. Its approximately 906 ms drawer clip was
reproduced playing after a pause at 100 ms before the fix. The real Java
audio source passes 12 scenarios / 75 checks using named platform doubles;
existing Breakwater routing (18 checks) and backend (16 scenarios / 226
checks) remain green.

**Final combined source verification: 21 / 21 groups passed.** The main
checkpoint contains 20 groups; the independent Red Ledger audio lifecycle
checkpoint contains one. Both record the identical tracked-source fingerprint,
and no source edits occurred during these runs. Repeated earlier groups are
not counted as additional unique tests. The final record retains 41 JUnit
binding/bus/profile tests, 27 real Java-runtime assertions using counting JNI,
22 actual native-host persistence/exit assertions, all eight actual classic
modules, 12 Red and 12 Gumball rule/codec programs, actual featured/recon/recent
module probes, short-input ASan/UBSan, five existing Breakwater/workshop rule
programs, identity/launch/docs/ABI gates and real Mesa GLES capture evidence.

See the classic, featured, recon and Red Ledger audio records dated
2026-11-08, `docs/project/NATIVE_RECENT_GAMES_REPAIR_2026-11-08.md`, and
`docs/records/evidence/2026-11-08-native-games-repair/` for preserved evidence.

**Still open:** a new Android build and installed-hub replay on the integrated
repair revision, physical Galaxy S9/Daydream/controller interaction and
listening, sustained frame/thermal behavior, real storage interruption, and
each project's remaining intended content/full-run acceptance. Earlier
Android and the concurrent audio-only release do not certify this revision.
No completed-game, new APK release or physical acceptance is claimed.
