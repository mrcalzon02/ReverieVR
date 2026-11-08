# Breakwater audio host checks

Run from any directory with Java 17 or newer:

```sh
./scripts/audio/verify-breakwater-backend.sh
```

An optional first argument selects another checkout. The runner supports both
`javac` and a Java installation exposing `jdk.compiler` without a `javac` binary.
It compiles to a temporary directory and removes those outputs on exit. It needs
no Android SDK, Gradle, downloaded dependency, device, or checked-in recording.

The runner compiles **current production sources**, not copied implementations:

- `UiFeedback.java` against a deliberately observable bank double verifies
  session generations, pointer loss/Quick Menu, lifecycle, focus loss, fallback
  precedence, exclusive normalized routing, Red Ledger routing, restart, and close.
- `BreakwaterAudio.java`, `BreakwaterAudioPolicy.java`, `MusicVolumePolicy.java`,
  and `UiFeedback.java` together exercise the real bank with small Android-edge
  fakes. Scenarios cover asynchronous and stale decode callbacks, loop lease
  expiry, stereo gain changes without retrigger, isolated voice budgets and
  priority eviction, normalized Quick Menu controls, cooldown commit semantics,
  independent pool replacement with bounded retry, decode/load/playback failures, lifecycle
  cancellation, and real UI-to-bank routing.

The fake Handler advances monotonic time explicitly and scopes cancellation to
the owning Handler. The fake SoundPool implements asynchronous completion,
finite streams, natural one-shot expiry, and explicit fault injection. It records
actual stream gains and categories for the simultaneous effects/music headroom
check. AssetManager supplies canonical PCM fixture lengths; the production asset
verifier must independently check the real PCM peak ceilings used by that test.

These checks verify host decisions and integration. Android's decoder, route
changes, real AudioManager focus delivery, audible loop joins, balance, and
latency still require the unified APK/device listening pass. The harness does not
claim those hardware results.
