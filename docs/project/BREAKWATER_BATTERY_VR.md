# Breakwater Battery VR — canonical native game specification

**Status:** Development-only VR prototype, physical Galaxy S9 acceptance pending. **Backlog:** RV-0640. The authoritative implementation is in app/src/main/jni/native/breakwater_campaign.{h,cpp} and breakwater_module.cpp. This game uses original assets/code and does not import Beach Head content.

## Pitch and campaign structure

A stationary coastal-defense gun battery faces a gradually escalating amphibious invasion. The player is seated, scans the horizon by turning their head, and aims the gun using the shell-calibrated handheld pointer. No artificial walking or jolting camera recoil.

The campaign has three defended locations: **Shingle Bay (two days), Iron Quay (three days), and Blackcap Island (four days)**. Every day and night is a separate wave period with a shop after **every** wave. Initial Casual/Regular/Veteran/Siege wave schedules per day are 2/3/4/5 and per night 1/2/3/4, increasing on later maps and days with a ceiling of eight per period.

Briefing → inter-wave Shop → Combat → Shop → next wave → sunset/night → sunrise/day → next location → Victory. Integrity falling to zero means Defeat. Fatal aircraft strikes, offshore gunboat salvos and gunboat shoreline collisions end the current simulation substep immediately after recording real damage. Later active attackers and infantry do not move, fire or disembark after the battery is destroyed; the terminal impact event remains available to the renderer. Native regression holds a second already-spawned attacker stationary on a fatal salvo. Nothing automatically skips the shop.

The native input contract permits frame deltas up to 0.10 seconds. The campaign now advances that time in bounded 0.05-second substeps, rather than silently dropping half the elapsed time at 10 FPS. This preserves boat movement, spawn cadence, reloads and hostile attack timing on slow frames, while capping long pause/resume catch-up to 0.10 seconds. Deterministic native regressions compare one slow frame against two half-frames, including offshore gunboat salvos. Headset frame-pacing acceptance remains outstanding.

## Enemy transport rules

**The opening enemy is a rowboat, not a robot.** A rowboat carries exactly **one infantryman**. Stronger military landing craft appear later, each carrying **three to six infantry**. Ships have real position, speed and hull HP. **Sinking the craft before it beaches cancels every passenger**, while a surviving craft unloads its passengers as separate hostile shore infantry. Each passenger has an individual in-wave soldier slot and world position derived from its landing craft, advances from the surf toward the parapet, and can only be killed by an actual ray hit against that soldier. The old generic beach-region auto-kill is removed. Infantry only damage the battery after individually reaching the forward firing line at z = -1.34; while they are advancing across the beach, their attacks are disabled. The four-second salvo countdown begins when at least one living soldier reaches that line, and killing every ready shooter resets it. Each salvo's damage scales only with soldiers actually at the firing line, not all landed soldiers. Every actual infantry salvo now records a sequence, an eligible firing soldier's world-space position and the number of participating shooters; the VR scene shows a short generated muzzle flash at that position only when the authoritative volley fires. This adds no textures and advances effect lifetime once per update for both eyes. Native regression tests reject imaginary early flashes and confirm the fired event comes from a living soldier on the firing line; reference-headset visibility is still unverified. Later progression introduces gunboats and overhead aircraft. Gunboats fire from offshore once within eight world units, dealing three integrity damage per salvo every 2.8 seconds. Surviving gunboats still cause eight integrity damage upon reaching the shore. A gunboat that actually reaches the shoreline now emits a separate collision event with its last position and actual integrity loss (capped at the remaining hull); the renderer shows a brief shoreline explosion from that event. Sinking it before shore prevents the collision event and blast. The effect is advanced once per update and shared by both eyes; same-frame collisions can coalesce to the last event. Source regression covers surviving collisions, destroyed gunboats and rejected phantom blasts; Galaxy S9 visual acceptance remains pending. Each simulated salvo has a short procedural muzzle flash and its own wave-status counter; destroying a gunboat before it reaches firing range prevents that bombardment. Aircraft now resolve as deterministic **strafers** or **bombers**: strafers pepper the battery repeatedly during one fast overhead pass, while bombers make a slower approach and deliver one heavier attack before departing. Destroying the aircraft before its attack window prevents that damage. Aircraft attacks now produce a simulation-owned event only when a bomber or strafer actually damages the battery, recording the attacking aircraft's coordinates, attack pattern, damage and sequence. The VR renderer turns those events into brief parapet sparks for strafing or an expanding explosion for bombs, reusing the existing fixed-size effect pool. Intercepted aircraft and non-attacking passes produce no false blast; concurrent same-frame attacks can coalesce to the last event. Native tests cover actual attack events, damage and successful interception; Galaxy S9 visual acceptance remains pending. Aircraft require an anti-air upgrade for gun damage or a purchased airstrike; firing the basic gun at one still consumes ammunition but cannot harm it. The anti-air line now has three permanent tiers: tier 1 unlocks aircraft engagement, tier 2 adds 15 damage per aircraft hit, and tier 3 adds 30 damage per aircraft hit; these bonuses never change surface-vessel damage. On a 100-HP aircraft without other upgrades, the number of hits falls from five at tier 1 to three at tier 2 and two at tier 3. Aircraft now also follow seeded lateral courses: bombers make shallow course corrections and faster strafers weave through a wider approach. Each aircraft's x position and banking angle are deterministic functions of its authoritative forward progress and seeded direction, so slow-frame substeps, aim hitboxes, muzzle/strike coordinates and both stereo eyes share the same flight pose. The tiny generated silhouettes use swept triangular wings with banked tips; bomber engine pods follow their corresponding wing height. This adds no asset/texture packages or per-eye simulation. More elaborate flight geometry, animation and headset visual validation remain ongoing.

The module generates simple cuboid rowboats, military boats and individually positioned shore infantry silhouettes (body and head), plus distinct generated bomber/strafer silhouettes with dynamically banked, swept vertex-color wings. Rendered infantry positions and bounded combat-ray hitboxes share the campaign's authoritative soldier objects, and defeated soldiers are removed individually. Combat targeting now selects the nearest positive ray-surface intersection rather than hitbox-center depth for overlapping vessels, aircraft and infantry. The shared deterministic intersection helper rejects malformed rays; Galaxy S9 pointer alignment and headset hitbox proof remain outstanding.

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

The ballistic foundation now defines five original mounted weapon profiles: MACHINE (12-round general-purpose MG), HEAVY (8-round heavier MG), FLAK (6-round dedicated AA), MISSILE (two guided rounds with independent secondary/ghost sight), and CANNON (four-round direct-fire anti-vessel gun). Their initial progression prices are 0, 900, 1,350, 2,200, and 1,600 credits. The standard MG is not a magical laser: its approximate trajectory drops 0.014 world units per squared unit of flight, which means a long-distance vessel requires leading the shot *upward* to compensate. A pure, deterministic projectile-to-sphere solver rejects straight crosshair hits when the round passes underneath; flak drops far less, and missiles use a straight guidance ray. The cannon has a much stricter elevation limit and the independent ghost sight remains aligned with the real input ray even when the physical mount cannot follow it. Weapon ownership and active selection are now simulated with real shop spending: the standard machine gun starts owned, other mounts must be purchased, weapon changes preserve per-weapon ammunition, and the SAM battery has a capped three-charge inventory at 540 credits per charge. The 80-byte shop save now writes version 2 with the ownership mask, selected mount and SAM inventory in formerly unused slots; the old version-1 format still loads. Snapshot loading validates the complete 32-bit wave index before narrowing it to the runtime byte: corrupted values such as 257 or 65537 must not masquerade as wave 1, and rejected snapshots leave the current shop state unchanged. The in-world shop now has an ARMORY page with actionable cards for purchasing/selecting five mounts and buying SAM charges. In combat, five compact weapon cards switch among purchased equipment; SAM is a two-step support action: press SAM to arm the ghost sight, aim that independent pointer at an actual aircraft, then press Select to fire. Press SAM again to cancel. An invalid lock never spends a SAM charge, and ordinary cannon elevation remains limited even while the ghost marker points up. SAM target confirmation now consumes exactly one Select press, whether the aircraft lock succeeds or fails: holding Select or dragging the pointer cannot silently retry a failed lock, spend another SAM charge, or fire the selected gun after a successful missile launch. Release and press Select again to retry. Each weapon has its own fire cadence, magazine and ballistic gravity; existing gun damage/clip/rate upgrades still apply. Curved fired tracers, target hits and water misses use the same dropping shot path rather than a straight laser. Missile battery targeting and SAM use the unrestricted ghost ray, while flak has a larger elevation arc. These are source-implemented controls with native and Android/device validation still required.

The gun carriage is world-anchored and the rendered barrel follows the handheld pointer only inside a finite traverse/elevation envelope; firing outside that mount arc is rejected. Successful gun shots create a short-lived generated tracer ribbon, including misses, while the 1.15-second automatic magazine reload makes clip-size upgrades meaningful. In combat, a secondary-button press now requests a tactical reload of a partially spent magazine using the same 1.15-second simulation timer. Full magazines, active reloads and non-combat phases reject the request without changing ammunition; shooting remains blocked until completion. The combat HUD shows SECONDARY RELOAD when a partial magazine is ready for manual reload, with the existing RELOADING progress and READY completion cues taking precedence. Native regression assertions cover the request lifecycle and rejection cases. During reload, the combat HUD displays a progress bar and RELOADING label, driven by the simulation timer and updated once before stereo rendering; firing remains blocked until the magazine refills. A fixed pool of eight procedural impact pulses produces sparks only for actual vessel/aircraft damage, infantry dust and sea splashes for downward misses. Lethal gun hits on hulls or aircraft now produce longer expanding smoke-and-fire explosions; rounds that cannot damage aircraft without the anti-air upgrade consume ammunition but no longer invent hit sparks. Impact positions are captured before lethal hits deactivate targets, and the pulses advance once per update rather than separately per eye. Horizontal and upward misses do not invent sea splashes. Native regressions cover damage-gated impact kinds, blast lifetime, pulse position, expiry, malformed input and capacity; headset visibility remains unverified. Artillery and airstrikes now emit separate, longer-lived procedural blast pulses for each actually damaged enemy (up to the fixed eight-pulse capacity), using pre-strike coordinates so sinking boats and destroyed aircraft still explode at the correct location. Artillery flashes amber at the waterline; airstrikes use a pale overhead flash for aircraft and surface targets. Rejected support calls and artillery-immune aircraft produce no false blasts. Native regression fixtures cover both support types, target locations, lifetime, and rejected calls; physical headset visibility is still unverified. The combat HUD now briefly shows READY for 0.55 seconds when the simulation confirms that the magazine has finished reloading; this cue does not trigger on ordinary idle frames or when combat has already ended. Native gameplay cue flags now drive distinct original generated gunshot, reload-start, reload-ready, artillery bombardment and overhead airstrike audio through reusable Android static AudioTracks. The 180ms gunshot uses two preloaded alternating voices so the fully upgraded 160ms firing cadence no longer truncates the previous report; the other cues retain one static voice each. PCM is synthesized once per cue when the bank initializes, with no per-shot allocation or stereo-eye synthesis. Host-side voice rotation and lifecycle were exercised with Android media stubs; actual Galaxy S9 mixing and latency remain unverified. Successful artillery and airstrike calls request their own shell-owned cues; rejected calls retain the failure cue without playing a support blast. Only valid rounds, successful reload requests, emptied clips, and simulation-confirmed ready transitions request those cues; failed actions preserve rejection feedback. Local/static and S9 audio latency/mix acceptance remain pending. Three AA tiers and seeded banking aircraft flight are implemented; additional aircraft types and richer airborne engagement geometry remain planned. An 80-byte versioned shop-state save preserves completed waves and bought upgrades through the module-private host save service. Mid-wave exit resumes at the prior shop; S9 exit/re-entry and APK update durability remain device gates.

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

## Sprite atlas and 2.5D mounted weapons — current source (2026-10-15)

**Implementation:** `breakwater_sprite_atlas.h` produces an original, fixed
128×256 RGBA atlas with 24 occupied 32×32 cells (eight reserved): two
infantry march frames; rowboat and travel-driven oar stroke; landing
craft with closed, half-lowered and open ramp; gunboat and firing flash;
neutral/left-bank/right-bank strafer and bomber; five mounted weapons;
and four artillery/airstrike/SAM/repair support symbols. Their muted olive, slate-steel, sand and
gunmetal shapes are generic late-1990s/early-2000s American/European-inspired
military arcade styling, **not replicas, military insignia or imported art**.
Art can later be replaced cell-for-cell without changing the gameplay ABI.
Currently these are code-authored pixel sprites, not image-generator files.

The native Breakwater GLES2 module now uploads this atlas once per GL context,
renders one hard-alpha-masked world card per live enemy and infantry slot,
and batches all cards into **one extra textured draw per eye**. The same batch
now also carries the selected weapon's near-field, controller-reactive body
card, compact selection icons for all five weapon slots, and dedicated
artillery, airstrike and SAM combat-HUD icons. The existing shop pages
reuse those symbols plus a repair-tool icon next to the matching purchase
amount. All support art is display-only: action rectangles, charges,
credit checks, selection focus and combat logic are unchanged.
The world, shore terrain, water, emplacement and 3D target volumes remain
authoritative and unmodified. Textured cards use original enemy/infantry x/z positions,
world-stable XY planes facing the defended battery, with aircraft roll taken
from existing simulated banking; their position is not independently
recomputed for the left and right eyes. Depth testing and nearest texture
sampling are retained, with fragment discard avoiding alpha-sort/blend
dependencies. The formerly flat forward sky is now a fully wrapped
world-space atmospheric panorama. The gun pedestal and hand-aimed barrel
remain anchored in 3D, while its visible
housing is now an equipment-specific 2.5D foreground card instead of
identical generic boxes for every weapon. It is not finished production
sprite artwork. The world-space housing follows the selected weapon in
Campaign, while controller aim still drives the barrel; no new input
bindings or ballistics are introduced.

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

## State-driven enemy sprite animation (2026-10-15 source)

The original 16-cell (128×128) procedural RGBA sheet now occupies a
128×256 power-of-two atlas (128 KiB) with 24 original silhouettes. Eight
new cells animate the **rowboat oar stroke**, **half/open landing ramp**,
**gunboat firing flash**, and distinct **left/right bank** for both
strafer and bomber. All icons, weapon views, previous sprites and HUD
stay inside the same atlas, with eight additional cells reserved for
future frames. No new sampler, texture, shader, draw pass or gameplay
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
this from each living, actually disembarked soldier's travel to z=-1.34 at
0.13 world units/second, plus the four-second shared salvo interval. Once
any soldier has reached the line, the display follows the actual shared
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
