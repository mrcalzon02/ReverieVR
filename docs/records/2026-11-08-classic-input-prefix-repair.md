# Classic native input: old-prefix memory-safety repair

Date: 2026-11-08.

## Defect and fix

The classic module validated only the mandatory input prefix before copying the entire current `ReverieNativeInputV1`. A valid old base input contains 24 bytes; the current structure contains 80. The full copy read beyond the caller's allocation even though that caller met the ABI contract.

The integrated source now zero-initializes its local copy and copies at most the smaller of the advertised input size and the current structure size. Existing sanitizers then process the local structure. Absent optional pointer/listener fields remain zero. The fix changes no game rule or ABI layout.

```cpp
ReverieNativeInputV1 copy{};
std::memcpy(&copy,input,std::min<size_t>(input->struct_size,sizeof(copy)));
```

## Reproducible production probe

```sh
bash scripts/verify-classic-input-prefix.sh
```

The script requires a C++17 compiler with AddressSanitizer and UndefinedBehaviorSanitizer, Khronos GLES2 headers, GLESv2 and zlib. On a host with those dependencies outside standard locations, supply `CLASSIC_TEST_GL_INCLUDE` (headers), `LIBRARY_PATH` (link libraries) and `LD_LIBRARY_PATH` (runtime libraries). It does not require an Android SDK, graphics context, device, workflow or network access.

The probe includes the real module source by its relative include name, allocates exactly each advertised input prefix on the heap, and invokes production Update. The tested sizes are 24 bytes (base), 52 bytes (pointer extension) and 80 bytes (listener extension). It compiles as Snake because the offending shared copy occurs before any of the eight game-specific branches. No rendering callback is called. The real GLESv2 library is linked because sanitizer global metadata retains references to the module API's graphics functions; graphics are not mocked.

The script disables LeakSanitizer by default because the execution environment's tracing arrangement rejects it. This is a bounds/undefined-behavior check; no leak-check pass is claimed. Supported environments may override `ASAN_OPTIONS`.

| Source under test | Compile | Execute | Observed result |
| --- | --- | --- | --- |
| Classic repair checkpoint retaining the old full copy | Exit 0 | Exit 1 | ASan identifies an 80-byte read from a 24-byte allocation in Update. |
| Scratch-only bounded-copy candidate | Exit 0 | Exit 0 | All three prefixes complete without sanitizer diagnostics. |
| Actual authoritative integrated module | Exit 0 | Exit 0 | All three prefixes complete without sanitizer diagnostics. |

The first failing evidence is retained verbatim in [classic-input-prefix-before.log](evidence/2026-11-08-native-games-repair/classic-input-prefix-before.log). The integrated passing output is in [classic-input-prefix-after.log](evidence/2026-11-08-native-games-repair/classic-input-prefix-after.log), with structured scope/status in [classic-input-prefix-results.json](evidence/2026-11-08-native-games-repair/classic-input-prefix-results.json). ASan's label in this runtime is `unknown-crash`; the reported access size, allocation boundary and source location identify the concrete out-of-bounds read.

This check supplements the eight classic lifecycle/save regressions. It does not establish Android or physical-device readiness.
