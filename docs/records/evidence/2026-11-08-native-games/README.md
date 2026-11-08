# Native-game audit: compact repository evidence

This is the compact evidence set for [the audit](../../2026-11-08-native-games-release-audit.md), production baseline `12d4018c`. The complete standalone archive also preserves all attempts, command arrays, captures and probes, and the PDF provides the consolidated report.

`manifest.json` records every copied evidence file and its SHA-256. Reports and logs retain their original execution paths/source references so historical attempts are not rewritten. `release-matrix.json` uses portable baseline-repository source paths and evidence-archive paths. Its complete archive contains those paths; the compact copies here are mapped by the manifest.

## Interpreting results

- The classic lifecycle runs and Snake/Guess/Breakwater expectation probes intentionally expose unmet requirements; their failure is audit evidence.
- Featured module probe success means the scripted assertions/reproductions completed, including defects. It does not label a game release ready.
- The Android artifact is prior-run emulator evidence at `c10ddd72`, with registered game implementation unchanged at the audit baseline. Local GLES captures are software Mesa. Neither is a physical S9/Daydream acceptance result.
- Existing publication policy is unchanged; visual/style/balance remain diagnostics. Full intended-game completion is a separate claim.
- Runner acceptance has both successful recovery and intentionally rejected/corrupted fixture cases. The full archive retains the deliberately corrupted fixture and its original bytes separately.

All game implementation remains unchanged. Audit tools/probe copies are under `scripts/audit/`; no workflow was added or triggered.
