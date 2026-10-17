# Breakwater Battery VR — canonical native game specification

**Status:** Development-only VR prototype, physical Galaxy S9 acceptance pending. **Backlog:** RV-0640. The authoritative implementation is in app/src/main/jni/native/breakwater_campaign.{h,cpp} and breakwater_module.cpp. This game uses original assets/code and does not import Beach Head content.

## Pitch and campaign structure

A stationary coastal-defense gun battery faces a gradually escalating amphibious invasion. The player is seated, scans the horizon by turning their head, and aims the gun using the shell-calibrated handheld pointer. No artificial walking or jolting camera recoil.

The campaign has three defended locations: **Shingle Bay (two days), Iron Quay (three days), and Blackcap Island (four days)**. Every day and night is a separate wave period with a shop after **every** wave. The expanded Casual/Regular/Veteran/Siege opening schedules are **12/14/16/18 daytime waves** and **8/10/12/14 nighttime waves**, increasing by one per additional day within a location and two per successive location, with a safety ceiling of 32 waves per day/night period. Across nine days and eighteen periods, the full campaigns now contain **244/280/316/352 separate waves**. Each wave has its own shop; the number of simultaneous craft remains capped at ten and soldier pools remain bounded. This expands *campaign length*, not the per-wave memory budget. All counts remain representable in the existing 80-byte shop checkpoint and uint8_t live wave index.

Briefing → inter-wave Shop → Combat → Shop → next wave → sunset/night → sunrise/day → next location → Victory. Integrity falling to zero means Defeat. Fatal aircraft strikes, offshore gunboat salvos and gunboat shoreline collisions end the current simulation substep immediately after recording real damage. Later active attackers and infantry do not move, fire or disembark after the battery is destroyed; the terminal impact event remains available to the renderer. Native regression holds a second already-spawned attacker stationary on a fatal salvo. Nothing automatically skips the shop.

The native input contract permits frame deltas up to 0.10 seconds. The campaign now advances that time in bounded 0.05-second substeps, rather than silently dropping half the elapsed time at 10 FPS. This preserves boat movement, spawn cadence, reloads and hostile attack timing on slow frames, while capping long pause/resume catch-up to 0.10 seconds. Deterministic native regressions compare one slow frame against two half-frames, including offshore gunboat salvos. Headset frame-pacing acceptance remains outstanding.

## 2026-10-16 — Event sound effects and original background music

**Mars des Souvenirs** is the owner's original commercially authorized passive Breakwater track. The master MP3 in music/source/audio is SHA-validated and staged by the existing Gradle music task. The hosted breakwater-battery route selects and loops it offline through the existing ShellMenuMusic library with separate music mute/volume. It is not a CC0 SFX, is not repackaged twice, and does not require a new player.

The original five weapon reports remain, and ten additional original deterministic PCM16 one-shots now respond to hull damage, shot splashes, destroyed vessels, gunboat salvo, shore infantry volley, bomber explosion, strafing fire, SAM launches, successful wave starts and shoreline collision. C++ triggers the cues only on actual game actions or positive-damage event sequence transitions. The host queues at most eight ordered sound IDs each update and Android preloads the short 22,050 Hz waveform bank once. Stereo-eye rendering never triggers audio. External CC0 recordings in the sound candidate register were not imported. Actual headset mix, timing and battery/thermal acceptance remain pending.

## Enemy transport rules

**The opening enemy is a rowboat, not a robot.** A rowboat carries exactly **one infantryman**. Stronger military landing craft appear later, each carrying **three to six infantry**. Ships have real position, speed and hull HP. **Sinking the craft before it beaches cancels every passenger**, while a surviving craft unloads its passengers as separate hostile shore infantry. Each passenger has an individual in-wave soldier slot and world position derived from its landing craft, advances from the surf toward the parapet, and can only be killed by an actual ray hit against that soldier. The old generic beach-region auto-kill is removed. Infantry only damage the battery after individually reaching the forward firing line at z = -1.34; while they are advancing across the beach, their attacks are disabled. The four-second salvo countdown begins when at least one living soldier reaches that line, and killing every ready shooter resets it. Each salvo's damage scales only with soldiers actually at the firing line, not all landed soldiers. Every actual infantry salvo now records a sequence, an eligible firing soldier's world-space position and the number of participating shooters; the VR scene shows a short generated muzzle flash at that position only when the authoritative volley fires. This adds no textures and advances effect lifetime once per update for both eyes. Native regression tests reject imaginary early flashes and confirm the fired event comes from a living soldier on the firing line; reference-headset visibility is still unverified. Later progression introduces gunboats and overhead aircraft. Gunboats fire from offshore once within eight world units, dealing three integrity damage per salvo every 2.8 seconds. Surviving gunboats still cause eight integrity damage upon reaching the shore. A gunboat that actually reaches the shoreline now emits a separate collision event with its last position and actual integrity loss (capped at the remaining hull); the renderer shows a brief shoreline explosion from that event. Sinking it before shore prevents the collision event and blast. The effect is advanced once per update and shared by both eyes; same-frame collisions can coalesce to the last event. Source regression covers surviving collisions, destroyed gunboats and rejected phantom blasts; Galaxy S9 visual acceptance remains pending. Each simulated salvo has a short procedural muzzle flash and its own wave-status counter; destroying a gunboat before it reaches firing range prevents that bombardment. Aircraft now resolve as deterministic **strafers** or **bombers**: strafers pepper the battery repeatedly during one fast overhead pass, while bombers make a slower approach and deliver one heavier attack before departing. Destroying the aircraft before its attack window prevents that damage. Aircraft attacks now produce a simulation-owned event only when a bomber or strafer actually damages the battery, recording the attacking aircraft's coordinates, attack pattern, damage and sequence. The VR renderer turns those events into brief parapet sparks for strafing or an expanding explosion for bombs, reusing the existing fixed-size effect pool. Intercepted aircraft and non-attacking passes produce no false blast; concurrent same-frame attacks can coalesce to the last event. Native tests cover actual attack events, damage and successful interception; Galaxy S9 visual acceptance remains pending. Aircraft require an anti-air upgrade for gun damage or a purchased airstrike; firing the basic gun at one still consumes ammunition but cannot harm it. The anti-air line now has three permanent tiers: tier 1 unlocks aircraft engagement, tier 2 adds 15 damage per aircraft hit, and tier 3 adds 30 damage per aircraft hit; these bonuses never change surface-vessel damage. On a 100-HP aircraft without other upgrades, the number of hits falls from five at tier 1 to three at tier 2 and two at tier 3. Aircraft now also follow seeded lateral courses: bombers make shallow course corrections and faster strafers weave through a wider approach. Each aircraft's x position and banking angle are deterministic functions of its authoritative forward progress and seeded direction, so slow-frame substeps, aim hitboxes, muzzle/strike coordinates and both stereo eyes share the same flight pose. The tiny generated silhouettes use swept triangular wings with banked tips; bomber engine pods follow their corresponding wing height. This adds no asset/texture packages or per-eye simulation. More elaborate flight geometry, animation and headset visual validation remain ongoing.

The module generates simple cuboid rowboats, military boats and individually positioned shore infantry silhouettes (body and head), plus distinct generated bomber/strafer silhouettes with dynamically banked, swept vertex-color wings. Rendered infantry positions and bounded combat-ray hitboxes share the campaign's authoritative soldier objects, and defeated soldiers are removed individually. Combat targeting now selects the nearest positive ray-surface intersection rather than hitbox-center depth for overlapping vessels, aircraft and infantry. The shared deterministic intersection helper rejects malformed rays; Galaxy S9 pointer alignment and headset hitbox proof remain outstanding.

## Heavy armored assault transport — final-stage invasion

Late Blackcap campaign waves now add a true `UnitType::ArmoredTransport`,
**320 HP with armored hull plating**, not a merely tough infantry boat.
The existing progression and escalating wave-pressure mix are preserved:
eligible heavy ships first enter at threat grade 10+, only in the later
half of a day/night's actual waves; the final campaign wave guarantees
one heavy flagship as its first spawn and a real gunboat escort second.
The escort approaches from an offset lane and fires ahead of the slower
0.44-unit/s heavy ship, whose deck gun can deliver real 5-point
battery hits every 3.8 seconds offshore.

The hull applies weapon-specific damage: standard 20-point machine gun
hits deal **4 HP**, 35-point heavy gun hits **11 HP**, 42-point flak
hits **10 HP**, while direct cannon 90 and missile 125 keep full
penetration. Artillery/airstrike splash deals 75% of listed damage.
Sinking pays 210 credits and cancels **two armored cars and four
infantry** before any unloading. Surviving heavy ships instead deploy
two real independent 95-HP cars and four infantry using existing
rifle/flanker/support tactics. The battle-only armor array was raised
to 20 total entries for maximum ten heavy ships; the original 80-byte
shop save and other combat limits remain unchanged.

A larger entirely original GLES2 3D hull has reinforced side/bow plates,
raised control compartment and deck gun, two visible vehicle cradles
and a landing ramp that lowers on approach. The active renderer
shows this 3D ship (not the ordinary atlas landing boat), with a
1.48-radius target volume, real 320-HP HUD, appropriately scaled
wreck fragments plus a three-second plated-hull sinking sequence,
a compact RAMP countdown during the exposed 1.25-second unloading phase,
and a true armored gun sound after damage. The heavy hull's center
beaches at z=-3.40, where its forward bow reaches the actual z=-2.23
armor and infantry cargo exit; smaller boats retain their old z=-2.20
center landing line.
The owner-attributed `Mars des Souvenirs` soundtrack stays unchanged.
Source evidence: [heavy transport implementation record](../../docs/records/2026-10-16-breakwater-heavy-armored-transport.md).
The complete native campaign, Android/NDK build, Galaxy S9
stereo/aim/audio and physical performance gates remain unexecuted;
only the exact standalone invasion/armor-selection regression
passed strict GCC, Clang and Clang sanitizers.

## Armored landing craft and shore vehicles — 2026-10-16

Later-progression landing craft may now carry **one real light armored car**
in addition to up to three infantry. The manifest is deterministic from
the authoritative wave's seed/spawn hash; first-day rowboats are always
one-soldier unarmored craft. Before the ramp opens, the armor remains cargo
and cannot be targeted as an independent shore unit. **Sinking the transport
cancels its infantry and armored cargo**, rather than magically spawning
a vehicle on the beach. The active atlas ship-render path includes a small
olive armored silhouette atop eligible landing craft.

A surviving carrier produces one independent `ArmoredVehicle` (95 HP)
in a fixed ten-slot, battle-only pool at the existing shore landing line.
The vehicle rolls inland at 0.29 world units/s to z=-1.56, stops, and
fires a four-damage armored cannon every 3.3 seconds while alive. Its
confirmed attack emits a simulation-sequenced `ArmorShotEvent`, visible
gun flash and a distinct synthesized gun report. Destroying the vehicle
before its next shot cancels that attack. Armored units independently
prevent wave completion until killed; a vehicle kill awards 110 credits
and increments the armor-loss tally once. A battle reset clears all armor
and its event without changing the existing 80-byte shop save schema or
the max-ten-craft/minion capacities.

Mounted weapons now use the same true cannon-origin ballistic solver
to resolve a separate 0.43-radius car hitbox (y=-0.72) and the nearest
physical cover/soldier/boat obstruction. In-arc targeting shows orange
armor brackets and `ARMORED CAR HP n/95` on the existing compact HUD,
with a faceted original olive chassis, wheel/track bodies, turret and
barrel generated in the same shared stereo world geometry. A 20-damage
machine-gun round deals 5 damage to a car's armor; the 35-damage Heavy
Gun deals 17; the 42-damage Flak deals 14, and the missile and direct
cannon deliver their regular damage. Real destroyed armor receives
an explosion, distinct destruction sound and no further firing.
Purchased artillery and airstrikes can also destroy deployed cars and
produce the relevant battle impact; armor is not an unsupported purely
decorative exception. No third-party textures or sample recordings were
added.

**Late-wave save, turret, and warning integration (2026-10-16):**
One older checkpoint validator still rejected `wave_index > 8` after
the campaign had been expanded to as many as 25 actual waves per
period. The native reader now rejects the full 32-bit serialized value
only when it is zero, above the 32-wave global cap, or later than that
specific location/day/night/difficulty period's `waves::ForPeriod`
limit. `waves::CheckpointAllowed` is shared with an executable test
enumerating every allowable checkpoint position across all 18
periods and four difficulty tiers, including invalid 257/65,537
wraparound attempts and the final Siege 25-day/21-night boundaries.
Old 80-byte shop state remains compatible, without changing the save
structure.

The purchased support turret no longer ignores vehicles just because
their landing craft has unloaded: after any nearer surface vessel,
it automatically shoots the closest deployed armored car with heavy
penetration (7 HP per 15-damage turret report), credits/kill counter
only on actual destruction, and an actual simulation-owned turret
impact event. It still never damages aircraft. The short combat HUD
now predicts the earliest legitimate armored car shell from the
same physical travel-to-line distance and cooldown that control real
fire, showing `ARMOR nS` instead of `SHORE nS` only if the vehicle's
next salvo would happen first. This reuses the original one shore
warning location instead of stacking another floating box.
Destroying that car removes its prediction immediately. Exact
standalone header tests passed GCC, Clang, and Clang ASan/UBSan; the
complete Campaign/module/Android tests remain unexecuted.

**Near-shore ballistic geometry repair:** During the armored landing pass,
the original physical gun-mount z=-0.88 was found to put the muzzle at
approximately z=-2.3, seaward of the soldier firing line z=-1.34 and
the vehicle line z=-1.56. Under the real muzzle-origin ballistics these
shore units were behind the muzzle and could not be shot. All three
gun-mesh mount uses (barrel mesh, flash and muzzle-origin provider)
now share `gunmesh::kMount={0,-0.76,+0.65}`, placing the gun breech
low behind the seated viewpoint and its muzzle approximately z=-0.98
to -0.60 for the five physical mounts, landward of the beach units.
A short new cradle deck supports the mount. The same pre-recoil muzzle
remains the **actual** projectile origin; no controller-origin hit bypass
was reintroduced. Extended cannon trajectory regressions now assert
that every weapon's shot intersects actual nearby soldier and armored
car hit spheres. Strict host GCC/Clang and Clang ASan/UBSan passed these
exact trajectory test bytes using the earlier locally mirrored gun and
ballistic dependencies; full original gun geometry test/NDK/headset
fit remains pending.

The exact `breakwater_armor.h` and native regression source passed
strict host GCC/Clang C++17 and Clang ASan+UBSan with a minimal test-only
campaign dependency shim; the exact armor helper/test GitHub hashes were
verified. The broader `breakwater_campaign_test.cpp` now includes a
seeded two-run test of cargo-carrier survival versus interception,
independent armor HP, movement, reward and killing. That **full test,
complete game module, Android NDK/APK, and Galaxy S9 headset visuals and
sounds remain unexecuted** in this pass. The user can expect source
implementation, not a claim of device-ready shipping or complete QA.

## Campaign credits and purchases

Players earn credits for destroyed hulls, eliminated shore infantry and surviving a wave. Purchasing is allowed only between waves; upgrades have real effects in the simulation.

| Choice | Base cost | Existing behavior |
|---|---:|---|
| Fire rate | 180 | 35 ms less shot delay per tier; maximum 4 tiers |
| Damage | 200 | +10 impact damage per tier; maximum 4 tiers |
| Clip size | 160 | +4 rounds per tier; maximum 4 tiers |
| Anti-air | 750 | Tier 1 unlocks aircraft; tier 2 costs 1,050 and adds +15 aircraft damage; tier 3 costs 1,350 and adds +30 total aircraft damage. Max 3 permanent tiers |
| Artillery strike | 260 | One consumable bombardment of surface vessels |
| Airstrike | 420 | One consumable strike against surface or air |
| Support turret | 470 | Automatically damages nearest surface craft at a fixed cadence |
| Repair | 140 | Restores 25 battery integrity, capped at 100 |

Advanced weapon mounts, separate caliber variants and impact/splash effects remain later features; the current lightweight gun tracer is generated procedurally and purchases affect the actual C++ gameplay calculations, not merely their labels.

## First playable controls and 3DoF contract

The Development-only module is launched from Native Apps after setting launcher logging to Development. The shell retains Back, Home, Quick Menu and recenter control. The gun is aimed with the normalized native pointer direction.

In the shop, point at any of the eight labelled upgrade cards and press **Select** to buy it. The AA card explicitly shows its owned level as `AA 0/3` through `AA 3/3`, and displays MAX after tier 3. All tiers persist through the existing 80-byte shop save format; invalid tier-4 values are rejected without mutating the loaded campaign. Point at **START** and press Select to begin the next wave; an available secondary action can also begin it. In combat, point at a hull or an individual disembarked infantry silhouette and hold Select to engage; the nearest actual ray-intersected live target is selected, while aiming into empty sand spends a round as a miss. Fire cadence depends on purchased rate. Every valid in-traverse gun shot now consumes a round whether it hits or misses. A shot aimed at aircraft before anti-air is purchased still spends ammunition and produces feedback but cannot damage the aircraft. Point at **ARTY** or **AIR** and press Select to use a stockpiled strike; Select on either support label now captures the entire press until release, even if the pointer drifts off the label, preventing accidental gunfire and ammunition loss. The shop START press (including a secondary-triggered wave start while Select is held) also remains captured across the transition into combat, so a held shop input cannot fire the first round; each combat HUD label displays its remaining charges. Artillery requires an active surface vessel and airstrikes require at least one active enemy (surface or aircraft). An empty-target call fails with the shell's rejection feedback without spending a charge, including when only infantry have already landed. The turret fires automatically after being purchased. The combat HUD also predicts the earliest real aircraft strike and gunboat salvo up to six seconds ahead, displaying separate AIR RAID and GUNBOAT countdowns. These warnings use live attack cooldowns, movement speeds, and actual firing windows; a destroyed attacker, exhausted bomber, or ship whose cooldown expires after it leaves the firing lane does not generate a false warning. Native regression assertions cover initial approach, post-attack and destroyed-target cases; headset legibility remains unverified. Each actual turret shot now records a deterministic simulation event with the target's pre-destruction coordinates and whether the hit was lethal. The native renderer consumes this event once per update, briefly flashes the turret muzzle and draws a metal impact or an expanding explosion for a destroyed vessel; idle turrets and unpurchased turrets do not fabricate hits. Regression coverage exercises a real first turret hit, a lethal follow-up shot, the original target position, and invalid effect coordinates. Headset appearance remains a device acceptance gate. Invalid actions produce the existing shell-owned failure cue. Victory/Defeat provides an implemented restart. Phone test must prove readable target silhouettes and reliable pointer hitboxes; this is not physical validation.

Aircraft targeting feedback now derives strictly from the same live `CombatTarget` ray used to fire: an actually selected aircraft receives small wing-adjacent world-space brackets, plus a combat-HUD readout of BOMBER/STRAFER HP if the AA package is equipped or AA REQUIRED if it is missing. Actual gun damage, not merely a valid trigger pull, produces a short AIR HIT damage readout; a lethal intercepted aircraft produces AIRCRAFT DOWN using the pre-destruction target coordinates. These feedback timers advance once per native update, not once per eye; destroying a target removes its aim brackets on that frame. Unselected, already-destroyed and ordinary vessel targets produce none of these cues. Pure native regressions cover no-AA rejection, positive damage, lethal interception, live hit-position consistency, timer expiry and invalid target slots. Physical headset legibility and targeting latency remain unverified.

The ballistic foundation now defines five original mounted weapon profiles: MACHINE (12-round general-purpose MG), HEAVY (8-round heavier MG), FLAK (6-round dedicated AA), MISSILE (two guided rounds with independent secondary/ghost sight), and CANNON (four-round direct-fire anti-vessel gun). Their initial progression prices are 0, 900, 1,350, 2,200, and 1,600 credits. The standard MG is not a magical laser: its approximate trajectory drops 0.014 world units per squared unit of flight, which means a long-distance vessel requires leading the shot *upward* to compensate. A pure, deterministic projectile-to-sphere solver rejects straight crosshair hits when the round passes underneath; flak drops far less, and missiles use a straight guidance ray. The cannon has a much stricter elevation limit and the independent ghost sight remains aligned with the real input ray even when the physical mount cannot follow it. Weapon ownership and active selection are now simulated with real shop spending: the standard machine gun starts owned, other mounts must be purchased, weapon changes preserve per-weapon ammunition, and the SAM battery has a capped three-charge inventory at 540 credits per charge. The 80-byte shop save now writes version 2 with the ownership mask, selected mount and SAM inventory in formerly unused slots; the old version-1 format still loads. Snapshot loading validates the complete 32-bit wave index before narrowing it to the runtime byte: corrupted values such as 257 or 65537 must not masquerade as wave 1, and rejected snapshots leave the current shop state unchanged. The in-world shop now has an ARMORY page with actionable cards for purchasing/selecting five mounts and buying SAM charges. In combat, five compact weapon cards switch among purchased equipment; SAM is a two-step support action: press SAM to arm the ghost sight, aim that independent pointer at an actual aircraft, then press Select to fire. Press SAM again to cancel. An invalid lock never spends a SAM charge, and ordinary cannon elevation remains limited even while the ghost marker points up. SAM target confirmation now consumes exactly one Select press, whether the aircraft lock succeeds or fails: holding Select or dragging the pointer cannot silently retry a failed lock, spend another SAM charge, or fire the selected gun after a successful missile launch. Release and press Select again to retry. Each weapon has its own fire cadence, magazine and ballistic gravity; existing gun damage/clip/rate upgrades still apply. Curved fired tracers, target hits and water misses use the same dropping shot path rather than a straight laser. The missile battery, like the other four mounted weapons, now fires from its physical muzzle while inside its permitted traverse (using its existing zero-gravity guidance trajectory); **only SAM support retains its independent controller-origin lock**. Outside the selected gun's firing arc, the pointer-aligned ghost sight appears but ordinary guns cannot shoot through the disallowed angle. Flak retains its wider elevation arc. These are source-implemented controls with native and Android/device validation still required.

The gun carriage is world-anchored and the rendered barrel follows the handheld pointer only inside a finite traverse/elevation envelope; firing outside that mount arc is rejected. As of the expanded wave/muzzle pass, valid mounted shots now use the true *resting* muzzle position from `gunmesh::Muzzle`, including normal target selection, infantry and cover occlusion, splash/impact location and the stored tracer path; the handheld controller supplies the gun's direction **not** its starting point. A restrained dashed ballistic preview emerges from that same cannon-origin ray only inside the traverse arc. Once the controller's direction exceeds the mounted weapon's arc, the in-arc preview disappears and the out-of-range controller ghost sight takes over. Clicking beyond traverse is rejected, while the existing independently aimed SAM remains a special two-step support operation. The muzzle shot begins before the 90-ms barrel recoil; the transient muzzle flash follows the recoiling barrel. Both stereo eyes render one shared scene trajectory and no new aim/save ABI has been introduced. Successful gun shots create a short-lived generated tracer ribbon, including misses, while the 1.15-second automatic magazine reload makes clip-size upgrades meaningful. In combat, a secondary-button press now requests a tactical reload of a partially spent magazine using the same 1.15-second simulation timer. Full magazines, active reloads and non-combat phases reject the request without changing ammunition; shooting remains blocked until completion. The combat HUD shows SECONDARY RELOAD when a partial magazine is ready for manual reload, with the existing RELOADING progress and READY completion cues taking precedence. Native regression assertions cover the request lifecycle and rejection cases. During reload, the combat HUD displays a progress bar and RELOADING label, driven by the simulation timer and updated once before stereo rendering; firing remains blocked until the magazine refills. A fixed pool of eight procedural impact pulses produces sparks only for actual vessel/aircraft damage, infantry dust and sea splashes for downward misses. Lethal gun hits on hulls or aircraft now produce longer expanding smoke-and-fire explosions; rounds that cannot damage aircraft without the anti-air upgrade consume ammunition but no longer invent hit sparks. Impact positions are captured before lethal hits deactivate targets, and the pulses advance once per update rather than separately per eye. Horizontal and upward misses do not invent sea splashes. Native regressions cover damage-gated impact kinds, blast lifetime, pulse position, expiry, malformed input and capacity; headset visibility remains unverified. Artillery and airstrikes now emit separate, longer-lived procedural blast pulses for each actually damaged enemy (up to the fixed eight-pulse capacity), using pre-strike coordinates so sinking boats and destroyed aircraft still explode at the correct location. Artillery flashes amber at the waterline; airstrikes use a pale overhead flash for aircraft and surface targets. Rejected support calls and artillery-immune aircraft produce no false blasts. Native regression fixtures cover both support types, target locations, lifetime, and rejected calls; physical headset visibility is still unverified. The combat HUD now briefly shows READY for 0.55 seconds when the simulation confirms that the magazine has finished reloading; this cue does not trigger on ordinary idle frames or when combat has already ended. Native gameplay cue flags now drive distinct original generated gunshot, reload-start, reload-ready, artillery bombardment and overhead airstrike audio through reusable Android static AudioTracks. The 180ms gunshot uses two preloaded alternating voices so the fully upgraded 160ms firing cadence no longer truncates the previous report; the other cues retain one static voice each. PCM is synthesized once per cue when the bank initializes, with no per-shot allocation or stereo-eye synthesis. Host-side voice rotation and lifecycle were exercised with Android media stubs; actual Galaxy S9 mixing and latency remain unverified. Successful artillery and airstrike calls request their own shell-owned cues; rejected calls retain the failure cue without playing a support blast. Only valid rounds, successful reload requests, emptied clips, and simulation-confirmed ready transitions request those cues; failed actions preserve rejection feedback. Local/static and S9 audio latency/mix acceptance remain pending. Three AA tiers and seeded banking aircraft flight are implemented; additional aircraft types and richer airborne engagement geometry remain planned. An 80-byte versioned shop-state save preserves completed waves and bought upgrades through the module-private host save service. Mid-wave exit resumes at the prior shop; S9 exit/re-entry and APK update durability remain device gates.

## Controller-focus gameplay suspension (2026-10-14 source)

Breakwater's game-private `PointerFocusGate` now treats the native host's
`POINTER_NONE` as lost gameplay focus, including the universal Quick Menu.
The module still receives its normal host frames, but does not advance the
campaign, enemy attacks, magazine/reload timers, combat effects, or UI
interactions while no usable calibrated gameplay ray exists. This prevents
unopposed invasion damage during shell menu interaction or temporary
controller-ray loss. The visible game remains frozen at its current state;
there is no paused-time catch-up. Shell navigation and recovery remain owned
by ReverieVR and other native games are unchanged.

On focus loss, held Select and secondary input are released from Breakwater's
game input latch. When a valid ray returns, each physical button must be
observed released before a new press can trigger firing, purchases, strikes,
weapon switching, or reload. The same re-arm policy applies on the module's
Android lifecycle pause callback; GL resources and campaign saves are
unmodified. The gate has isolated C++17 deterministic regression coverage for
both-button holding, independent release, fresh press and lifecycle pause.
Full native/NDK build and S9 Quick Menu/connection-interruption proof remain
separate acceptance gates.

### Assigned background music and progression SFX — 2026-10-16

The project owner's previously assigned **Mars des Souvenirs.mp3** is the
Breakwater soundtrack; it remains a separately authorized original music
recording under `music/source/audio`, validated against the
`music/source/manifest.json` by the existing Gradle `stageOwnerMusicAssets`
task, placed into APK assets as
`music/tracks/mars-des-souvenirs.mp3`, and selected through
`NativeMusicPlaybackPolicy.cueForNativeModule("breakwater-battery")`.
`ShellMenuMusic` loops it while this native module is hosted. No replacement
song, new downloads, or alternate music-license assumptions were introduced.

The existing ten synthesized ordered battle SFX are supplemented by four
short, distinct, deterministic PCM16 cues: shop purchase/equipment selection
(`REVERIE_NATIVE_SOUND_SHOP_PURCHASE=16`), actual completed wave
(`WAVE_CLEAR=17`), terminal victory (`CAMPAIGN_VICTORY=18`), terminal
battery destruction (`BATTERY_LOST=19`). Production emits the cues only
on successful purchase or the actual `Campaign::Tick` phase transition,
never from cosmetic draw/update frames. Select actions that fail do not
produce a purchase noise, and repeated Shop/terminal frames cannot replay
a phase-change cue. Cue IDs 1–15 retain their earlier meanings. Android's
`NativeCombatWaveform` generates all audio during bank initialization and
`NativeCombatSound` reuses fixed `AudioTrack` voices; the soundtrack
remains a separate `MediaPlayer` with independent volume/mute.

A JVM regression in `NativeCombatWaveformTest` covers IDs 16–19, four
distinct deterministic waveforms, bounded PCM memory, non-silent fade
and one reusable voice per new cue. The full APK, actual source-song MP3
packaging in a produced APK, handset mixing, loop continuity and S9 audio
latency are not physically verified in this pass.

## 2026-10-16 extended campaign and cannon-origin aiming correction

**Authoritative schedule:** `breakwater_wave_schedule.h::ForPeriod` is used by `Campaign::waves_this_period` and tested across every location, day/night cycle and difficulty. Totals are Cadet 244, Regular 280, Veteran 316, Siege 352; 32 is a hard per-period ceiling and every save remains an original versioned 80 bytes. First-wave craft counts (2–5, scaling as before up to the existing ten-craft wave limit) are separate from these progression counts. Late period wave indices, invalid overflows, and end-of-day night transitions are covered by revised native campaign regression source.

**Authoritative trajectory:** `breakwater_cannon_trajectory.h::CannonShot` takes only the mounted weapon and the controller direction, validates the firing arc, reads the 3D gun mesh's true muzzle coordinate (pre-recoil), and generates one normalized ray. The native target selection, damage gate, actual projectile hit-depth, water impacts and stored shot effect now all use that same ray. The 3D weapon follows controller aim within traverse for all mounts, including the missile pod. A small dashed predicted path is rendered from the same cannon muzzle while within arc; a separate hand-controller ghost marker is used only when the gun cannot follow or SAM is armed. Controller ray remains authoritative for direct HUD/shop controls and intentional SAM special-target designation. Valid outside-arc shots are not invented; all menu/host focus and held-press safety rules remain intact. Native exact header regressions cover traverse rejection, launch coordinate equality with all five mounted mesh mouths, malformed directions, ballistic drops, hit intersections, and no accidental controller-origin shot. Android build and reference headset positioning/visibility remain pending.

## Sprite atlas and 2.5D mounted weapons — current source (2026-10-15)

**Implementation:** `breakwater_sprite_atlas.h` produces an original, fixed
128×256 RGBA atlas with 30 occupied 32×32 cells (two reserved): two
infantry march frames plus ready/firing soldier and support-gunner poses;
rowboat and travel-driven oar stroke; landing
craft with closed, half-lowered and open ramp; gunboat and firing flash;
neutral/left-bank/right-bank strafer and bomber; five mounted weapons;
and four artillery/airstrike/SAM/repair support symbols. Their muted olive, slate-steel, sand and
gunmetal shapes are generic late-1990s/early-2000s American/European-inspired
military arcade styling, **not replicas, military insignia or imported art**.
Art can later be replaced cell-for-cell without changing the gameplay ABI.
Currently these are code-authored pixel sprites, not image-generator files.

The native Breakwater GLES2 module uploads this atlas once per GL context,
renders one hard-alpha-masked world card per live enemy and infantry slot,
and batches all cards into **one extra textured draw per eye**. The batch
also carries compact selection icons for all five weapons and dedicated
artillery, airstrike and SAM combat-HUD icons. Shop pages reuse those
symbols, plus the repair icon next to the matching purchase amount.
The older near-field weapon-body billboard has been **removed** from combat:
the actual mounted 3D weapon is now generated in the same flat-color mesh as
the shoreline. Atlas weapon silhouettes remain available as catalog/HUD icons.
Textured enemy cards retain authoritative simulated positions and aircraft
banking. Shader creation, sampling, color and target collision rules are
otherwise unchanged. The atmospheric panorama still encloses the world.

**2026-10-16 menu and gun implementation:** The old gun was essentially
two crossed, camera-thin barrel planes behind a billboard. The game-private
`breakwater_gun_mesh.h` now emits deterministic 12-sided XYZRGB triangles
for five weapon variants: tapered barrel, darker muzzle recess, breech and
reinforcing collars, two thick faceted wheels and a sloping armor shield.
Flak and missile mounts gain a second actual offset barrel. The gun tube
rotates with the existing world-space pointer aim (including elevation);
it introduces **no change** to weapon ownership, hit testing, the 80-byte
shop save, simulation ballistics, or the shell's emergency controls. The
emplacement and its large lower parapet render only during active combat,
not behind the shop/briefing menus where they could obscure credits and
action labels. Exact geometry tests for the game-private generator passed
strict GCC/Clang C++17 builds; full GLES2/NDK/stereo proof remains pending.

The shop previously painted its credit total below its own board, and
incorrectly allowed text larger than the card width. Breakwater now lays
out the entire shop and difficulty selector using
`breakwater_menu_layout.h`, one authoritative set of bounds for drawing
and ray hit testing. The compact panel uses fitted actual stroked letter
geometry for purchase names/prices, a row of concise `START`/`ARMORY`
actions, hover highlighting, and credits **inside** the bottom margin.
The live headings show location, day/night, wave and available credits.
Shop elements sit above the shoreline's -1.10 sea-height occlusion plane,
and non-existent armory cells 6/7 no longer accept presses. Actual text
still uses `reverie_native_vector_text.h`, not square placeholder boxes.
The CPU geometry tests cover card bounds, label fitting, all actionable
ray regions and menu/footer clipping; device stereoscopic readability
remains unverified. The renderer draws all of this in the same existing
scene batch and does not add a per-eye menu simulation pass.

### Idle-menu redraw suppression and difficulty preview — 2026-10-16 continuation

The original Breakwater `Update` ended with an unconditional
`scene_dirty=true`; `Draw` then regenerated the entire coastal water/sky,
terrain, procedural UI, and sprite batch, and re-uploaded buffers every frame,
including while the player was idle on a static shop/difficulty/result menu.
The game's private `breakwater_scene_refresh.h` now makes this distinction
explicit: combat phases continue refreshing every update, but Briefing, Shop,
Victory and Defeat reuse the last complete scene until selection, button
activation, pointer availability, or phase transition changes. Both eyes
still render the same cached scene; no additional GL objects or draw calls,
menu latency timers or gameplay changes were introduced. The backdrop
is deliberately static while idling in menus; sea animation resumes in
combat. On loss of pointer/Quick Menu focus, active UI highlights and the
ghost sight clear on the **first** lost frame. Subsequent pointer-none frames
neither advance gameplay nor repeatedly rebake frozen geometry. On pointer
return, the complete UI is invalidated once and interaction is still held
behind `PointerFocusGate`'s release requirement. The pause handler clears
the stale pointer and hover state as well.

The difficulty selector now shows one centered, fitted, selectable preview:
CADET — 2 opening threats and 2 starting day waves; REGULAR — 3/3;
VETERAN — 4/4; SIEGE — 5/5. Those exact opening numbers match
`Campaign::NextCount` (2 + tier) and `Campaign::waves_this_period`
(2 + tier at location 0, day 1, daytime). Later days and locations have
their existing campaign scaling. The new preview lives between the title
and four actual difficulty hitboxes without changing any hit target, input
binding, wave logic, upgrade balance or persisted save format.

The game-private regression includes real menu rectangle/text vertical
separation and all four opening descriptions plus a deterministic
`RefreshNeeded` policy fixture (5,000 idle frames per phase, redraw
transitions, uninterrupted combat frames and pointer focus change).
Host strict GCC, strict Clang, and Clang AddressSanitizer/UndefinedBehaviorSanitizer
passed all four visual helper suites. The exact committed
`breakwater_menu_layout.h`, test, `breakwater_scene_refresh.h`, and test
matched independent `git hash-object` IDs after local reconstruction.
The full game module and Android/GLES2/Galaxy S9 remain **not run** this
session; actual menu clipping, controller focus and thermal improvement
must be validated on device before acceptance.

### Shop inspection and gunshot model motion — 2026-10-16 continuation

The new compact shop also explains the highlighted item on a single
unobtrusive, real-vector-text line between the wave heading and the product
cards. This line uses the **current** Campaign prices, owned/equipped state
and available credits (including the original update cap / max state):
for example, **FASTER FIRE - BUY 180 CR**, **AIR DEFENSE - NEED 750 CR**,
**NAVAL CANNON - EQUIP**, **NAVAL CANNON - ACTIVE**, **START THE NEXT WAVE**.
Unaffordable items have a muted red background, but remain reachable so the
status can explain the cost. The status is never a new unimplemented button.
An extracted game-private `breakwater_shop_feedback.h` implements bounded
fixed-buffer text emission, with the actual price resolved by a single
`ShopDisplayPrice` path used by both purchase cards and descriptions.
The renderer uses existing `StrokeText` and width fitting, not a font asset,
extra overlay, or another menu. Unknown/empty armory slots have no buy target.

The existing confirmed-shot 0.09 s flag now also animates the new gun mesh:
the barrel/breech move backwards along the current mount direction by at
most 0.12 world units, while the wheels and protective carriage stay fixed.
A faceted gold/amber muzzle flash is generated around the recessed barrel
mouth only when a shot really occurred. This is a visual consequence of
`Campaign::FireAt`/`FireMiss` success, not a trigger-based fake hit.
The recoil and flash return to zero without adding simulation state, save
fields, audio events, shader/texture resources, or extra draws. Their geometry
is generated into the existing scene buffer once per state-changing Update
and shared between stereo eyes. Host GCC/Clang/Clang ASan+UBSan checks cover
exact committed headers, bounded status labels, stationary wheels/shield,
recoil displacement, finite flash triangle mesh and rejected/nonfinite
state. Integrated Android NDK/GLES2/physical headset proof remains open.

Native regression assertions check atlas coverage, transparency, tile UVs,
finite six-vertex card geometry, aircraft bank, fail-closed dimensions,
all five weapon enum-to-art mappings, all four support icons, both shop-page
icon mappings, unmapped/invalid shop cells, unique pixel identities and
cell bounds. The expanded 24-sprite atlas occupies 128 KiB of RGBA
data; the final eight cells are deliberately transparent/reserved.
Static verification is not a substitute for an Android ARM NDK compile,
actual GLES2 shader/texture creation, near-plane stereo readability, distant
pixel aliasing, realistic FPS measurements or a Galaxy S9 wave trial.
The original 3D boat/infantry silhouette recipes remain available as
reference geometry but no longer draw live units.

### Next visual asset runs and acceptance

The art contract is **3D shoreline and emplacement + 2D enemy sprites +
2.5D near-field weapon cards + textured HUD + layered atmospheric sky**.
Production sheet order: (1) deeper boat direction and detail studies
(current code art already animates oars, ramps and gunboat salvos);
(2) eight-direction infantry idle/march/fire frames with readable
helmet, vest and rifle; (3) aircraft underside and oblique detail
(current code art already supports left/neutral/right bank);
(4) additional depth/aim variants of the five mounted weapon cards;
(5) optional bespoke HUD panel styling after headset readability tests
(the functional support icons and equipment selection already use the
atlas); (6) optional authored texture-based atmospheric refinements after the
procedural 360-degree sky passes headset acceptance. Author all
replacement art with transparent backgrounds, fixed local foot/waterline/
center-mass anchors and no copied real military unit markings. Keep the
single 128-KiB RGBA atlas and one additional masked draw per eye until
measured device proof justifies more. Materialized artwork must be explicitly imported,
registered and verified before being reported as in-game.

## Exact-ray surface targeting and hull-condition display (2026-10-15 source)

Breakwater now extends its aircraft-specific targeting brackets to *only*
the boat or individual infantry slot selected by the authoritative
`CombatTarget` ballistic intersection. Rowboats, landing craft and
gunboats display compact world-space corner brackets and a narrow
remaining-hull bar, derived from their actual current and spawn HP.
Disembarked infantry receive tighter person-sized corner brackets, with
the actual weapon's anti-infantry capability reflected in the marker
color. The existing target-status region carries one succinct
`ROWBOAT / LANDER / GUNBOAT HP N/M` or `INFANTRY` label;
only the currently targeted unit appears, preventing HUD clutter.

`EnemyStartingHp(UnitType)` in the campaign rules is the single source
for 20/85/125 surface and 100 aircraft starting HP; the rendering
helper reuses it. The display introduces **no aim assistance, altered
combat hit spheres, damage, simulation state, serialized data, textures,
GL shader, additional draw call, controller binding or input target**.
The markers are ordinary triangles and two small panels in the existing
shared vertex-color mesh. Target inspection happens once per game
update; the two VR eyes receive the same markers. Focus loss, pause,
new wave and campaign restart clear selection. In SAM-only armed mode,
a selected boat is not falsely advertised as a valid missile lock.
Native C++ regression cases cover unit types, damaged/clamped HP,
weapon vulnerability, malformed coordinates and phase/slot guards.
Full Android NDK compile, visual alignment at headset depth, binocular
readability and physical controller tests remain outstanding.

## Riflemen pinned by a real hit against occupied cover (2026-10-15 source)

A successful player shot into a **living barricade** now briefly pins
only riflemen physically close to that same assigned structure.
The test uses the actual cover width, current soldier x/z position,
slot ownership and original wall coordinates before applying damage;
troops still coming from the surf, rowboat infantry, flankers, support
gunners and soldiers at another wall are unaffected. A 1.20-second
battle-only suppression counter begins on every qualifying hit, even
when that hit destroys the wall. The gun still consumes normal ammo
and the wall still takes its usual damage. No automatic soldier damage,
new target or invulnerability is introduced.

Pinned riflemen remain individually hittable at the **unchanged**
position and hit sphere. Their movement now **halts** for the remaining
suppression time, even if the same shot destroyed the barricade and
assigned a retreat or nearby alternative. They cannot contribute to a
volley or show a false firing flash while the counter is positive.
Once it expires, movement resumes without teleporting; if suppression
ends during an update, the remainder of that update advances movement.
An additional hit restarts suppression. The shared
`InfantryReady`, `InfantrySecondsToReady` and `AdvanceInfantry`
functions drive both real salvo eligibility and SHORE countdown;
the simulation never advances independently for each stereo eye.
Their displayed card switches to a distinct original rifle-lowered
defensive pose whenever suppression is active, occupying cell 31
(0-based slot 30) of the existing 128-KiB, 32-cell atlas. All
other art/weapon silhouettes and the single sprite draw remain intact.

Regression cases exercise real hit/ammo/damage coupling on a seeded
Iron Quay landing party, isolated proximity and role restrictions,
the exact pin duration, physically stationary covered-to-exposed
retreat, matching arrival prediction including recovery-plus-travel,
0.05/0.10-second frame-split equivalence, no firing pose during
suppression, and the new atlas tile/UV bounds. The existing 80-byte shop save, controller,
shell HUD and enemy collision geometry are untouched. Device stereo
visibility and sustained frame timing remain subject to S9 testing.

## Infantry cover-seeking and post-destruction regrouping (2026-10-15 source)

Landing-craft **riflemen** at Iron Quay and Blackcap Island now seek
their nearest *living* shoreline barricade when disembarking within
1.18 world units of that structure. A covered rifleman advances both
forward and laterally to a firing station 0.29 world units *behind*
the selected barricade, offset slightly inside its physical width.
This puts the existing ballistic cover between the battery and
that actual infantry hit sphere; it does not confer artificial damage
reduction or invulnerability. Flankers retain their separate side
lanes, support gunners retain their rear-line role, and rowboat
riflemen preserve their original uncomplicated approach.

The existing transient `Infantry` state now retains one signed cover
slot, an explicit lateral-regrouping flag, an open-ground fallback
x-coordinate, and a cover firing-line z-coordinate. `AssignInfantryCover` sets a deterministic order at
landing. `AdvanceInfantry`, `InfantryReady`, and
`InfantrySecondsToReady` share cover-aware station and lateral
movement logic with the actual authoritative `Campaign::TickStep`.
If that *same physical barricade* is reduced to zero HP, the
authoritative `FireAtCover` hit immediately refreshes attached
riflemen's orders. A soldier on Blackcap can reoccupy the nearest
still-intact barricade within 1.90 world units of both its *current*
position and former barricade, keeping its original fallback lane and moving laterally
in real simulation time. That longer route delays its firing
readiness and the SHORE warning; no soldier teleports or shoots
while crossing to a new station. Iron Quay's widely spaced barriers
have no close alternate, so their riflemen regroup toward the
original x lane and z=-1.34 exposed firing line. If Blackcap
alternatives are subsequently destroyed, the same open-lane fallback
applies. `RefreshInfantryCover` also runs during simulation updates
and read-only `incoming_threats()` prediction; an invalid slot never
becomes permission to claim a different wall.

Cover assignment is intentionally confined to larger landing-craft
parties at later locations. No new enemy count, hitbox, damage value,
audio bus, UI panel, GL draw call, sprite atlas cell, Daydream mapping
or shop checkpoint field is introduced. The 80-byte save remains
unchanged because all cover/infantry orders are battle-only.
Pure role regressions, Blackcap multi-wall relocation assertions and
an Iron Quay seeded landing-craft scenario now exercise selecting
live walls, moving to covered stations, destroying the claimed wall
with real ammunition/cooldown, immediate order changes and eventual
open-lane fallback. A standalone C++17 `-Wall -Wextra
-Wpedantic -Werror` exercise of the extracted helper caught and
corrected an actual lost-lateral-regrouping defect before commit.
Full repository-native/Android NDK builds
and Galaxy S9 device performance remain separate acceptance gates.

## Destructible shoreline cover with true ballistic blocking (2026-10-15 source)

The native campaign now owns up to **four actual destructible shoreline
barricades**, rebuilt on each wave and always rendered using the same
positions, widths and heights used for ballistic collision.
Shingle Bay's initial rowboat introduction deliberately remains open.
Iron Quay deploys three low concrete barriers (105 HP each), and
Blackcap Island deploys four heavier basalt barriers (170 HP each)
near the later infantry firing approaches. As shots wear them down,
they shrink and eventually disappear. A destroyed barrier no longer
blocks subsequent gunfire. These are obstacles to the player's
projectiles, not invulnerable infantry or fake armor damage.

The single authoritative `CombatTarget` selection now orders
real enemy sphere hits, individual infantry hits and physical cover
hits by **projectile travel time**. For cover, a continuous parabolic
slab calculation intersects the correct X and Z extents and the Y
range, including shots that first enter through the upper surface
after clearing the front edge. It uses the same weapon gravity as
existing trajectory and hit tests. A cover strike consumes
ammunition, respects the selected gun's normal firing cadence, and
subtracts actual gun damage from that barricade; existing impact
effects show stone or metal sparks at the intersection. Holding aim
on cover displays one compact `COVER HP/MAX` target readout.
The block geometry is emitted through the existing stereo-shared
vertex-color batch: **no new GL shader, texture, sampler, draw call,
model dependency, or permanent HUD panel**. Projectile hits and
rendered obstructions derive from one `breakwater_cover.h` source.

Cover health is battle-only state. It is not added to the 80-byte
shop checkpoint; between waves cover respawns, and all three
campaign locations retain their original enemy, infantry, attack
and victory progression. SAM and missile ghost targeting remain
unchanged, and no Daydream input binding has moved.
Regression coverage checks empty opening beach, location templates,
partial damage and destruction, ammunition/cooldown behavior,
wall rendering/alpha-independent geometry, exact ballistic occlusion,
top-entry hits, misses around and above obstacles, and zero-length
or malformed ray rejection. Full native Android/NDK build,
Galaxy S9 physical stereo/occlusion inspection, and stable headset
framerate are still outstanding.

## Authentic infantry firing poses and confirmed-volley muzzle flashes (2026-10-15 source)

The original single generic 3D flash beside the first infantry shooter
has been replaced by **individual 2D muzzle-flash art on the actual
soldiers who participated in a damaging volley**. Riflemen gain a
deployed firing-line stance and a firing frame, left/right flankers
share a distinctive deployed lateral-stance pair, and the rear support
gunner uses a heavier steady silhouette with its own firing frame.
Marching soldiers keep their two existing stride frames. The already
allocated **128×256 RGBA texture remains 128 KiB**, with 30 of 32 tiles
occupied and the last two left transparent for future effects.

The authoritative `Campaign::TickStep` now captures a 60-bit
`InfantryVolleyEvent::firing_mask` in the exact 0.05-second
substep when the real battery-damaging salvo occurs. The mask is a
*transient* event field; it is not serialized, and there is no new
player-control or audio protocol. The module's existing 0.15-second
infantry flash timer gates an individual firing frame per bit only
while that soldier remains alive and actually at their tactical
firing line. The pose selection and roster are evaluated only once
for the shared VR scene; stereo eyes consume the same batched atlas
and campaign-driven movement. Recording at the actual substep prevents
soldiers arriving later in the same 0.10-second input update from
appearing to have fired retroactively. Clearing the timer and bitmap
on pause/focus loss and mission/wave transitions prevents stale flashes.
The pre-existing parapet hit spark remains tied to actual hull damage;
the duplicate decorative world-space muzzle box is removed.

No hitboxes, aim assistance, health, damage, volley timing, weather,
textures, GL draw passes, controller mappings or save schema changed.
C++ regression source checks the real volley mask against the event's
authoritative shooter count, guards against inactive/advancing troops,
checks pose selection for all four tactical roles, verifies distinct
pixel cells, atlas UVs, alpha masks and the unchanged 128-KiB memory
budget. Host fixture checks and full NDK/Galaxy S9 rendering acceptance
must not be conflated; physical headset proof remains outstanding.

## Landing-party flanking and rear support tactics (2026-10-15 source)

Single-passenger rowboat infantry retain their original straightforward
advance to z=-1.34 at 0.13 units/second and the authoritative four-second
shared volley timer. Later landing craft now deterministically assign
**riflemen**, **left flankers**, **right flankers**, and **rear support gunners**
from their actual passenger order. Flankers move toward separate side lanes
at 0.19 units/second while continuing to advance at 0.135 units/second;
their own firing line is the original z=-1.34, and they cannot contribute
to a volley until both forward and lateral travel are finished. Rear
support gunners advance more slowly (0.105 units/second) and deploy behind
the lead troops at z=-1.68. They contribute 1 hull damage to a real
volley instead of the rifleman's 2. All infantry are still individually
targetable at their **actual moved x/z coordinates**; hit spheres, player
weapon eligibility and the underlying campaign fire rate are untouched.

The pure game-private `breakwater_infantry_tactics.h` is shared between
`Campaign::Land`, the authoritative `TickStep` movement/volley, and
`incoming_threats()`. The SHORE countdown therefore considers both
the correct tactic-specific forward distance and lateral maneuver, rather
than incorrectly predicting the old uniform sprint. Loss of all ready
troops still cancels the accumulated four-second group salvo, and the
next arrivals must wait their own full period. The original one-rowboat
timing and threat prediction are deliberately preserved.

The first support-gunner silhouette used **one of the eight
previously blank cells** in the 128×256 RGBA atlas. The later
confirmed-volley sprite update fills five more cells, bringing the
current total to **30 filled cells, two reserved, 128 KiB unchanged**.
Flankers use existing two-frame march sprites, following their actual
lateral position. No new render pass, texture, input, sound events, save
fields or new infantry allocations. C++ regression coverage was added for
role assignment, lateral bounds and speed, readiness and ETA, support
damage, one-shot state guards, finite values, atlas transparency/identity
and a real seeded Iron Quay landing-craft disembarkation. Focused
C++17 -Werror movement/prediction fixture passed; full integrated native
suite, Android NDK build, and S9 target readability/performance tests
remain unverified.

## State-driven enemy sprite animation (2026-10-15 source)

The original 16-cell (128×128) procedural RGBA sheet now occupies a
128×256 power-of-two atlas (128 KiB) with 24 original silhouettes before infantry pose expansion. Eight
new cells animate the **rowboat oar stroke**, **half/open landing ramp**,
**gunboat firing flash**, and distinct **left/right bank** for both
strafer and bomber. All icons, weapon views, previous sprites and HUD
stay inside the same atlas, with eight initially spare cells (subsequent soldier art now leaves
two spare). No new sampler, texture, shader, draw pass or gameplay
object is introduced.

`EnemySprite(const Enemy&)` is a pure presentation-only selector.
Rowboat stroke follows the existing vessel travel-phase helper.
Landing ramps change from closed to partially lowered to open while
the authoritative craft moves from z=-3.7 to the real beach z=-2.2.
Gunboats use their existing near-fire attack timer, matching the old
muzzle-flash predicate; this removes the duplicate 3D flash box.
Aircraft select neutral/left/right wingtip silhouettes from the live
simulation bank angle, with the existing small world-space roll
retained. A read-back of one frame never increments animation and
both eyes see the same selection. Existing hitboxes, attack windows,
target ray, infantry spawn, saves and movement are untouched.

Added native C++ test cases for all pose transitions, atlas mask
coverage, unique appearance, per-tile UV bounds, invalid bank/cooldown
handling, 128 KiB atlas budget and unused-cell transparency. Full
Android native/NDK build, Galaxy S9 sprite readability and frame-rate
evidence are still required; source-level checks do not establish
physical-device performance.

## 360-degree atmospheric sky (2026-10-15 source)

The first front-facing ten-strip sky wall is replaced with
`breakwater_sky.h`'s complete sixteen-sector, seven-band cylindrical
panorama, at world radius 25.6, behind the sea's farthest z=-24.08 strip.
Head yaw/roll therefore cannot reveal the old horizontal side edges.
The same render recipe adds 8–12 far low-poly cloud cutouts, a fixed
sun or moon disc, 24 tiny nighttime star diamonds, and a few distant
smoke plumes (three at Iron Quay, one at Blackcap Island, none at
Shingle Bay). The three coastlines each have a distinct atmospheric
cast, day/night gradients and deterministic appearance.

The panorama uses **270–312 vertex-color triangles** in the existing
world mesh and **no new draw call, atlas cell, shader, dynamic texture,
input surface, gameplay collider or CPU simulation tick**. It is
generated once for the two stereo eyes. Clouds, sun/moon and smoke are
2D cutout geometry in cylindrical world space, not finished authored
image-generation assets or per-eye head-facing stickers. Coastal
terrain, combat selection volumes, mounted gun and boat/infantry
simulation remain unmodified. At the present radius the sky is a
world-anchored VR panorama, not a head-position-translated skydome;
seated movement and reference-device comfort remain acceptance gates.

A matching isolated C++17 `-Wall -Wextra -Wpedantic -Werror` fixture
passed for six day/night-location combinations, exact triangle budgets,
determinism, +X/-X/+Z/-Z cardinal coverage, finite bounded positions,
palette distinctions and invalid-location empty output. Full native
suite, Android NDK compile, far-plane clipping and phone headset
stereo/thermal proof remain **unverified**.

## Runtime art and visuals

Follow the tiny pre-PlayStation, early-PC 3D aesthetic: simple vertex-color meshes, procedurally striped sea, day/night palette, chunky silhouettes and a 3×5 generated pixel-card text renderer. Future recipe-driven additions: richer weather, authored cloud variants, per-location distant silhouettes, procedural RGB565 materials and sprite-based effects. Landing ramps, wakes, impact effects, coastal terrain and the first 360-degree clouds are already implemented. No large texture packs, modern PBR, heavy shaders, imported licensed game materials or duplicated per-eye scene simulation.

## Breakwater sound-effects acquisition and integration queue (2026-10-15)

**Source implementation (2026-10-15):** ReverieVR's Java
`NativeCombatWaveform` generates ten compact 22,050-Hz PCM16
mono sounds at module initialization: the original generic
`FIRE`, `RELOAD`, `READY`, `ARTILLERY` and `AIRSTRIKE`,
plus original differentiated `MACHINE`, `HEAVY`, `FLAK`,
`MISSILE`, `CANNON` reports. `NativeCombatSound` preloads
them into bounded static `AudioTrack` voices and routes them
through `UiFeedback`. The optional append-only host
`request_sound(cue_id)` path accepts ordered weapon IDs from
successful simulation shots; the old `request_feedback(WEAPON_FIRE)`
still serves older hosts. Shell focus/confirm/reject sounds remain
independent. **No third-party recorded Breakwater SFX have been
imported, compiled or bound.**
`Mars des Souvenirs` belongs to owner-original background MUSIC and is
a separate catalog/playback concern, not a sound-effects substitute.

**Source policy:** CC0 1.0 is preferred for every asset committed to this
public source repository or its redistributable APK. Explicitly capture
per-file creator, original page, license, local filename, content SHA-256,
edit/normalization recipe and accepted in-game use. Do not assume
"royalty-free" means redistributable as raw public-repository audio.
Do not hotlink or ship entire source packs. Candidate sites must be
auditioned; being CC0 does **not** mean an effect is suitable or already
downloaded. The authoritative, directly linked README-style source record is
[Breakwater Sound Sources](BREAKWATER_SOUND_SOURCES.md): twelve individually
identified candidate pages, source authors, CC0 claims, cue-to-source
priorities, audition gates and one naval sound pack **held for provenance
review**. Keep source/licensing links there rather than duplicating a
second candidate list in this specification.

Do **not** put Sonniss GDC bundle WAVs or other library-licensed
raw effects in public GitHub as reusable assets: its current EULA allows
game synchronization but prohibits independent sound-library/sample
redistribution. Those could be considered only in an appropriately
licensed finished-game packaging process. Avoid firearm recordings
with uncertain provenance, unverified Freesound variants, third-party
music embedded in SFX, and overly futuristic sci-fi effects.

### Ordered in-game cue queue

| Order | Cue IDs / design | Exact playback trigger | Current status |
|---|---|---|---|
| P0.1 | `gun_machine`, `gun_heavy`, `gun_flak`, `gun_missile`, `gun_cannon` | A **successful** `BeginGunShot` / `FireAt` / `FireMiss`, identified by active `WeaponKind`; respect actual fire cadence and suppress rejected fire | Shared procedural `FIRE` exists; distinct recorded samples not ingested |
| P0.2 | `impact_metal`, `impact_sand`, `impact_water`, `boat_destroyed` | Real `ImpactEffects` events, projectile/water collision or lethal target impact; never speculative proximity | Visual events exist; recorded audio unbound |
| P0.3 | `battery_hit`, `gunboat_salvo`, `infantry_volley`, `aircraft_bomb`, `aircraft_strafe` | Simulation `BatteryDamageFeedback`, `GunboatSalvoEvent`, `InfantryVolleyEvent`, `AircraftAttackEvent` **only after positive damage**; do not double-trigger for one sequence | Native authoritative events exist; recorded audio unbound |
| P1.1 | `boat_motor`, `row_oars`, `ramp_drop`, `aircraft_flyby` | Actual visible enemy travel or landing state, per-actor distance/pan and gated lifecycle; no per-frame re-trigger | No ambient/vehicle loops yet |
| P1.2 | `reload_mechanism`, `reload_ready`, `shop_purchase`, `action_reject`, `sam_launch` | Explicit successful/failed action and reload transition; never click noise for passive selection | First two synthesized; generic shell UI cues exist |
| P2.1 | `surf_loop`, `coastal_wind`, `night_air`, optional `gull_distant` | Location and day/night state, bounded loop voices, lifecycle focus/mute transitions | Not implemented |
| P2.2 | `mission_start`, `wave_clear`, `victory`, `defeat`, optional radio stingers | Real campaign phase transition once, never per-eye/per-frame | Not implemented |

### Implementation dependencies and nonnegotiable gates

1. **Select and clear** a *small* number of individually licensed CC0
   candidates; audition them for plausible generic late-1990s/2000s
   Western military texture, avoid copied recognizable weapon/vehicle
   recordings, keep the author/source/license text and SHA-256 alongside
   any admitted binaries. No license audit workflow or mass pack mirror.
2. Normalize peaks, trim silence, and build very short mono effects for
   immediate cues. Evaluate compact compressed assets for longer loops.
   Preserve local/offline operation: playback must work without PC,
   network or extraction from the APK.
3. The generic native feedback path remains a small coalesced
   **bitmask**, not an audio bus. A separate, optional ABI-v1 tail
   now dispatches ordered `request_sound(cue_id)` requests for the
   five weapon types; it preserves duplicates with a bounded queue,
   reuses the shell mixer, and falls back to `WEAPON_FIRE` on older
   hosts. Do **not** repurpose legacy feedback bits as sound IDs.
   Broader typed NPC, spatial and recorded-sample routing remains
   future work, and source implementation is not device acceptance.
4. Trigger at most once per real sequence or shot; avoid sound in
   `BuildScene` / per-eye `Draw`. Limit simultaneous voices, prioritize
   player shots, incoming damage and target hits over distant ambience,
   and stop/fade loops on Quick Menu, pause, route loss and game exit.
   Retain synthesized host cues as graceful fallback if sample loading
   fails. Avoid a new settings screen until actual audio-mix controls
   exist.
5. Gate promotion on the existing Android ARM NDK/Java build, successful
   event-to-sound integration tests, verified original file provenance,
   signed installable APK read-back, headphone stereo/latency test,
   music coexistence, 15-minute Galaxy S9 load/thermal test and APK-size
   comparison. **Nothing in this intake queue counts as an ingested
   audio file, an audible cue, or a released APK.**

## Original MIDI-era music for every native game

Soundtracks should evoke **early Sierra MIDI composition** through new melodies and lightweight note-event sequences, not copied tunes or proprietary instrument banks. Breakwater cues: harbor brass title, shop intermission, daytime battle, night assault, aircraft-warning sting, victory/defeat. Red Ledger: weary smoky noir. Lantern Desk: chamber-intelligence tension. Forward Detachment: measured snare and low winds.

**Actual native MIDI playback does not yet exist.** The current module has host-owned feedback cues only. The shared note-event sequencer, software synthesis and audio-focus service must be implemented and measured on the S9 before calling music functional. Reference: docs/native/MIDI_MUSIC_STANDARD.md.

### Explicit restart and checkpoint invalidation (2026-10-14 source)

Selecting **FIRE TO RESTART** after victory or defeat now starts a fresh
difficulty-selection briefing and atomically invalidates the prior saved shop
checkpoint. The game uses the existing ABI-v1 host's supported zero-byte save
write at the original `breakwater-v1.bin` slot, not a new delete callback or
save schema. A successfully read empty slot means "no campaign to resume";
a missing slot also starts fresh, while malformed non-empty saves still fail
closed. Choosing the next difficulty writes a new normal 80-byte shop snapshot.
If the host supports saves but invalidation fails, the game still restarts in
memory but reports failure through the shell cue and native log; reopening
before a successful new snapshot may resume the old campaign.

The checkpoint helper has C++17 regression assertions for the zero-byte host
write contract, successful/failed slot interpretations, missing/invalid host
service and write errors. An isolated host-side fixture passed; the full native
suite, Android APK, restart/relaunch persistence and Galaxy S9 proof remain
separate gates. No other game, music, GL or controller behavior changes.

## Acceptance gates

1. Native C++ tests for boat HP, 1-passenger rowboat, 3–6 military landing party, prevented landing, individual infantry allocation/advancement/ray-hit discrimination, infantry danger, upgrade behavior, turret and anti-air restrictions.
2. Android ARMv7 + ARM64 NDK build and signed installable phone-test APK, verified by remote read-back.
3. Independent Development-only launcher list and runtime gating.
4. Galaxy S9 + Daydream: stereo, seated comfort, shop Select, held fire, real boat sinking, shore-landings, day/night progression and emergency shell recovery.
5. Campaign save/update durability, actual projectile effects, audio/MIDI ownership, 15-minute thermal/frame pacing and battery test.

Do not promote the prototype to the normal launcher until the hardware gates have been met.

### Confirmed enemy-destruction wreck fragments (2026-10-15 source)

The original impact pool now has an adjacent, game-private
`breakwater_wrecks.h` visual layer. An aircraft, rowboat, landing craft
or gunboat emits eight short metal-shard triangles plus a transient smoke
triangle **only when a real, previously active enemy's HP crosses to zero
and the authoritative campaign deactivates it**. The hull disappears from
gameplay immediately, as before; debris remains anchored at its last
actual simulation position and gradually spreads/sinks or falls for 1.8
simulation seconds. The existing hit/explosion effect still plays
separately. No body or fragment becomes a new target.

All destruction paths route through the same event check: successful
player gunfire, SAM kills, artillery and airstrike batch results, and
simulation-owned support turret kills. Disembarkation, offscreen
aircraft departure, ship arrival, mere damage and rejected calls create
no wreck. Invalid coordinates cannot place debris, and a fixed eight-slot
pool drops additional visuals under saturation rather than evicting
currently visible wrecks. Each pulse generates at most nine triangles,
or **72 triangles total**, in the existing untextured geometry batch
shared by both stereo eyes. Wreck lifetime advances only in normal
unpaused campaign updates; it is cleared at new campaign and wave
start. No new atlas, shader, GL draw, sound, control binding, gameplay
HP, collision sphere or save field is introduced.

Native regression source covers kill-vs-damage and landing guards,
invalid input, location retention, aircraft/boat geometry, finite
vertices, lifetime expiry, pool saturation, slot recovery and reset.
Full Android/NDK build, Galaxy S9 stereoscopic debris readability,
performance and thermal acceptance remain **unverified**.

### Impact pool saturation behavior

The eight-slot impact pool prefers expired entries, then active minor effects. Minor sparks, dust, and splashes cannot evict live explosions when the pool is full; fresh explosion and support-blast effects may replace existing blasts when necessary. This is deterministic and allocation-free. Native regression tests cover retention, saturated rejection, and slot reuse after expiration. Physical headset validation remains outstanding.


### Authoritative gunboat salvo visuals

Offshore gunboats now emit a sequence-numbered firing event only when their simulation actually removes battery integrity. The event preserves the firing vessel's world coordinates and the real damage amount (including a fatal salvo against a nearly destroyed battery). The native renderer uses it to draw a short-lived amber tracer from that vessel to the defensive parapet and a metal impact there, advancing the effect once per update for both eyes. Intercepted vessels produce no false salvos; the event resets on a new wave and does not alter the 80-byte campaign save format. Native regression assertions cover firing coordinates, repeated salvos, intercepted vessels, frame subdivision, and fatal damage clamping. Galaxy S9 stereo alignment and legibility remain device acceptance gates.

### Damage-verified infantry impact feedback

Infantry volleys now record their actual battery integrity loss, capped at the remaining hull, alongside the live firing soldier and shooter count. The simulation emits no infantry hit event when another attack has already reduced integrity to zero. The VR renderer retains the shooter muzzle flash and adds a brief metal impact on the parapet at a position derived from the actual shooter. Both effects advance once per update for stereo rendering, with no new textures or allocations in the effect pool. Native regression covers an ordinary damaging volley and a fatal volley against one remaining integrity point. Galaxy S9 stereo/visibility acceptance remains pending.

### Battery impact feedback

Breakwater now derives a brief **HULL HIT -N** warning from the authoritative campaign integrity delta after each simulation update. The warning persists for 0.65 seconds and shows the real damage amount from gunboats, aircraft or infantry; no warning is generated by harmless shots or mere threat predictions. A lethal hit is also reported on the defeat panel. The display state is advanced once per update, not once per VR eye, and resets on campaign restart or new wave. The native regression fixture covers unchanged/invalid integrity, multiple successive hits, expiry, and actual gunboat/bomber damage. Galaxy S9 readability remains a device acceptance gate.

### Shore infantry attack countdown (2026-10-12 source)

The combat HUD now displays a small **SHORE Ns** warning in the six-second
window before a genuine infantry firing-line salvo. The campaign computes
this from the actual disembarked soldier's maneuver plan: riflemen follow
the original z=-1.34 / 0.13-unit advance, flankers must also reach
their lateral lane, and support gunners fire from z=-1.68. The four-second
shared salvo interval remains unchanged. Once any soldier has reached
its real firing station, the display follows the actual shared
salvo timer, including resets after an attack. Sinking a transport before
landing never creates a phantom infantry warning; eliminating every ready
shooter cancels their partial countdown and unready survivors must still
travel and then wait a full salvo. The low-poly HUD appears below, rather
than overlapping, aircraft/gunboat threat panels, and is generated once per
simulation update for both eyes. Existing 80-byte shop saves and host
controller bindings are unchanged. Pure C++ prediction tests passed in a
local -Werror focused fixture; integrated campaign compilation plus actual
Galaxy S9 input, legibility and frame pacing remain unverified.


### Distinct surface-vessel and infantry silhouettes (2026-10-12 source)

Game-private, allocation-free C++17 recipes in
app/src/main/jni/native/breakwater_silhouettes.h now build distinct
vertex-color bodies without a model file or texture atlas: narrow pointed
rowboats with a single embarked human; wide military landing craft with
high gunwales, bow ramps, and visibly represented 3–6 passengers; and
long patrol gunboats with wheelhouse, windows and a forward cannon.
Surface vessels trail two minimal foam wedges. Active shore soldiers
have separate helmets, arms and split legs. The existing simulation
positions, passenger manifests and sphere-hit tests remain authoritative.
Embarked silhouettes are decorative, not independently shootable infantry.
Inactive ships, dead infantry and aircraft do not emit these recipes.
They require no RNG, GL calls, dynamic allocation or additional draw calls:
generated vertex data is appended to the existing shared CPU mesh and VBO
once per game update, not independently for the two stereo eyes.
At most 16 box primitives and four direct triangles per surface vessel,
and two boxes plus five triangles per living shore soldier. This additional
CPU mesh work must be profiled on the actual Galaxy S9. Integrated native
test source covers recipe counts, passenger scaling and invalid/inactive
entities; an isolated C++17 host fixture passed using compatible type
stubs, not the full NDK build. Headset silhouette legibility, hitbox
alignment and thermal performance remain open.


### Position-driven low-poly model motion (2026-10-12 source)

The game-private breakwater_silhouettes.h recipes now provide three
deterministic pose helpers. LandingRampProgress raises the landing-craft
bow ramp while offshore and lowers its free edge over the last 1.5 world
units of approach, from z=-3.7 to the actual simulation landing line at
z=-2.2. VesselWakePhase changes the shape of the existing two foam
wedges as a function of the live hull's z position. InfantryMarchStride
alternates left/right limb positions as a function of the individual
soldier's live x/z location, then holds the legs still from the actual
front-line z=-1.34 onward. These are procedural geometry poses, not new
units, hitboxes, physics, or combat animations. Invalid/nonfinite
coordinates and inactive entities produce no animated geometry.

No animation timer, per-eye state advance, new GL draw, extra triangle,
bitmap, model dependency, or save-format field was added. The geometry
still enters the existing once-per-update scene mesh and is reused for
both eyes. Because the animation phase follows displacement, motion stops
when simulation movement stops and remains deterministic across
10 FPS catch-up substeps; this does not prove its headset appearance.
The pure geometry fixture passed locally with C++17 -Wall -Wextra
-Wpedantic -Werror, and the corresponding campaign-test assertions
compiled/passed in a separate isolated harness. The complete native
campaign/Android build, S9 stereoscopic legibility and actual
sustained performance remain unverified.


### Three distinct procedural shore environments (2026-10-12 source)

The native campaign's Shingle Bay, Iron Quay and Blackcap Island now have
different **actual world-space shore geometry**, not merely different location
labels over the same open sea. The original C++17 game-private recipe in
`app/src/main/jni/native/breakwater_terrain.h` generates six two-triangle
shore strips above the sea surface, eight tiny surf-edge wedges at the real
z=-2.20 transport landing line, and location-specific details. Shingle Bay
has pale shingle chips and driftwood, Iron Quay has masonry seams and rusty
outer-edge mooring posts, and Blackcap Island has dark basalt outcrops.
Existing individually targetable soldiers traverse the central beach without
decorative structures obscuring their assigned gameplay hitboxes.

All three terrain palettes shift for day/night; placement, geometry counts and
colors are deterministic without a new RNG, texture, material dependency,
physics layer, target or save field. The recipe is appended to the existing
once-per-simulation-update vertex-color mesh, and both stereo eyes reuse that
one buffer with no extra draw calls. Nominal terrain budgets are 38 triangles
plus two box primitives for Shingle Bay, 25 triangles plus eight boxes for
Iron Quay, and 28 triangles plus eight boxes for Blackcap Island; generated
box primitives are still expanded using the existing lightweight Box emitter.
Native test source checks location identity, night colors, counts, finite
geometry, color bounds and invalid location rejection. Full native/NDK build,
physical Galaxy S9 binocular rendering, target visibility and thermal
acceptance remain outstanding.


## Intra-period wave escalation (2026-10-16)

The long nine-day campaign now escalates *within* the same day/night period, not merely when the next period begins. `breakwater_wave_threat.h::SelectInvasionUnit` retains the original deterministic per-spawn roll and day/location/difficulty base grade, then adds three bounded pressure bands as the player advances through that period's waves. The opening two grades remain one-soldier rowboat-only, and the first wave of every period preserves the previous spawn choice at that grade. Once landing craft, gunboats and aircraft are eligible, late waves favor heavier mixtures; aircraft can begin arriving toward the back of a suitably advanced period. Each wave still has the same fixed ten enemy slots, soldier/armor caps, save layout and shop handoff, and does not instantiate extra craft.

The same source selector now drives the actual campaign spawn loop. `breakwater_wave_threat_test.cpp` verifies unchanged opening fairness, previous first-wave probabilities, all pressure transitions, representative new late-wave threats and deterministic bounded results. The host visual script includes the new test; Galaxy S9 pacing, perceived balance and full APK behavior still require device validation.

### 2026-10-16 — late-war convoy pressure and interruptible heavy beaching
The original four wave-pressure bands remain in service. The highest band now uses a distinct high-bit mix of the same deterministic seeded choice for gunboats, avoiding the former shared modulo-three aircraft/gunboat collision that eliminated gunboats at high threat grades. Heavy transport promotion now occurs *after* ordinary pressure-band selection, replacing only rowboats or normal landing craft instead of stealing aircraft/gunboats, with a guaranteed first-slot flagship on qualifying final waves. Heavy armored transports stop at the shoreline for a 1.25-second exposed ramp-open unloading phase: their real 3D hull remains targetable, offshore gunfire/threat warnings cease, and both armored cars plus four infantry deploy only if the ship survives. One real beaching cue reuses the native shoreline collision sound and water impact. The 20-car and 60-soldier capacities and existing 80-byte shop save are preserved. Mars des Souvenirs and the global Home menu are untouched. Host full-Campaign, Android APK and Galaxy S9 device acceptance remain unverified.

## 2026-10-16 — 360-degree location environments and world-space day/night illumination

Breakwater now uses its pre-existing 360-degree `EmitSkyPanorama` together with the new game-private `breakwater_environment.h` landscape generator. Every playable campaign location has two fully closed 48-sector silhouette rings (hills at radius 17 and mountains at radius 22), 12 joined rear-ground strips, and a distinctive landward skyline with physical world geometry. Sky radius 25.6 keeps the atmosphere behind the ridges; the front sea lane remains unobstructed. Shingle Bay uses lush coastal hills, pines and a service track; Iron Quay uses industrial slate hills, concrete supply sheds and quay edges; Blackcap Island uses sharp volcanic basalt ridges and rock spires. A campaign-specific sea palette, revised beach colors and differentiated off-lane shoreline shoulders make the beaches visibly different in addition to their pre-existing props/cover.

Day palettes now use bright sky blues and vivid coastal colors; night uses deep navy-blue atmosphere, moon-tinted ground/sea, and a separate low-cost vertex-color illumination pass for physical gun models, enemies and scenery. Combat flashes and HUD lettering retain brightness and do not receive this environmental attenuation. Environment vertices are authored deterministically once per scene rebuild and reused by both stereo eyes, with no texture assets, new shaders, per-eye CPU simulation, draw call, platform API, save-format or controller change. `Mars des Souvenirs` remains assigned; the global Home UI is not changed.

The environment geometry header's exact remotely committed blob (`559fbaafd19c6fc3c77ad824b6dc459a2521a1c0`) was compiled and executed using GCC and Clang C++17 (`-O1 -Wall -Wextra -Wpedantic -Werror`), plus Clang AddressSanitizer/UndefinedBehaviorSanitizer, with a minimal isolated `Campaign::kLocations`/sky geometry stand-in. The standalone new test also lives in the repository's existing local visual-verification runner. This proves isolated deterministic geometry and bounds only; full Android native compile, GLES2 draw, APK and Galaxy S9/device comfort/performance remain **unverified**.
