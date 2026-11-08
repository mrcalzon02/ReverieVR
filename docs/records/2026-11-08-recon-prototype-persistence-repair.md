# Lantern Desk and Forward Detachment persistence repair

Date: 2026-11-08. This extends the [Red Ledger/Gumball repair](2026-11-08-featured-native-persistence-repair.md) on the existing native module ABI. It repairs continuity and the observed consequence reset in the two Development-only decision-board prototypes. Their intended seated intelligence workspace and physical reconnaissance game remain future work. This change does not alter the existing development-APK publication policy or add a visual/style/balance CI gate.

## Durable current phases

Each module uses its own host-scoped `session-v1.bin`. The new game-private codec writes exactly 128 bytes: `RECON001` magic, version 1, a separate title kind, explicitly encoded current state, zero reserved space, and a 64-bit FNV-1a integrity checksum over the first 120 bytes. Integer fields use fixed little-endian encodings and boolean fields accept only 0/1. The checksum detects accidental corruption, not deliberate forgery by an actor able to recompute it. No previous save formats existed for these prototypes, so no historical migration is introduced.

Lantern Desk stores its phase, assigned sector, observed photograph and clarity, cumulative credibility and operational effect, the already filed result, and the selected menu action. Forward stores its phase, route, decision, condition, fuel, information result, unnecessary-force consequence, menu selection, and whether its separate base-service confirmation is open. Observable contact clues and the debrief view are rebuilt from that same restored current state. GL resources, raw object layout, input edges, and pointers are never persisted.

A read first verifies the complete envelope, strict fields, phase/selection coherence and the current prototype's evidence/resource constraints. It commits only after the whole candidate is valid. A corrupt or unavailable save displays recovery instructions, blocks gameplay replacement of the original, and permits a fresh Select or Resume to retry the read. The existing observation boundary remains intact: the displayed photograph/contact evidence contains observations, not a newly exposed hidden correctness label.

Every accepted action and menu selection checkpoints its complete state. A rejected write leaves the latest state in memory, shows a dedicated warning above the title, and retries at a one-second interval during updates and on Pause, Resume, and ordinary exit. An error after new bytes are already visible still means durability is unacknowledged; retrying writes that same current state. The appended `prepare_exit` callback returns zero until recovery or a successful flush makes ordinary destruction safe. The updated host owns retaining the instance and any separate explicit discard choice.

Recreated or resumed boards require controls to be released before a fresh action. A held input cannot receive a photograph, file a report, resolve a contact, or confirm a refit merely because the module was recreated.

## Lantern Desk: preserve cumulative consequences

`NextSortie()` now transitions the existing simulation from Filed to Briefing while clearing only the completed sortie's evidence/result view. It keeps cumulative credibility and operational effect. Current totals appear on briefing and in-flight views and above the retained photograph on the filed-report view. A report remains once-only: recreating Filed neither resubmits it nor awards its effect again.

The existing three fixed sectors, deterministic 16×16 photographs and four assessments remain the prototype's actual content. Persistence does not turn these repeating cases into a changing operational campaign. Physical desk interaction, annotation, dossiers, richer intelligence consequences and campaign completion are still outside this repair.

## Forward Detachment: explicit resource continuation and refit

NEXT PATROL now returns to Briefing with the actual remaining fuel and condition. It does not refill or repair. Briefing shows both values and the selected route's total round-trip requirement: 10 fuel for Main Road, 16 for Ridge Bypass. Deployment requires a usable vehicle and enough fuel to return. Repeated patrols cannot drive fuel negative; damage clamps at zero, and a disabled vehicle cannot depart again.

BASE SERVICE is a separate third briefing option. It opens an explicit confirmation showing **REPAIR TO 100 / REFUEL TO 20**, with **CANCEL** initially selected. Cancel changes neither value. Only a deliberate confirmation calls `RefitAtBase()`. This is a clearly exposed prototype service action; no resupply economy, repair inventory, time cost, squad campaign, or vehicle recovery simulation is invented. The full intended logistics loop remains development work.

This gives the limited existing patrol loop a way to recover at zero fuel without disguising a resource reset as another patrol. The current route/contact/decision/return/debrief sequence and ambiguous contact observations remain intact.

## Executed validation

Run `bash scripts/verify-recon-persistence.sh` for the two existing strict kernel tests and the new strict checkpoint/continuation test. Add `--gles <capture-directory>` for both actual production modules, each linked with only its own simulation source, matching `Android.mk`. The real module runner uses EGL/GLES and the game-private fault-injection host introduced in the first repair lane.

The strict tests exercise every sector/assessment and every route/decision, current-phase and menu round trips, cumulative totals, once-only report/debrief behavior, resource-preserving continuation, safe deployment bounds, explicit refit, one-bit corruption at every byte, every truncated length, cross-title rejection, correctly rehashed impossible fuel/condition/evidence, and transactional preservation on failed decoding.

The actual Lantern module test advances through all four phases, saves/restores the selected sector and assessment, and compares both eye images across recreation. It rejects a real report write, demonstrates bounded idle recovery, retains the photograph and filed result, and starts the next sortie with trust+3/effect+2 still present. A second real report reaches cumulative trust 5/effect 3. A visible-but-unacknowledged write then retries without duplicating that report's effect.

The actual Forward module test traverses all six phases with a real Engage decision. NEXT PATROL restores condition 75/fuel 10. Attempting the 16-fuel ridge leaves the briefing unchanged and shows the base-service guidance. Opening service and selecting Cancel leaves 75/10 unchanged. A normal second road patrol ends at 75/0; further deployment is blocked. Only the separately confirmed service changes the state to 100/20. Its uncertain save acknowledgement remains dirty and guarded until the identical checkpoint is acknowledged. The test also restores the service-confirmation screen itself.

Both actual modules reject corrupt files and read I/O failures without writing over them, reload externally recovered bytes, reject unsafe ordinary exit, and prevent held input from advancing a restored phase. Every recorded before/after phase recreation has identical framebuffer hashes for both eyes. The final successful run and representative unaltered PNG conversions are in [the recon repair evidence](evidence/2026-11-08-recon-persistence/). Intermediate compile corrections are retained in the evidence: an indentation warning in the new codec and a duplicate local variable in the new module test were fixed before the successful production-module runs.

## Evidence limits

The driver is real Mesa llvmpipe GLES 3.2 in an offscreen host. These are genuine renderer output and executed production code; they do not establish Android ARM packaging, physical headset readability, controller comfort, audio, thermal performance, spatial gameplay, or completion of the intended full games. Shared optimized GL diagnostic formatting warnings remain visible in the actual-module build log; the kernel/codec tests pass with GCC warnings treated as errors.

The host supplies atomic replacement and durable write acknowledgement. The modules cannot make an arbitrary process kill preserve unacknowledged memory during a storage outage. The guarded ordinary-exit behavior depends on the host's size-gated `prepare_exit` support. These constraints are distinct from the demonstrably repaired current-state continuity and explicit resource handling.
