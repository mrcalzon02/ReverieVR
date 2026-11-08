# Red Ledger and Gumball Armada persistence repair

Date: 2026-11-08. Implementation starts from `43a1626d40788067128915aa77deb356434ae9b3` in an isolated checkout. This record concerns the existing games and the observed persistence defects. It does not declare either complete game release ready, reduce its intended content, or create an additional development-APK publication gate.

## Behavior repaired

### Between Deliveries / Red Ledger

Every accepted economic interaction remains in memory until the host acknowledges its checkpoint. A rejected write now leaves explicit dirty/error state, displays a separate warning, retries at a one-second interval while updating, and retries on Pause, Resume, and ordinary exit. The module no longer forgets a failed payment collection after the initiating click. An error returned after bytes became visible is also treated as an unacknowledged write: the next attempt writes the identical latest economic state instead of undoing the transaction or paying it twice.

The existing `state-v4.bin` slot now contains a 396-byte checked envelope: eight-byte `RLSTATE5` magic, little-endian version and payload-size fields, the existing 372-byte composite, and a little-endian 64-bit FNV-1a checksum over the complete preceding envelope. The four payload sections remain simulation, trade, renovations, and clientele. The checksum includes the base cash and inventory that the prior composite left unchecked. It detects accidental corruption; it is not cryptographic authentication or protection against a party able to forge a checksum.

Reads decode into candidates and commit the entire composite only after all integrity and semantic checks pass. A corrupt, incompatible, or unreadable checkpoint enters a visible recovery state. Gameplay cannot replace it with a default save. Click retries the read; Resume also retries. The optional host `prepare_exit` callback returns zero while read recovery or unsaved progress remains unresolved, so an updated host can retain the instance and offer an explicit discard choice separately.

The previously supported 332-byte and 372-byte composites still import through their existing strict section and value checks and are upgraded only after a successful read. Historical base bytes had no checksum; a valid-looking historical currency mutation cannot be detected retrospectively. No additional historical save versions are introduced.

The ROOM plaque moves above the status rows. The save warning occupies its own row below the cash/status text. Actual rendered frames verify those localized layout changes.

### Gumball Armada

The module now saves the campaign, exact active sortie, and constructed ship as one 4096-byte transaction in the existing `campaign.v1` slot. `GUMFLY04` identifies the new checked format. An explicit little-endian field codec writes integers, strict booleans, and IEEE-754 binary32 values. It does not persist C++ object layout, padding, pointers, renderer resources, or input edges. Reserved padding must be zero. Integrity verification precedes semantic validation and transactional restoration.

The saved state includes campaign progress and purchases; paid support bookings and deployment; consumed kits; the active mission and cast encounter; RNG, timers and player condition/position; enemies, shots and salvage; score and result accounting; and the complete voxel construction and stock. The live ship profile is preserved and checked against construction in active/result phases. This prevents a paid support contract from disappearing when a live sortie is recreated and prevents campaign and construction writes from partially committing as separate transactions.

An active sortie restored from storage waits on a visible continuation screen. It advances neither combat nor timers until a released primary input is followed by a fresh press. That continuation edge does not fire a shot or advance the saved simulation. Given the same subsequent inputs, the restored production module produces the same next states.

Gameplay mutations and purchases save immediately where the existing module already did so. An active sortie additionally checkpoints at one-second intervals; Pause, Resume with pending changes, and ordinary exit flush the latest state. Failed writes retain dirty state and the latest live simulation, retry at a bounded interval, and display a warning above the main heading. Construction failures explicitly identify unsaved ship edits. The prior warning at the footer no longer overlaps the contextual purchase hint.

Current checked campaign snapshots (56/52/44 bytes) and checked `shipyard.v2`/`shipyard.v1` construction remain supported import sources. The two legacy slots are read into one candidate before the module can upgrade to the full checkpoint. A read error or invalid construction blocks gameplay and replacement. Once a full checkpoint exists, it is authoritative and obsolete construction files are not applied over it. New saves do not write a second construction slot.

## Validation and evidence

The existing per-title local runners include the new format tests:

- `bash scripts/verify-red-ledger.sh`: 12 C++ regression programs passed with GCC C++17, `-Wall -Wextra -Wpedantic -Werror`; Clang was unavailable.
- `bash scripts/verify-gumball-armada.sh`: 12 C++ regression programs passed with the same strict warning policy; Clang and its optional sanitizer run were unavailable.
- `bash scripts/verify-featured-persistence.sh <capture-directory>`: two real production modules, linked to actual EGL/GLES, exercise pointer-driven actions, persistence faults, lifecycle calls, and stereo framebuffer capture. This local diagnostic runner is not a substitute for Android or headset verification.

The Red Ledger format test flips one bit at every byte of the 396-byte envelope, checks every truncated size, preserves the destination state on rejection, and checks both supported old composite sizes. The actual module test performs the real cup/pour/serve/payment loop, rejects the payment save, recovers it during idle retry, and recreates with cash 2480 and no pending duplicate payment. A second service checks Pause recovery. Corrupt checked base currency and read I/O errors remain protected; externally recovered bytes load through the real recovery route. Visible-but-unacknowledged writes retry without duplicating the payment. Stereo framebuffer hashes match after a genuine GL context destruction/recreation.

The Gumball format test covers all nine cast cases across 36 seeded campaigns, varied mission/fortress conditions, paid escorts and kits, construction, thousands of subsequent simulation steps, one-bit corruption in each of all 4096 bytes, and correctly rehashed impossible/NaN data. Result recreation does not award completion cash again. The actual module test buys Eclair through the menu, rejects the launch write, recovers during flight, and restores a 5.600-second live sortie after the escort has acted. Both eye hashes and the subsequent production-module next states match. The same test rejects a real upgrade write and a real voxel edit, then verifies that both campaign and construction survive together after recovery. It also exercises corrupt/read-error recovery, legacy-pair import, unacknowledged visible writes, missing save callbacks, and the exit guard.

Detailed logs and unaltered PNG conversions of framebuffer PPMs are in [the repair evidence directory](evidence/2026-11-08-featured-persistence/). Earlier before frames remain in [the native audit evidence](evidence/2026-11-08-native-games/), including `red-ledger-bar.png` and `gumball-save-failure.png`. The test driver reports Mesa llvmpipe GLES 3.2. Shared optimized shader-diagnostic formatting warnings remain visible in the actual-module compile log; the new codec/simulation suites pass with warnings treated as errors.

## Limits and integration requirements

The module callback depends on the host's appended, size-gated `prepare_exit` ABI support. Older hosts retain their legacy destruction behavior; these modules make a final best-effort write on destruction. A forced process kill cannot save memory that the host has not acknowledged. During ordinary healthy storage, live Gumball combat therefore has at most the periodic interval of unflushed progress between checkpoints; during a storage failure it retains pending state in the live instance and requires recovery or an explicit discard decision before ordinary exit. An arbitrary OS kill during an outstanding failure can still lose that pending memory.

The host owns atomic file replacement and durable acknowledgement. A returned error can occur before replacement or after replacement without final durability acknowledgement. Neither module assumes that error means old bytes are still present. Both retain the latest in-memory state and retry it safely.

These captures exercise the actual renderer with the locally available assets. Logs truthfully report unavailable optional Red Ledger room/patron packs and Gumball route artwork. They do not validate the complete installed downloadable art set, physical VR readability, phone audio, controller comfort, long-session thermals, or full campaign/content acceptance. Those remain independent completion work and diagnostics under the existing publication policy.
