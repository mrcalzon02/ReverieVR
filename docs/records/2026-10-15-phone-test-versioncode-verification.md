# Phone Test #173 verification failure: versionCode contract
Date: 2026-10-15

## Observed failure (not an Android compilation failure)

Run [#173](https://github.com/mrcalzon02/ReverieVR/actions/runs/38665868928),
source `4f260a515aaf5c915fe8880ca89f726bd1d481aa`, completed
Android unit/assembly and signed `assemblePhoneTest` for both packages.
The verification stage listed all native objects, passed 24 native ELF
export checks and printed the headset's valid distribution signing
certificate. The step then exited 1 without publishing.

The intervening `app/build.gradle` changed the headset install
`versionCode` from `run * 1000 + attempt` to `buildEpochSeconds -
1577836800` so later APKs can update an installed build from an older
GitHub workflow with a higher run number. The existing release verifier
still demanded `run * 1000 + attempt`. The two contracts were inconsistent.

## Root-cause correction

The signed build records a single UTC build epoch and passes it as
`-PphoneTestBuildEpochSeconds` to Gradle. Both headset and optional
controller APKs now compute the same timestamp-based install
`versionCode` from that explicit input. Verification reads the exact
same build-step output and independently checks `aapt dump badging`
for *both* packages, in addition to existing certificate/native checks.
On a mismatch it emits an actionable error. This does not skip or weaken
any verification; no new workflow was added and signing remains unchanged.

Preserve run number and attempt for readable release names and embedded
build metadata. This is a build/release repair; 3D headset tracking,
DOS controller mapping, battery HUD, optical viewports and native games
remain unmodified.

Evidence gate: compare remote commit and workflow success; verify
published APK asset names/sizes and release target SHA. Device install
and interactive headset acceptance require hardware.
