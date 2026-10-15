## 2026-10-15 — Breakwater state-driven enemy sprite variants (source)

Advanced the existing 16-cell 128x128 procedural atlas into a single
24-sprite 128x256 (128 KiB) texture with eight additional unused slots.
The new variants are rowboat oars (derived from true hull progress),
half-lowered and fully open landing craft ramps (authoritative beach
distance), a gunboat salvo flash (existing attack_timer firing window),
and left/right aircraft-bank silhouettes for both bomber and strafer
(simulation-owned bank angles). Replaced the generic enemy sprite
selection with a pure function; no per-eye animation counters, new
target colliders, combat changes, extra draw pass or asset downloads.
Removed the duplicate 3D gunboat flash in favor of the same timing
inside the sprite. Added native regression coverage for variant
pixel masks/hashes, UV bounds, state transitions and atlas budget.
Full native/Android NDK compile, real S9 appearance and runtime FPS
remain unverified; no APK compiled or published.

## 2026-10-15 — Breakwater full-circle sky and location atmosphere (source)

Replaced the flat, forward-facing sky strips with a 16-sector, seven-band
vertex-color panorama at 25.6 world units, beyond the existing sea
geometry. Added 8–12 cloud cutout triangles per arc, a sun/moon,
twenty-four night stars, and Shingle/Iron Quay/Blackcap atmospheric
variation with distant industrial smoke. The deterministic world-space
sky is generated once per changed scene and submitted with the existing
untextured mesh. No separate texture, shader, draw, atlas growth,
headset-bound object, gameplay state, or collision logic. Isolated
C++17 -Wall -Wextra -Wpedantic -Werror regression fixture passed all six
location/day/night permutations with 270–312 triangles, full 360-degree
coverage, bounded finite vertices and deterministic reuse. Full native
test integration, Android NDK compile, phone clipping and stereo
legibility remain outstanding; no APK was built or released.

## 2026-10-15 — Breakwater compact combat and shop support icons (source)

Extended the existing 128x128 sprite atlas to all sixteen cells without
changing its 64 KiB texture footprint or GL path: artillery targeting,
airstrike silhouette, SAM twin missile mount, and repair wrench. Added
the first three to the existing combat-action HUD positions, and
reused each applicable icon on the matching shop or armory purchase
control, with repair in the normal upgrade shop. Text labels and
values remain legible and action rectangles unchanged. Mapping remains
game-private, with Count returned for all other shop cells instead
of creating inert new controls. Added deterministic C++ regression
assertions for nonempty alpha masks, distinct artwork, bounded
per-cell UVs, shop mapping and invalid slot cases. Full integrated
native test, Android NDK compile, and physical S9 stereo readability
and performance gates are still unverified.

## 2026-10-15 — Breakwater 2.5D mounted weapon artwork (source)

Expanded the previous 7-sprite 128x64 enemy atlas to 12-sprite 128x128
(64 KiB) with original compact machine gun, heavy gun, flak, box launcher
and direct-fire cannon weapon silhouettes. The currently selected campaign
weapon controls one 2.5D foreground gun-housing card, layered over the
fixed mount/pedestal and in front of the existing genuinely hand-aimed
barrel triangles. All five HUD selection panels reuse the same equipment
sprite IDs, existing atlas upload and one shared textured batch per eye;
no HUD tabs, second texture, new GL draw call, ballistics or ABI changes.
C++ regression assertions were added for all weapon mappings, distinct
sprite pixel coverage, UV cell boundaries and invalid input. No Android
NDK compile or Galaxy S9 stereo/comfort/performance evidence was available
from this run; those gates remain explicitly open.

## 2026-10-14 — Breakwater textured enemy sprite architecture (source)

Started the approved sprite-forward art direction with a game-private
GLES2 RGBA atlas (128x64; seven 32x32 cells) and six-vertex cards for
all live infantry/boats/aircraft. Silhouettes use generic, original
turn-of-the-millennium Western military design cues: helmet/vest/rifle,
rowboat, ramped troop carrier, patrol boat, strafer and heavier bomber.
The existing world geometry and campaign hit volumes remain unchanged,
and the single world-space sprite mesh is reused between stereo eyes.
One texture/one additional batched draw per eye, created and destroyed with
the GL context, nearest-neighbor hard-alpha discard. No external art
downloads, imagery claims, or copyrighted model sources. Isolated host
C++17 atlas/billboard fixture passed -Wall -Wextra -Wpedantic -Werror;
full native/NDK build, Galaxy S9 rendering and thermal tests are open.

## 2026-10-14 — Breakwater suspends combat during native pointer loss (source)

The universal Quick Menu sends native modules update frames with POINTER_NONE.
Breakwater previously advanced waves and incoming attacks during those frames
even though the player could not target or fire. Added a game-private
PointerFocusGate and a focused early-out after native input sanitization:
without a usable controller ray, no Breakwater simulation or combat input
advances, while the native shell and rendering lifecycle remain unchanged.
Both game buttons require observed release after focus returns; the same
release policy applies to Android module pause. An isolated C++17
-Wall/-Wextra/-Wpedantic/-Werror focus regression passed, covering retained
buttons, loss and return, independent release and lifecycle pause.
Integrated full native test/NDK build and Galaxy S9 headset/menu testing
remain outstanding; no shared ABI, shell optics, DOS, other games or
battery HUD changes.

## 2026-10-14 — Breakwater restart invalidates old persisted campaign (source)

Inspected native Breakwater's defeat/victory restart and ABI-v1 save callbacks.
The in-memory reset previously left `breakwater-v1.bin` holding the previous
shop checkpoint, which could resume on a subsequent app launch before a new
difficulty had been selected. The native host already accepts and atomically
renames a zero-byte save payload. Added a game-private checkpoint helper:
on explicit restart replace the old slot with an empty one, treat a successful
zero-length read as no active save, and preserve existing normal 80-byte
v1/v2 campaign checkpoint handling. A failed write logs a warning and triggers
failure feedback without blocking gameplay. Tested success, rejected host
services, read-state policy and failed writes in an isolated C++17 -Werror
fixture. Integrated full native/NDK test and signed APK build, device
restart/re-entry and save durability remain unverified.

## 2026-10-13 — White room brought closer to screens; DOS fullscreen audited

Confirmed the current DOS fullscreen rendering uses a complete per-eye viewport, identity matrix rather than a 3D plane, and excludes world room rendering; DOS Back exits fullscreen before opening the in-room DOS options. An explicitly opened universal Quick Menu may remain accessible for safe headset recovery; it is not the DOS options rail. Adjusted only the white room rear backdrop depth transformation from 0.52 to 0.24 (rear wall approximately z=-4.1 instead of -4.8, screens remain near -3). Room furniture, UI panel placement, screen geometry, controller targets, other homerooms, tracked headset optics and power display untouched. Needs signed APK and physical headset acceptance.

## 2026-10-13 — Isolate Daydream inferred gestures from Steam/HID providers

Constrained the new click/double/hold and high-acceleration gesture recognizer in VrInputRouter to snapshots whose provider identifies as Daydream. Non-Daydream snapshots reset held semantic-gesture state, avoiding accidental joystick13–15 controls from Steam or other controllers. Existing raw SELECT, pointer pose, touch and platform Menu/Home routing remains unchanged. Source gate and documentation updated together; #165 release does not include this gate, so next signed source release must include it.

## 2026-10-13 — Finish normalized signed-roll and extreme-look inputs

Added HEAD_ROLL_POSITIVE/NEGATIVE semantic pulses for Cardboard Euler roll beyond ±0.29 radians, rearming below 0.17, with signed naming until Galaxy S9 verifies direction. The existing extreme-up/down and sharp yaw events now pulse BindingInput IDs as well as showing in the ghost overlay; no default game binds these safety-adjacent gestures. Activity still limits sustained head-up axis guest movement to bundled Doom only. Hardened long-press recognition against dropped intermediate BLE packets. No changes to headset pose matrices, controller geometry, power HUD, ghost overlay position or automatic Menu behavior. Android and physical-device validation pending.

## 2026-10-13 — Daydream control index, gesture outputs, ghost overlay and charge HUD placement

Read reference ControllerSnapshot and VrInputRouter capabilities rather than inventing unavailable sensors. Added a bounded recognizer for click tap/double/hold and acceleration shake; routed recognized events through BindingInput normalized guest input callbacks. Native profiles add opt-in semantic joystick buttons 13/14/15 while keeping select, physical joystick and shell Home/Menu behavior. Head extreme and sharp turn events are diagnostic-only. Added a standalone per-eye ghost-grey, six-line middle-left control feed on a separately allocated translucent GLES texture, updated for new events/slow fade ticks rather than constantly. Requested battery HUD relocation is a pure NDC translation of both original arcs/labels, +20% further down and +20% inward; scale/shape and reveal are unchanged. Added tests for position and native action IDs. Physical Daydream false positive and readability checks outstanding.

## 2026-10-13 — Complete DOS fullscreen/options mutual exclusion

The universal Quick Menu -> Bindings route was the remaining transition which could show the DOS configuration pane over an active full-eye guest. This path now sets the DOS renderer back to windowed mode before entering bindings, consistent with physical Back's DOS Options behavior. Only these three authoritative source/docs files changed; immersive binocular viewport math, headset pose, game keyboard mapping, power bar and other UI are untouched. Verify through the signed phone-test APK and S9 reference run.

## 2026-10-13 — User-directed immersive DOS eye fill, closer White Room, and Doom pitch/deadzone

Implemented three behavior corrections as one focused change: fullscreen DOS no longer draws the VR room, covers both complete raw eye viewports using identity NDC instead of preserving aspect margins, and never overlays windowed DOS options; Back leaves fullscreen before presenting options, while selecting fullscreen in DOS bindings/tuning resumes the guest automatically. White Room rear walls/ceiling/floor and wall art move approximately 1.2m closer to the VR screens using a one-time rear-depth compression, retaining identical raycast and display geometry and preserving foreground props and the other two environments. For bundled Doom, held head pitch uses the view forward-Y component to produce guest Up/Forward or Down/Backward keyboard signals (and matching menu navigation) above a 0.21 enter threshold, with 0.147 release hysteresis. Existing left/right mouse yaw remains unchanged. This does not add vertical freelook to the original DOS engine. Added JUnit checks for Doom direction, hold, hysteresis, simultaneous pad/head release, and White Room geometry proximity. No headset view transforms, controller calibration, global power HUD, or unrelated menus touched. Galaxy S9/fullscreen proof still pending.

## 2026-10-13 — Native launch failure hints prioritize shader details

Followed up the six-classic shader diagnostic work by surfacing the native host's `detail=` portion first in the existing headset Home/native shelf footer (which truncates long text). The original complete error is still written to the diagnostic incident for accurate troubleshooting. Successful launch, shader GLSL, module ABI, controller, headset view and charge overlay are unchanged. Signed APK/device acceptance pending.

## 2026-10-13 — Classic-game shader driver diagnostics

Signed Phone Test #160 succeeded, including direct ELF verification of all 24 host/module objects and their exported native launch symbols; handset GLES2 launch remains unconfirmed. Targeted the final blind spot that `ReverieNativeBuildProgram` deletes failed shaders/programs without preserving the Adreno/compiler/linker info log, so a real shader compile failure could still appear as `guest_GL=0x0`. Added optional bounded diagnostics to the existing shared shader builder while retaining the original helper signatures. All six classic games now request the error buffer and emit failures via their existing ABI-v1 log callback; the JNI host retains that bounded guest message only for a failed GL creation callback and appends it to the module/GL code in the existing VR failure notice/diagnostics. Normal successful rendering, gameplay, headset matrices, HUD, and input are unchanged. Android release build and Galaxy S9 test pending.

## 2026-10-13 — APK-native loadability and symbol verification

After signing Phone Test #158, expanded the existing phone-test verification from checking named module `.so` archive entries to checking actual Android ELF module images. The new dependency-free Python helper cross-checks every native host allowlist entry against the Android.mk build target, validates 32/64-bit ARM ELF class and machine types, requires the ABI-v1 C-exported `reverie_native_module_entry_v1` symbol from every module and required JNI exports from the host, and rejects missing private `DT_NEEDED` dependencies within the APK. There are currently 11 native modules across two ABIs plus the native host, so 24 ELF objects are inspected. No module logic, VR headset transforms or battery overlays changed. The added failure gate is static; GLES creation/playability acceptance requires reference Galaxy S9 device proof.

## 2026-10-13 — Native guest GL *creation* isolation joined to render isolation

Phone Test #158 is signed and published (source a6971581deba1bd1864d8e122fe16c312e16900a); all 11 native shared libraries are present for each supported ABI, and the VR failure notice is packaged. Audited the native JNI host's lifecycle and identified that shared GL error ownership was only isolated before `render_eye`, while Test Chamber and several games inspect GL errors during their earlier `on_gl_context_created` callback. Added a bounded pre-guest error guard to both entry points using the existing warning-once discipline, and module-specific GL creation failure diagnostics. Guest errors still fail normally: no ignore, retry or no-op. GL host code and normative documents updated together. Signed build and actual Galaxy S9 execution still required for this new correction; no headset camera/charge-HUD/input changes.

## 2026-10-13 — Native launch failure made visible in the headset

After successful signed phone-test #157, a remaining defect was identified in failure reporting: failed JNI create, module GL initialization or per-eye render returned to Home without any in-headset explanation. The native shelf/Home now shows a bounded module identifier and actual JNI runtime error; the message survives the failed module's normal shutdown and clears after a successful native launch. Errors are still logged and terminate the failed module: no stubs, bypasses, retries, new controls, camera/UI layout changes or different Back behavior. Existing signed build/device testing remains the acceptance gate.

## 2026-10-13 — Remove shell-generated GL scissor error at native-game launch

Focused correction in `VrShellRenderer.onDrawEye`: replace `glGetIntegerv(GL_SCISSOR_TEST)` with `glIsEnabled(GL_SCISSOR_TEST)`, the supported GLES2 enable-state query. The prior query could contaminate a native game's own `glGetError` on first eye and produce the observed immediate Home return. Existing original scissor state restoration and viewport/scissor rectangles remain the same; no headset/view, input, power HUD or geometry change. Native GL host boundary remains diagnostic defense. Signed APK build and Galaxy S9 runtime test pending.

## 2026-10-13 — Native games / classic launch path and blue headset-origin ray audit (device proof pending)

Audited 11 registered native modules (Test Chamber; Red Ledger; Breakwater; Lantern Desk; Forward Detachment; Pong, Snake, Chutes and Ladders, Checkers, Go, Chess) against Android.mk library names. Previous release verification explicitly checked only Test Chamber/Red Ledger and did not prove the nine other native libraries were packaged. Extended the **existing** signed phone-test verification with exact paths for the nine additional modules under both arm64-v8a and armeabi-v7a; no new build workflow. Native render startup remains fail-closed; to eliminate cross-renderer error contamination, JNI consumes/logs prior shell GL errors at the module boundary before invoking each module's render callback, without clearing errors originating from the guest; failed callbacks now report module/eye identity. Gaze fallback no longer enables the headset-origin pointer beam when controller pose is unavailable, while actual controller rays, shell gaze hit-testing and recovery remain intact. Galaxy S9 launch/draw/playability and headset visual check still required; source and APK tests alone cannot prove runtime success.

## 2026-10-13 — User DOS/Quick Menu corrections: pointer polarity, real fullscreen, explicit close, no implicit recovery popup

Four narrowly scoped behavior repairs: (1) reverse X and Y *guest mouse* scale only on the direct DOS headset-view mouse profile to correct reported inverted pointer navigation; preserve Cardboard head pose and other input profiles; (2) change DOS fullscreen from a slightly larger offset 3D quad inside a 73.8% inset eye to a full Cardboard eye-viewport image with exact DOS-aspect fitting, no side-console draw or invisible clickable option rail; (3) append an explicit **CLOSE MENU** to the existing universal Quick Menu without replacing EXIT PHONE or HOME; (4) eliminate automatic Quick Menu opening triggered by controller availability dropouts during hosted play. Five-second upward-gaze recovery continues to work only deliberately with no usable controller and requires looking down to re-arm. Kept power HUD, battery offsets, headset view matrices and controller calibration untouched. Regression source for mouse sign and full-eye sizing; Android build and S9 confirmation pending.

## 2026-10-13 — User-requested battery power bars shifted 10% lower (source, device proof pending)

Moved both phone and controller battery arcs together by exactly one-tenth of each full Cardboard eye's height (vertical clip-space offset -0.20). The shift is applied only to the existing small HUD quad's normalized display vertices, after its established 0.30 size scaling; no arc radius, label location within the artwork, percentage size, charge value, adaptive look-up reveal, headset/view matrices or controller transforms changed. Added an invariant test for the 10% shift and unchanged quad height. Validate physical lens visibility and comfort on the S9 before declaring device acceptance.

## 2026-10-13 — DOS Daydream directional navigation in dedicated mouse modes (source committed, handset pending)

Repaired missing Daydream touchpad keyboard/RetroPad navigation in `dos-head-mouse` and `dos-controller-mouse` while keeping their exclusive orientation-driven guest-mouse sources and Select click. The shared mapping emits four arrow keys with 0.35 axis thresholds plus DOSBox Pure RetroPad buttons 4–7; release neutralizes the inputs. The `dos-touchpad-mouse` profile remains pure swipe-to-mouse and must not emit simultaneous arrow events. Existing DOS standard, FPS, DOOM, HUD, headset orientation and controller pose transforms remain untouched. Added targeted source regression tests. The authoritative GitHub source is committed; Android build and Galaxy S9 device testing have not been demonstrated.

## 2026-10-13 — DOS UI focus releases already-held guest actions (source; Galaxy S9 pending)

Repaired DOS guest-focus handoff rather than adding a menu feature. Previously `isHostedInputSuppressed` rejected new controller events whenever a DOS options button was hovered or Quick Menu was open, but guest buttons pressed before that transition could remain held, and the new retained quick-tap pulses could leak into the resumed game. The renderer now invokes the existing activity's binding-release path through a narrow new `onDosGuestInputFocusLost` host callback when DOS first enters a shell UI target or opens Quick Menu, including gaze-recovery menu opening. Activity releases held and queued key/mouse/joystick input and resets the controller-relative pointer baseline. Regular DOS overlay pause reuses the same function; hovering a button does not pause emulation. No orientation, HUD, renderer geometry, or native ABI changes. Source commit/read-back can be verified; Android build, DOSBox runtime, S9 physical input and return-to-play acceptance remain outstanding.

## 2026-10-13 — DOS/native quick-tap and slow-pointer input reliability (source; hardware gate pending)

Traced a concrete dropped-input failure: a short Select press and release (including the `VrInputRouter.submitAction` pulse path) could be committed to `VirtualInputBus` entirely between DOS/native worker polls, leaving no press in either snapshot. The bus now latches an unseen rising edge for exactly the next capture on virtual keyboard, mouse and joystick buttons. A true hold remains continuously down; profile change/Quick Menu/recovery `releaseAll` clears pending pulses so no activation occurs after guest input resumes. Added focused tests for a quick tap, hold/release, and recovery clearing. DOS mouse now accumulates subpixel head/controller/touchpad motion and sends its actual integral motion to both the libretro relative mouse and the visual/absolute pointer instead of rounding tiny movements permanently to zero. No new menus, standalone validators, external software, headset quaternion correction or charge-HUD geometry were introduced. Source-only: build and S9 acceptance remain pending.

## 2026-10-13 — DOS relative/absolute mouse bridge and visible cursor repair (source; device gate pending)

Traced the previous cursor failure to two divergent libretro inputs: the guest received head/controller/touchpad movement on RETRO_DEVICE_MOUSE X/Y, but RETRO_DEVICE_POINTER X/Y remained at the VirtualInputBus default 0.5, 0.5 without physical absolute HID motion; the existing onscreen crosshair was separately glued to headset gaze even when mouse control came from a controller or touchpad. DosNativeRuntime now integrates the exact rounded, bounded relative motion it submits to the native bridge into normalized pointer coordinates (640x480 virtual pointer units) while giving explicit absolute HID X/Y priority. The same coordinate is sent to RETRO_DEVICE_POINTER, exposed read-only from the DOS session and drawn as the native video texture's clearly visible cursor overlay. Input selection, clicking, shell recovery and pause lifecycle are preserved. This does not claim access to an arbitrary DOS game's internally scaled mouse pointer, which may have separate sensitivity. No Cardboard headset transform, controller calibration or battery HUD geometry changed. Android compile, signed APK, and user-visible S9/DOS guest-pointer interaction remain outstanding.

## 2026-10-13 — Direct DOS mouse-source binding controls (source implemented; Galaxy S9 gate pending)

The DOS Quick Menu now exposes explicit HEADSET VIEW, CONTROLLER AIM, TOUCHPAD SWIPE, HEAD + TOUCHPAD, STANDARD PAD ARROWS and FPS HEAD LOOK binding choices; Advanced retains existing cycle/sensitivity/deadzone/reset. Each selection is persisted through existing DosModuleRepository and applied to the running hosted profile. A tracked Daydream quaternion is normalized into bounded relative guest-mouse yaw/pitch only in the controller-mouse profile; gaps, invalid poses, near-vertical aim and shell menus reset the baseline. The prior head-relative and pad-swipe mouse paths are reused with their own standalone profiles. No HUD, phone/headset orientation, shell controller visual calibration, or other native game input was altered. Source committed pending Android APK compile and physical DOS pointing/click test.

## 2026-10-13 — Actual classic-game Daydream gameplay input restoration (source; S9 proof pending)

Replaced the previous classic-games validation-only follow-up with a functional input repair. In the prior `native-standard` hosted mapping, Daydream TOUCHPAD_X/Y did not reach the native joystick, even though the classic C++ engine expects those axes for Pong paddle, Snake direction and Checkers/Go/Chess grid cursor; only external Android gamepad axes worked. Added `native-classics` as a small specialization that inherits every existing native key/button and gamepad mapping and additionally maps normalized touchpad X/Y into the native joystick axes with a centered deadzone; choosing a `classic-*` module now selects this profile through one shared classifier. Native worlds still use `native-standard` and shell-owned locomotion; neither headset axes nor HUD changes. Removed the one-off classic packaging validator script and its bootstrap invocation at the user's direction. Unit source for pad/Select activation included. These are production input changes committed to `main`; Android compilation, signed APK packaging and S9 controller testing have not yet been demonstrated.

## 2026-10-13 — Classic games committed-source and APK payload gate (source-verified; device gate pending)

Confirmed that six first-party games were already committed to `main`: Pong, Snake, Chutes and Ladders, Checkers, Go 9x9, Chess. Each has a distinct `classic-*` module id, `CLASSIC_GAME_KIND` C++ implementation selector, own NDK shared-library target, native host allowlist entry, and the in-headset Native Games / Native Apps shortcut routing. Added dependency-free `scripts/verify-classic-games-packaging.py`, invoked by existing `verify-build-bootstrap.sh` to prevent missing-source/host/NDK/menu registration from going unnoticed. The optional `--apk /path/to/ReverieVR.apk` checks all six actual `.so` payloads for both armeabi-v7a and arm64-v8a. Source audit of remote repository paths passed; validator Python syntax and mock complete/incomplete APK cases passed locally. This is not signed APK proof: build a real APK, run the payload audit, then test each title and recovery physically on Galaxy S9. No existing gameplay engine, hardware orientation, HUD, or controller binding was altered.

## 2026-10-13 — Ministry and Forward Detachment native prototype admission (source; Android/S9 pending)

The existing Lantern Desk and Forward Detachment deterministic kernels are now linked separately into two Development-only GLES2 native modules using a common compact graphics/interaction implementation. Ministry supports sector dispatch, 16x16 photo examination and graded reports; Forward supports route, observation, decision, return and debrief. A controller's native secondary rocker button (or gamepad axis pulse) cycles available choices, Select advances the kernel. Both native host registry and Java launch checks enforce Development logging. The shell owns recovery, stereo eye matrices and battery HUD. There is no new APK or ABI, no completed full desk/outdoor game and no claim of successful NDK/Gradle/device verification. The next gate is an actual signed APK compile and reference S9 test.

## 2026-10-12 — Native Apps and Native Games discovery consolidation (source; hardware gate pending)

The in-headset Native Apps catalogue now includes the actual packaged classic titles as well as native project modules admitted for the current session and existing shell-owned tools (OBJ viewer/import, media, music, keyboard text pad, controller laboratory). Native Games now directly lists the six classics and Development-admitted Red Ledger and Breakwater Battery previews. This reuses existing runtime and preserves the Development admission gate; it does not introduce empty buttons for the simulation-only Ministry of Intelligence or Forward Detachment. Source-only: Android/JVM validation, signed APK packaging, S9 in-headset launch/recovery, and the independent Native Apps return-home crash diagnosis are still pending.

## 2026-10-12 — Phone-test #146 AGP generated-assets variant API correction (source; rerun pending)

Rerun #146 rejected the previous attempt at Gradle configuration before any compilation: current Android Gradle Plugin refuses Provider instances in the legacy `android.sourceSets` API and explicitly directs generated folders through `SourceDirectories.addGeneratedSourceDirectory`. The replacement now declares a task-owned `@OutputDirectory DirectoryProperty` on `StageOwnerMusicTask`, preserves the existing SHA-256-validated task action, and registers the same task's output through `androidComponents.onVariants` using `variant.sources.assets.addGeneratedSourceDirectory`. This is AGP's documented generated-assets protocol; it retains all eleven music tracks, their hashes, lint and signing, and does not use `android.sourceset.disallowProvider=false` or bypass validation. The controlled build trigger advances to 122. Verification requires a successful new signed phone-test release and subsequent reference-device VR first-frame retest.

## 2026-10-12 — Phone-test #145 Gradle owner-music implicit-dependency repair (source; rerun pending)

GitHub Actions phone-test build #145 at `5af8a593c135dbdb2732f4d8dca99ce72d518048` passed the Android debug build and pure-JVM video shader regression but failed in `:app:lintVitalAnalyzePhoneTest` during persistent-signature release APK creation. Gradle reported an undeclared dependency on the generated owner-original music asset directory output by `:app:stageOwnerMusicAssets`. The authoritative `app/build.gradle` had attached a prematurely resolved `File` as the main asset source and maintained an incomplete hand-listed subset of consumers. Use the `stageOwnerMusicAssets` TaskProvider as `assets.srcDir` so Gradle's task-dependency metadata accompanies the generated source directory to AGP's asset mergers and lint tasks. Remove the redundant incomplete task-name dependency filter, retaining the existing release checks, SHA-256 music validation, and preReleaseBuild archive gate. The controlled authorized phone-test trigger advances to 121 for an actual signed build rerun; no completed release or Galaxy S9 first-frame verification is claimed here.

## 2026-10-12 — Galaxy S9 VR startup: GLSL reserved-word repair (source; handset retest pending)

User-supplied Samsung SM-G960U Android 10 phone-test v135001 diagnostic ZIP recorded three repeatable renderer-surface-create failures on 2026-10-12. All failed at VideoSurfaceRenderer fragment-shader compilation, GLSL ES line 23, because `packed` is a reserved identifier. Daydream controller pose packets and the model loader were operating prior to failure; this is not a Bluetooth or location-services failure. Renamed the GLSL fragment shader local to `packedUv` and its sample-transform use without changing the eye crop, external OES sampler, GL lifecycle, phone permissions, or shader execution path. Added a pure-JVM test that inspects the actual runtime shader string for the reserved token and verifies eye crop / SurfaceTexture transform wiring. Triggered the already authorized phone-test build on main. Build, signed APK release, and first-frame/device validation are not claimed until observed remotely and on the S9.

## 2026-10-12 — Breakwater position-driven low-poly animation (source)

The Beachhead native VR game now derives landing-craft bow-ramp lowering
(z=-3.7..-2.2), two-wedge wake movement and infantry marching gait
directly from authoritative entity positions. Marching stops at the
existing z=-1.34 firing line. The animation is computed by pure game-private
C++ helpers and shares the single world-scene mesh for both eyes; it
does not add animation clocks, textures, triangles, GL draw calls,
combat hitboxes, ABI changes or save fields. Nonfinite coordinates fail
closed. The exact updated recipe SHA was compiled/executed locally
with a C++17 -Werror standalone geometry fixture; new campaign-test
assertions were separately compiled/executed against compatible entity
type stubs. Complete native campaign/NDK/Android compilation,
Galaxy S9 device visual testing and thermal profiling are pending.

## 2026-10-12 — Breakwater differentiated landing-craft and infantry models (source)

The Beachhead native VR module now generates distinct original primitive
silhouettes for one-man rowboats, 3–6-person landing craft with ramp,
gunboats with wheelhouse/weapon, and individually moving infantry with
helmets and separated limbs. All use existing game coordinates and
hitboxes, share one batched stereo scene mesh, and add no textures,
model imports, native ABI or permissions. The C++17 standalone recipe
fixture passed with compatible public-type stubs; corresponding
assertions were added to the full-campaign native test source but not
executed through the Android NDK here. Reference S9 legibility, aiming
and thermal acceptance remain pending; no new APK generated.

## 2026-10-12 — Live now-playing selection after music advancement

The active Music Library song has a readable `[NOW]` text marker,
independent of color or reticle hue. When the local MediaPlayer
automatically completes and advances to the next track, a one-shot
UI-thread callback invalidates the VR music texture so the title and
selection change immediately on the next GL frame; no frame polling,
new network traffic, OCR or audio-content analysis. The callback is
attached after renderer construction and cleared on close. Input-driven
music buttons retain their existing refresh path. Android build and
Galaxy S9 device acceptance remain pending.

## 2026-10-12 — Music-only volume and mute controls (source)

The Native Music Library now exposes three working focusable controls:
MUSIC -, MUSIC + and MUTE/UNMUTE. They alter the actual local
`MediaPlayer.setVolume` gain, not Android's stream/phone volume; the
user's percent and independent mute status persist through
`ReveriePreferences` and are restored at next launch, including when
Breakwater uses its owner-original cue. Default 44% maps to the previous
0.22 absolute player gain, avoiding a startup loudness increase. Range
is safely bounded to 0..100%, with a peak local gain of 0.5 and 10%
increments; toggling mute preserves the previously chosen gain. The
status/button labels refresh from real persisted/player state after
UI-thread input dispatch. Controls use existing shell raycast/keyboard
navigation and need no extra audio stream or permissions.

Pure-JVM `MusicVolumePolicyTest` covers unchanged default, clamping,
mute and stepping; in-headset controller activation, Android Gradle
build and Galaxy S9 playback/loudness acceptance are still pending.

## 2026-10-12 — Accessible high-contrast shell buttons (source)

Quick Settings exposes a real, persistent `CONTRAST: ON/OFF` toggle
next to the shortened `EXT HID: ON/OFF` cell; neither the earlier menu
navigation nor dedicated Back/Close buttons were displaced. The mode
changes the shared shell button draw path to monochrome near-black/
near-white, draws persistent boundaries, inverts focused button
background/text and uses dark focus rails over light focus, while
keeping a circle-slash disabled symbol. It applies to Home navigation cards (through their separate draw
routine), Quick Settings, Music Library, native module shelves and other
standard shell buttons, without altering Android Media,
VR optics, native gameplay textures or remote screen content. A
pure-Java `ShellContrastPalette` exposes luminance contrast checks;
JUnit regression source demands >=7:1 text/background contrast
for enabled/focused/disabled and grayscale-only palette entries.

No claim of fully adapted protan/deutan/tritan color vision handling,
full nonstandard surface coverage, Galaxy S9 lens-measured contrast or
real low-vision participant acceptance. Android APK build/device
checks remain outstanding.

## 2026-10-12 — Breakwater shoreline infantry threat warning (source)

Beachhead/Breakwater now predicts a shore infantry salvo from actual
living, disembarked soldier travel and the shared four-second firing-line
timer. The new small SHORE Ns indicator appears only within six seconds
of a plausible attack, complements AIR RAID/GUNBOAT, and clears with its
shooters; boats still offshore never fabricate a land-soldier threat.
The per-eye renderer only observes campaign status, and the shop save
format, controller routes and texture budget are unchanged. A local
C++17 -Werror standalone fixture passed seven representative
arrival/timer/cancellation assertions; integrated full native build,
APK packaging and reference S9 headset validation remain open.

## 2026-10-12 — Owner original music authorized for *every* APK variant

The owner explicitly reaffirmed that all eleven provided MP3 files are
their own original musical products, that they have commercial rights to
them, and directed inclusion in the normal application. The source manifest,
REUSE scope, new `LicenseRef-Owner-Original-Music` and
`docs/licensing/OWNER_ORIGINAL_MUSIC.md` reflect that declaration;
these are owner-commercial permissions, not MIT, CC BY, CC0, public domain
or unrestricted third-party music sublicensing.

ReverieVR's `stageOwnerMusicAssets` now verifies master file byte lengths
and SHA-256, copies all eleven into Gradle generated assets, checks staged
SHA-256, and generates `music/owner_library.tsv` with owner-original license
IDs. `main` assets source set and all variant asset-merger tasks depend
on that stage: debug, phoneTest and release now all include the full
twelve-track offline library alongside Starry Cereal. The asset registry
marks all eleven recordings required, commercially authorized, and
hash-pinned, while the licensing audit validates their source masters,
authorization, registry mapping and exact bytes. No duplicate Git binary
blobs, remote music service, desktop companion, OCR or screenshot
interpretation. APK size increases intentionally by 66,325,607 audio bytes.

Home alternates Starry Cereal/Sky Lobby Drift/Glass Lobby Drift across
sessions and between songs; Breakwater Battery plays Mars des Souvenirs
when active. The assigned Ministry and bar cue IDs still await their
native game modules, while the remaining six records are playable in
the general offline library.

**Acceptance limitation:** authoring, commits and remote checks do not
prove Gradle execution, an APK build or Galaxy S9 audio behavior.
Validate assembled source and playback before claiming device acceptance.

## 2026-10-12 — Music preview APK allowlist and truthful library label

The optional generated `assets/music/source_preview.tsv` file is now
explicitly registered as first-party test catalog metadata, while all
eleven actual MP3 contents retain exact SHA-256 and `review-required`
licensing in the APK audit. This repairs a missing allowlist entry for
the phoneTest asset verifier. The Music Library no longer incorrectly
describes preview-pending recordings as universally "licensed local
audio". No download, stream or remote service was added.

## 2026-10-12 — Live test-build music library, three lobby cues and Breakwater soundtrack

Verified that all 11 source MP3s (66,325,607 bytes total) are present in
GitHub's authoritative `music/source/audio/`. The Java native music player
now safely combines licensed `music/catalog.tsv` with a *separate*
development-only `music/source_preview.tsv` catalog. Each runtime audio
source is locally packaged and validated by ID/path shape; the Gradle
`stageSourceMusicPreview` step verifies every file's source length,
SHA-256 and copied output checksum. Only `debug` and `phoneTest`
include the eleven recordings (~66.3 MB added). The unrestricted
`release` variant continues to omit them. The APK asset registry
contains review-required SHA entries and the existing strict licensed
Suno-only catalog remains unchanged.

ReverieVR Home rotates among Starry Cereal, Sky Lobby Drift and
Glass Lobby Drift on successive application sessions and completed
tracks, not arbitrary song content. Manual Music Library selection
exposes the 12-track test collection. Starting the existing
`breakwater-battery` native game suppresses lobby audio and plays
Mars des Souvenirs as a looping local cue; exit restores Home lobby
music. The Last Bugle Call/Ministry of Intelligence and The Last Note
Hangs Unresolved/bar project identifiers remain **future contracts**
until actual modules with those IDs exist; no unrelated game receives
their music automatically. Hosted DOS/video/native modules without
assigned cues retain the existing music suppression.

Title-specific Suno/commercial redistributability is not independently
documented. Those recordings are explicitly review-pending in test
builds and excluded from unrestricted `release`; this is no license
grant. Java regression tests for catalog isolation, duplicate ID
rejection, lobby rotation and project cue routing are committed.
Gradle/APK build and Galaxy S9 stereo/thermal/real playback validation
remain to be established; this state does not claim an APK was built.

## 2026-10-12 — Phone panel physical size and viewing-distance controls (source)

The world-space Phone Screen placeholder has a second four-key low-poly
rail for bounded scale (0.75–1.15x) and depth (-0.30..+0.45m). The existing
six control identities, X/Close, Back recovery and shell menu are retained.
One pose routine is now authoritative for the stereo render and controller
inverse-ray intersection; the raycast reuses frame scratch buffers instead
of allocating six new arrays per sample. The existing orientation reset
now restores size and depth too. Six JVM geometry/behavior tests were run
locally using Java 21 with a minimal compatible JUnit annotation/assert
harness (not Gradle); Android/NDK compilation and Galaxy S9 headset
validation remain pending. The panel still intentionally presents a dark
surface, without MediaProjection, Android pixels or OS gesture dispatch.
No APK built.

## 2026-10-12 — Headset-only gaze continuity after render stalls (source)

The existing allocation-free five-second shell and upward-look recovery
dwell now discards a partial countdown if more than 500 ms elapses
between actual live gaze samples, or the sampling clock regresses.
This prevents an application pause, render stall or resume from
converting stale focus into an immediate Select/Quick Menu action.
One-shot/no-repeat, controller-return reset and 5-second timing remain.
Four deterministic JVM cases passed a local Java 21 compile/runner;
Android build and Galaxy S9/Daydream pause/resume, low-frame-rate and
controller-free recovery evidence remain open. No APK released.

## 2026-10-11 — Three Classic board games (source; device proof pending)

The existing native Classic Games build and host allowlist now admit
Checkers, 9x9 Go, and Chess. These use `classic_board_games.h` for
bounded rule-state, CPU heuristics and terminal adjudication; the existing
GLES2 renderer draws geometric boards and labeled pieces, with ray/cursor
selection and a Go pass command. NDK builds use three `CLASSIC_GAME_KIND`
variants, discoverable automatically in the existing `classic-*` shelf.
Tests, actual Android/Gradle native packaging and S9 thermal/stereo
controller acceptance remain to verify; no shipped APK claim.

## 2026-10-11 — Scoped open-source licensing and per-release rights verification

The owner selected **MIT** for independently authored ReverieVR software,
**CC BY 4.0** for verified owner-created art, and optional **CC0** dedications.
Third-party code/assets keep upstream licenses. `REUSE.toml` and adjacent
sidecar records distinguish known from unresolved rights (notably music,
logos and Daydream visual). Android resource binaries are annotated outside
their `res/` folders to avoid breaking aapt2. The asset registry and
`scripts/verify-distribution-license.py` validate the actual packed
assets/native libraries for each public phone-test APK, with a separately
stricter `--channel open` for unconditional commercial distribution.
The existing phone-test workflow packages notices, exact release manifest
and GPL DOSBox Pure pinned source. The build bootstrap runs offline tests.
No claim of a complete LGPL/GPL linked-work legal clearance, upstream model
license resolution, or Android headset device acceptance is made yet.

## 2026-10-11 — Hosted gamepad B no longer exits active games

The platform now routes physical Android `BUTTON_B` to guest output when
native/DOS content is actively receiving input, instead of interpreting
it as an immediate shell Back and exiting/opening the overlay. Standard
native hosted profile exposes it as virtual joystick button 3 and shifts
X/Y/shoulder/trigger/stick/select guest indices to 4–12. Outside active
guest mode B remains shell Back. Android system Back and Start/Mode
retain shell recovery; in-VR Quick Menu still routes home/recovery.
JVM profile tests are updated; Gradle, multiple device driver reports
and Galaxy S9 input/recovery evidence remain outstanding.

## 2026-10-11 — Shared raw Android gamepad input vocabulary (source)

Introduced physical Android HID A/B/X/Y, L/R shoulders/triggers,
thumbsticks, Select/Start/Mode and four D-pad key identities into
`BindingInput`; physical edges are sent separately from shell action
translation. L/R analog trigger axes now enter the existing analog profile
pipeline using Android trigger/brake and trigger/gas alternatives.
The existing `native-standard` profile additionally maps nine buttons
(X/Y/L1/R1/L2/R2/left-right stick click/Select) to joystick 3-11 and
left-stick axes to the host joystick, while preserving Daydream
Select/rocker buttons 0-2. Face B still triggers shell Back and is *not*
assigned to the default native profile; Menu/Mode remain reserved.
Generic, Xbox 360 and DualShock may expose these common Android HID
symbols, but actual hardware label/report verification and a full
physical shell/guest remap UI remain RV-0227 device-gated work.
Regression source for button identity, standard native profile and
hosted codec is added. No APK or handset device claim.

## 2026-10-11 — Per-device primary Android controller selection (source)

Extends the existing VR controller-source chooser without inventing another
launcher screen. Choosing Android USB/BT selects an exact non-ambiguous Android
InputDevice descriptor; tapping that row again cycles sorted, connected
gamepads. Device descriptors, not transient device IDs, persist. All gamepad
KeyEvent/MotionEvent ingress is gated to that exact device; AUTO admits at most
one ordinary Android gamepad rather than all attached gamepads. Missing or
duplicate descriptors are not trusted to represent a unique physical device.
Switches release existing virtual/hosted key state and reset input edges.
The existing emergency disconnection rule reverts to Auto if the explicitly
selected Android controller disappears; it does not silently promote another
device while the explicit selection remains connected. Controller status now
reports selected device name and count. No new Android permissions.
Pure JVM selector regression source: selection, cycling, duplicates,
disconnect, missing devices. Android build/S9 multi-controller, reconnect,
scrcpy and routing validation remain open.

## 2026-10-11 — Shell UI hover stabilization with live-ray Select

Source-only shell interaction refinement: Home panels, desk keys and modal
menus use allocation-free target-level hysteresis (45ms cross-target
confirmation and 65ms loss grace) without filtering the pointer ray or
changing gameplay input. Initial acquisition is immediate. Select consumes
the live target from the same render-frame raycast rather than a retained
highlight, preventing stale or occluded key/menu activation. Recentered
interaction namespaces (menu, mode, keyboard navigation, input kind and pointer
preference) reset the latch. Focus remains purely visual; physical controller
feedback uses live contact, not the stale highlight. Deterministic JVM tests
cover jitter, loss, selection, clock resets and mode clearing. Local pure-Java
logic check passed; full Android/Gradle build and reference Galaxy S9/Daydream
pointer latency, stereo readability and accidental-hit checks remain pending.

## 2026-10-11 — Home ray pointer physical mesh intersections

Source implementation adds a bounded allocation-free ray/triangle hit against
static cached geometry of all three Home environments, with a normal-aligned
impact marker, nearest hit selection and depth-respecting beam. Shell UI/keyboard
targets keep precedence within 35mm to protect clickable keycaps. This is a
PS1-style no-texture primitive impact indicator; no runtime physics or native
game mesh collision is claimed. Exact triangle intersection, normals, winding,
off-surface misses and geometric decal orientation have JVM regressions.
Galaxy S9 headset runtime, stereo and frame-timing measurements pending.

## 2026-10-11 — PS1 primitive art direction and three-home recipe layer

Source-level art pass: the existing Birch Living Room, Pastoral Glade and Dune Beach detail generator now produces additional angular wall motifs, fixed-seed fallen logs and faceted tide rocks. Vertex positions are quantized once at build time to 1/128 m; new material tints use deterministic RGB555 values. No image/texture assets, external generation dependency, additional shaders, per-eye CPU geometry generation or destructive scene replacement. Tests enforce bounded triangles, deterministic explicit seeds, repeated-vertex-grid quantization and the unobstructed home interaction corridor. This implements the explicit PlayStation-era/.kkrieger-inspired art philosophy as compact recipes, **not** emulation or a claim of proven quicker boot or improved battery. Existing GitHub APK validation and Galaxy S9 stereo/performance acceptance are separate gates.

## 2026-10-11 — Phone Screen explicit Close and no pointless capture consent

Sixth geometrically distinct X button added to the adjustable in-home phone
panel. Renderer and raycast share the same six-key layout, including
close; X and Back use a single dismissal path that restores Home,
clears retained hover and releases capture resources without exiting VR.
The Android consent request was removed from opening the dark prototype
housing: MediaProjection currently acquires/discards frames without
rendering them in the headset, so that permission was premature. The
consented-capture lifecycle source remains dormant until real local
presentation is implemented. Privacy rule in README and phone-panel
spec prohibits any OCR/visual scanning/image analysis or export of screen
pixels; retains accurate disclosures for locally saved state, manual
diagnostics, paired controller exchange and optional GitHub release checks.
Pure JVM geometry regression source is included. APK/Gradle and Galaxy S9
ray/close/Back testing remain pending.

## 2026-10-12 — 11-track source music handoff catalog (binary import pending)

The user's eleven actual MP3 uploads were independently inventoried
(66,325,607 bytes total) with SHA-256 and recording durations. The
source-only manifest under `music/source/manifest.json` identifies the
bar cue, Beachhead-style cue, Ministry of Intelligence cue, two alternate
lobby cues, and six general-library sources. The original binary uploads
are **not in remote GitHub**: the connected GitHub blob API cannot read a
local binary path, and this runtime has no authenticated filesystem Git
transport. A locally hash-verified ZIP containing all original MP3s and
a Windows bounded-retry Git importer was provided for authenticated
transfer. Do not claim those binaries are hosted until remote read-back.
No APK/catalog entries were created for absent source audio; Suno
entitlement and public music redistribution rights remain per-track
review gates.

# ReverieVR Execution State

**Role:** durable operational state  
**Updated:** 2026-10-11 (America/Anchorage)

## 2026-10-11 — Stage A launcher and in-VR import correction

Source change (APK and Galaxy S9 validation pending): Stage B QoL, media, video projection and DOS import panels removed from the pre-VR setup screen. Retained phone/controller status and tests, VR entry, updater, diagnostics and recovery. VR-to-Android document-picker handoff and import storage are preserved. In VR, the DOS library has an always-available Import control alongside paging and Home, even when modules already exist. Static source verification only; not an APK or device acceptance claim.

## 2026-10-11 — RV-0502 media scene / stereo video source slice

The media pipeline now distinguishes ordinary 2D cinema and 360-degree content from
explicitly configured packed stereoscopic side-by-side and top/bottom layouts
(including equirectangular 360-degree variants). Both Cardboard eyes receive the
proper packed UV region before the external SurfaceTexture transform, with
aspect correction per eye for flat cinema. Video occupies the full physical eye
viewport rather than the inset menu rectangle and does not render the home
environment behind it. The in-VR Media panel cycles modes; selecting a different
video resets to a conservative filename hint or 2D fallback. Unrecognized 3D
metadata is **not** detected automatically. VR180 fisheye decoding is not
implemented. Regression tests cover packing, defaults, aspect and suggestions.
Status: source-integrated, APK / JVM execution and Galaxy S9 stereo/thermal
acceptance pending; no device validation claim.

## 2026-10-11 — RV-0501 180-degree equirectangular scene support

Following verified stereo/360 APK build, this source slice adds a distinct
front-facing half-sphere for 180-degree equirectangular media. It uses the same
per-eye packed texture-region mapping and Cardboard stereo path as 360-degree
media, but does not wrap images behind the viewer. Both sphere meshes are
allocated once per renderer instance. VR users can explicitly choose mono,
side-by-side or top/bottom 180-degree equirect modes through the Media format
selector. Only filenames explicitly marked as 180-degree equirectangular
receive that default; generic VR180 filenames remain conservative because the
camera-native specification requires calibrated per-eye projection mesh
metadata and may include per-frame motion orientation.
Physical headset stereo/projection/thermal and Android build acceptance pending
for this new slice; do not claim calibrated VR180 fisheye support.

## 2026-10-11 — RV-0503 bounded MP4 spatial metadata identification

Implemented source-only MP4 Spatial Media v2 inspection behind the existing
Stage B -> Android document-picker handoff. A dedicated single-thread executor
reads at most 4 MiB from the selected SAF URI. The pure-JVM parser follows ISO
BMFF sample-entry box structure rather than searching bytes in video payloads,
recognizes st3d mono/top-bottom/left-right and sv3d/proj/equi with zero crop
and pose. Supported metadata overrides filename suggestions only while both
the chosen URI and initial projection remain unchanged. Files without accessible
metadata retain the filename hint or 2D default. Unsupported layouts emit an
honest warning; calibrated mesh, cubemap, custom/reversed eyes, cropped/rotated
projections and external containers do not get falsely auto-rendered. A bounded
4-MiB scan may not reach moov boxes at the end of large MP4 files; this is
reported as unknown rather than guessing.
New synthetic JVM tests cover supported stereo/spherical variants, malicious
mdat lookalikes, conflicting tracks, unsupported projection and truncation.
No APK, real media, or Galaxy S9 acceptance claim for this new slice.

## 2026-10-11 — RV-0505 fixed-eye prerecorded headset output

Headset-output captures can contain baked stereo eye frames with no live
scene, camera pose or genuine VR navigation. The Media panel now offers mono,
SBS and top/bottom fixed-eye modes separate from regular stereo cinema and
180/360 projection. Each decoded video view maps directly to one Cardboard eye
in clip space, using aspect-fit and no additional head-pose reprojection.
Explicit headset-capture naming can suggest these modes; generic SBS/TB still
defaults to cinema, and spatial video keeps its own geometry.
The optical headset compositor is still active. Lens-prewarped recordings
may need calibrated inverse optical warp before they can look undistorted:
that correction is not implemented or claimed. The VR panel tells the user.
Existing MediaPlayer, SAF permissions and OES texture lifecycle are unchanged.
Source and JVM-test coverage have been added; cloud APK and reference Galaxy
S9 + Daydream headset playback remain acceptance gates.

## 2026-10-11 — RV-0505 adjustable optical prewarp recovery

Extended the distinct fixed-eye headset-recording path with in-VR signed
radial image remapping. Lens -5% / +5% controls appear only for selected
headset recordings and adjust a radial coefficient from -0.30 to +0.30
in steps of 0.05. Off is the exact existing passthrough path. The fragment
shader samples the encoded local eye coordinate after a center-preserving
radial transform, then maps into the selected SBS/TB eye crop before the
Android SurfaceTexture producer transform. If the transformed coordinate
falls outside that eye region, the fragment draws opaque black instead of
leaking pixels from the adjacent eye. It does not alter 2D cinema, stereoscopic
cinema, or genuine equirectangular 180/360 media.
Correction is stored with the selected file, cleared for different media,
and restored on playback/Resume. Recenter and Home are retained in the expanded
headset-only in-VR menu. Regression tests check the no-op case, signed bounds,
center invariance, correction limit clamping and presets.
This is *not* calibrated optical inversion: real inverse models can require
source headset radial/tangential lens coefficients and off-center per-eye
optical origins. Nor does it restore six-degree head tracking to a recording.
Status: source slice; GitHub Android build, actual GLES2 shader link, Galaxy S9
headset viewing and visual distortion proof remain necessary.

## 2026-10-11 — RV-0503 tail-MP4 spatial metadata recognition

The Android Media picker now opens the selected document's SAF file descriptor
for bounded random-access inspection before falling back to an InputStream
when seek/position operations are unavailable (as with some content-provider
pipes). SpatialMp4Metadata retains its pure JVM container decoder, adds a
SeekableByteChannel path, checks the ftyp/styp header, walks at most 1024
top-level boxes using complete 32- or 64-bit box lengths and never reads
the media payload of mdat. Each moov is read and parsed only when its
complete size fits the 4 MiB limit. Truncated, oversized and malformed
containers stay UNKNOWN; multiple incompatible spatial tracks are rejected.
A metadata selection is applied only if the selected URI and initial format
still match, so an asynchronous result cannot overwrite the user's later
file choice or manually selected format. No broad file access permission
or Android background work was added.
Tests use sparse large video-data sections followed by trailing metadata,
64-bit box sizes, misleading moov payload inside mdat, conflicting tracks,
oversized moov, zero-length mdat and incomplete headers.
Status is source-implemented, pending signed APK/Android tests and reference
Galaxy S9 playback/thermal verification. Unsupported camera-specific lens
and mesh calibrations remain separate work.

## 2026-10-11 — RV-0501 calibrated camera VR180 projection

Added the actual calibrated-mesh playback path, not a generic fisheye
substitute. `Vr180MeshProjection` parses Google Spherical Video v2 mshp
boxes with CRC32 and raw or raw-deflate encoding; a bounded bit reader
expands shared coordinate tables, zigzag-delta encoded component and vertex
indices, and triangle/strip/fan topology. It validates mesh/packing counts,
range, finite UV and geometry values, rejects unsupported textures, and
caps decompression, vertices and rendered triangle counts to protect the
Galaxy S9. `SpatialMp4Metadata` exposes a CALIBRATED_MESH_VR180 projection
only when the embedded geometry validates, including stereo-custom dual
meshes. Arbitrary bare-VR180/SBS filenames cannot enable the mode.
`VideoSurfaceRenderer` now publishes immutable stereo mesh buffers and
draws the selected eye with its own geometry and correct packed UV crop;
until a mesh arrives, the immersive frame is cleared black. The app reopens
the selected document using SAF asynchronously on VR playback, with
monotonic cancellation against video stop/file changes and activity
destruction. No additional external libraries or storage permissions.
Synthetic JVM tests cover raw/deflate mesh, both-eye stereo-custom, parser
integration, CRC damage, mismatched eye counts and corrupt payloads.
Remaining acceptance: signed Android/JVM test pass; real recorded VR180
camera sample; per-eye alignment, stereo depth and performance on Galaxy
S9/Daydream. `camm` camera-motion metadata and nonzero projection pose are
not implemented; do not advertise stabilization of handheld camera motion.

## Authority

- Repository: `mrcalzon02/ReverieVR`
- Branch: `main`
- Current implementation baseline immediately below this documentation update: `c70026f8e5bd307bd5f53d46c862a87b629bff20`.
- Remote readback confirmed the Stage A application, verified Gradle wrapper, direct Daydream BLE backend, pinned Cardboard submodule, Stage B VR activity/renderer, first local-media vertical slice, unit-test sources, and build instructions are present on `main`.
- Cardboard submodule is pinned to `5969239e7c87f4cd64c8ec170ce1e7f4eb559e37` (v1.35.0).

## Last completed governance target

**RV-0001 — Governance/bootstrap adoption**

Acceptance: **static accepted**.

## Current implementation state

### Latest scoped handoff — RV-0209 original Steam Controller assets

Baseline before this change: `133042ffb4517e4bb9a538f13d9cafc328989b6d`, clean
`main`, fresh remote readback. Earlier component/build evidence below is historical
and does not describe this handoff's validation environment.

At the user's direction, prepared/uploaded a simplified-model package to Google
Drive for the online instance to implement programming/hookup. The inspected Valve
March 2016 ZIP and original CAD are preserved separately outside Git. Exact
CC BY-NC-SA 4.0 license and SHA256 are recorded in
`docs/records/STEAM-CONTROLLER-MESH-PROVENANCE.md`. No derived geometry is committed.
The converter now uses named QEM parts, explicit surface-repair reports, GLB/OBJ
assembly, precomputed GLES2 buffer data, previews and independent readback.

Verified: 22 parts, 29,734,379 source triangles -> 11,882 output triangles;
856,384-byte runtime buffer; five regression tests and real-asset/hash/GLB/normal
readback pass. Previewed assembly and all parts. Seven parts retain reported
topology flags; this is rendering approximation, not watertight CAD. Drive ZIP
readback confirmed name and 1,693,956-byte size. Handoff ZIP SHA256:
`6a847a6bd820a66fd0170643b7993a20e6079859d9f2bb6b58325f3c26ba1537`.

Remaining: renderer/controller hookup by the online instance, exact observed S9
Android event mappings, Android build and reference-device/performance validation.
The local Android attempt failed at configuration because no SDK was configured.
The native-documentation checker failed on unchanged baseline Test Chamber code:
it requires direct `ReverieNativeCompileShader` consumption although the module
uses `ReverieNativeBuildProgram`, which calls the shared compiler. No native SDK
source or validator was altered; that existing guard issue remains. RV-0209 is
**draft**. No APK, release, device support or thermal acceptance is claimed.
No branch, PR or GitHub Actions was used.

### RV-0002 — framework/runtime/controller stack

State: **draft**

Selected implementation baseline:

- Google Cardboard SDK v1.35.0 for head tracking, stereo projection, viewer geometry, and lens distortion;
- OpenGL ES rendering path;
- Cardboard Vulkan and Unity-plugin native paths disabled;
- direct Android BLE Daydream-controller provider;
- classic-Bluetooth RFCOMM receiver for a paired Android phone using the historical Daydream controller-emulator framing;
- Android gamepad/joystick detection and key/axis routing;
- normalized `VrInputAction` routing kept independent of Cardboard and transport-specific packet formats.

Remaining gate: successful build plus Galaxy S9/headset/controller validation.

### RV-0003 — Android APK skeleton

State: **draft**

Implemented:

- Android application project;
- application ID `io.github.mrcalzon02.reverievr`;
- minimum SDK 26;
- compile/target SDK 36;
- Java 17 app source;
- pinned Cardboard Git submodule;
- JUnit source tests for Daydream packet parsing and updater version comparison;
- documented Gradle/NDK/CMake build path.

Cardboard introduces its required AndroidX/native build dependencies; Stage A itself remains ordinary lightweight Java/XML Android UI.

Remaining gate:

- initialize submodule in a local checkout;
- run the committed wrapper/bootstrap verifier;
- run unit tests and `:app:assembleDebug`;
- inspect resulting APK;
- install and launch on the Galaxy S9.

### RV-0090 — 2D touchscreen boot/setup surface

State: **draft**

Implemented:

- conventional portrait Stage A launcher;
- device/model/API reporting;
- real phone battery display;
- persistent QoL/settings controls;
- controller Pair / Sync and live Test Controller flow;
- paired-phone controller chooser for historical Daydream-compatible controller emulator apps;
- live Android gamepad/joystick readiness detection;
- visible installed version/build identity;
- update status/check controls only when the build has a persistent signing identity;
- local reset/recovery;
- centralized pressed/disabled/failure button states with ACK/NACK audio feedback across Stage A, Stage B shell selection/rejection, and the controller companion;
- Enter VR gated on a ready controller.

Remaining gate: build and reference-device interaction test.

### RV-0091 / RV-0102 — controller sources and normalized input

State: **draft**

Implemented:

**Physical Daydream controller**

- Android-version-correct Bluetooth permissions;
- BLE discovery handles the Android 8–11 Location-services gate explicitly instead of silently returning zero results;
- already-bonded Daydream controllers are attempted before a new scan;
- low-latency foreground BLE scan is intentionally broad, then Daydream FE55/name matching occurs inside ReverieVR so Android cannot discard a controller merely because FE55 or the exact local name was absent from the initial advertisement;
- Development logging records aggregate advertisement/named/FE55 counts and detailed information only for a matched Daydream candidate, avoiding arbitrary nearby-device address/name capture;
- Android bonding flow;
- GATT connection/service discovery;
- pose notification subscription;
- READY gating on receipt of the first valid pose packet rather than descriptor-write success alone;
- bounded error if the pose notification channel subscribes but produces no usable packets;
- 20-byte pose/touch/button packet decode;
- controller battery percentage and voltage reads;
- controller recenter command;
- application-scoped controller lifetime;
- live Stage A controller diagnostics;
- independent protocol provenance record.

**Paired Android phone controller**

- user-selectable paired Bluetooth-device chooser in Stage A;
- classic Bluetooth RFCOMM connection to UUID `ab001ac1-d740-4abb-a8e6-1cb5a49628fa`;
- independent 4-byte big-endian length framing;
- independently authored minimal protobuf-wire decoder for motion/touch, gyroscope, accelerometer, orientation, and key events;
- historical click/App/Home/volume key-code compatibility;
- controller-phone battery deliberately reported unavailable because the transport does not expose trustworthy battery telemetry;
- pure-Java protocol parser fixtures for orientation, touch motion, click-key events, and malformed length-delimited input.

**Generic Android controller**

- attached Android `SOURCE_GAMEPAD` / `SOURCE_JOYSTICK` devices can satisfy the Stage-B input-readiness gate;
- device attach/remove/change notifications refresh Stage A and Stage B live;
- A/center/Enter -> Select;
- B/Back -> Back;
- Start/Mode -> Recenter;
- D-pad/hat/stick navigation emits normalized directional actions;
- L1/R1 and left/right navigation can drive player seek;
- generic gamepad battery remains unavailable unless a future Android/device-specific source exposes it honestly.

**Action layer**

- `VrInputRouter` is now the controller binding boundary;
- physical Daydream, phone emulator, generic gamepad, Cardboard trigger, and Cardboard system/back controls terminate in named `VrInputAction` values;
- volume actions adjust the Android music stream;
- raw Daydream button interpretation is no longer embedded directly in `VrActivity`.

Current verification: phone-test #21 completed successfully against `f1ccc93963bbcf18b81169626f45877a4c33f3e9` and published prerelease `phone-test-21-1`. This package includes broad foreground discovery, privacy-bounded scan diagnostics, explicit physical Daydream/Home pairing guidance, and manifest capture of Bluetooth/Location/permission prerequisites. GitHub Issue #2 tracks the remaining physical Galaxy S9 validation; FE55 discovery and live GATT/pose streaming still require handset evidence before acceptance.

Remaining gates:

- physical Galaxy S9 + Daydream-controller discovery, FE55 service discovery, pose subscription and live packet-stream validation;
- capture Development diagnostics and link the resulting RV-0208 diagnostic issue/receipt to Issue #2 if the physical path still fails;
- actual second-phone emulator RFCOMM validation;
- generic Bluetooth/USB gamepad validation;
- axis/polarity/comfort verification;
- user-remapping UI remains RV-0202.

### RV-0094 — ReverieVR Controller companion APK

State: **draft**

Implemented:

- independent Android application module `controller-app`;
- application ID `io.github.mrcalzon02.reverievr.controller`;
- portrait spare-phone controller surface with large touchpad plus Select, App/Back, and Home/Recenter controls;
- physical controller-phone volume keys forwarded as controller volume actions while connected;
- rotation-vector orientation with `TYPE_GAME_ROTATION_VECTOR` fallback;
- gyroscope and accelerometer transport when available;
- explicit refusal to start 3DoF control when no usable rotation-vector sensor exists;
- classic Bluetooth RFCOMM server using the historical controller-emulator UUID;
- bonded-client verification before accepting the headset session;
- 4-byte big-endian framing and independently authored protobuf-wire event writer;
- separate discrete-control queue plus coalesced orientation/gyro/accelerometer lanes;
- keep-screen-on behavior during companion use;
- backward-compatible ReverieVR status event exposing the real controller-phone battery percentage to the headset HUD;
- pure-Java protocol writer tests;
- headset updater asset filtering that rejects controller APKs.

Remaining gates:

- build `:controller-app:assembleDebug`;
- install on a second Android phone;
- pair both phones through Android Bluetooth;
- verify headset selection, RFCOMM connection, orientation/touch/button mapping, reconnect behavior, battery HUD, and long-session stability;
- validate that historical third-party/Google-compatible emulator traffic still works with the battery extension absent;
- measure sensor/update cadence and latency before claiming controller-comfort acceptance.

The companion is optional. Physical Daydream BLE and Android gamepad recovery paths remain independent.



### RV-0092 / RV-0100 — Stage A -> Stage B and VR shell

State: **draft**

Implemented:

- Enter VR enabled after either a dedicated controller provider is ready or an Android gamepad/joystick is attached;
- application-scoped controller connection survives Activity transition by design;
- landscape immersive non-resizable `VrActivity`;
- explicit GLES2, accelerometer and gyroscope hardware requirements;
- Cardboard stereoscopic/head-tracked rendering;
- fixed seated-origin home shell with three real lightweight GLES environment presets: White Cube Room + gray dais, Pastoral Forest Glade, and Windswept Dune Beach;
- persistent home-environment preference with live in-headset switching;
- world-space menu panel reorganized as the VR launcher hierarchy: Media, DOS Games, Native Apps, Environment, Setup / Comfort, and Exit to Phone;
- Media child interface for floating-screen playback, mono-360 projection switching, recenter, and Android document-picker handoff;
- DOS child library with module paging, direct module launch, Android import-picker handoff, and return-to-library behavior after a DOS session;
- flat media and DOS framebuffers composite onto the existing floating-screen geometry while the selected home environment remains visible around them; mono 360° media still owns the full sphere;
- head-gaze target selection;
- selectable VR pointer policy: Auto, Gaze, or Controller;
- physical Daydream controller quaternion drives a true 3DoF menu pointer ray;
- generic Android gamepads without inertial sensors drive a virtual pointer orientation through the right stick, with navigation/D-pad nudge fallback;
- Auto pointer mode prefers fresh tracked controller pose, then virtual gamepad aim, then gaze;
- active pointer source is visible in the Home status card as Tracked controller, Virtual gamepad, Gaze, or Controller unavailable;
- the selected low-poly Daydream controller OBJ is packaged as the normal in-world controller visual with separate touchpad/Home/App/volume material groups; the old procedural shape is retained only as an asset-load fallback;
- controller visual and pointer ray now share the same moving shell anchor and the ray begins at the model's front emitter rather than its center;
- handset TYPE_LINEAR_ACCELERATION feeds a damped, spring-returned, hard-bounded pseudo-positional headset offset, producing small inertial parallax while remaining explicitly distinct from true 6DoF tracking;
- Home UI is split into three independently positioned world-space planes: left navigation, center status/content, and right quick options;
- Home side planes are pushed farther left/right and tilted so their outer edges come toward the viewer, giving the launcher a shallow wrap-around arrangement;
- the shell panel envelope and screen-space power HUD are contracted to 90% of the prior extent to keep edge content inside the reference Daydream View comfort area;
- the universal Quick Menu captures the headset's live horizontal heading when opened and renders in that captured direction rather than the possibly-wrong shell-forward direction, allowing orientation recovery without physically spinning to find the menu;
- startup forward orientation follows the live headset forward vector through a short stability window before locking, with a bounded timeout fallback, and all headset/controller forward calculations now use tested shell-consistent yaw polarity;
- startup heading is explicitly initialized from the first headset heading rather than leaving shell yaw at the sensor/world default;
- the same shell-owned modal now serves as a first universal Quick Menu slice, reachable from Daydream App/Menu, Cardboard system menu, gamepad Start/Mode, and keyboard Menu; it exposes Headset Forward, Controller Forward when a fresh pose exists, volume down/up, contextual Back, Home, Exit to Phone, and Close;
- orientation changes are software-owned: shell yaw and controller yaw-calibration advance together when preserving controller physical direction, while Center on Controller rotates the shell until the current tracked controller ray becomes forward;
- focus transitions have quiet rate-limited audio, activation uses positive feedback, and rejected actions retain distinct failure feedback;
- unavailable/inert menu targets do not acquire hover or accept activation;
- Daydream touchpad-click activation;
- Menu/back navigation through the launcher hierarchy;
- Home controller + software-yaw recenter request;
- Exit to Stage A;
- no dead Media placeholder;
- no fake-success module entries;
- persistent VR-startup phase marker armed at Enter VR and cleared only after the renderer completes its first frame;
- recoverable Java/JNI/GL startup failures return to Stage A with explicit failure feedback rather than silently dropping the user;
- interrupted process-level launches are reported on the next Stage A start with the last persisted startup phase;
- renderer surface/new-frame/draw-eye callbacks contain recoverable RuntimeException/LinkageError failures and report the failing phase;
- phone-test CI requires Cardboard JNI `libcardboard_sdk_jni.so` in both packaged target ABIs.

Reference-device startup defect RV-0092 is resolved: the physical Galaxy S9 now reaches the rendered VR shell instead of exiting before frame one. Issue #3 is closed after visible handset evidence of the center interaction marker, close control, phone/controller battery HUD, and settings control.

Current validation: phone-test #43 completed successfully against `8e62a6772bbdfc61315431d58da89ec790f8d6ee`, passing JVM tests, unsigned Android/native debug assembly, persistent phone-test signing preparation, non-debuggable phoneTest packaging, final APK verification, artifact preservation, and prerelease publication as `phone-test-43-1`. The release retains signer SHA-256 `464dc2d100c3bb14deac1da680d29418cc22e60e1857852cb54541f7d7787f7c` and versionCode `43001`.

Phone-test #43 includes deterministic first-frame headset heading initialization, a shell-owned floating Orientation palette, Daydream App/Menu and Cardboard/gamepad/keyboard menu routing, Center on Headset, Center on Controller, Back One Level and Close actions, modal hosted-input suppression, and software controller-yaw calibration that follows shell heading changes without issuing an unrelated Daydream hardware-zero command.

Remaining gate: physical Galaxy S9 + Daydream View validation of startup heading behavior, Center on Headset, Center on Controller, menu placement/readability over Home/Media/DOS/Native surfaces, the 10% comfort contraction, HUD edge visibility, inertial translation direction/gain/settling feel, wrapped Home panel comfort/readability, Daydream model orientation/scale, emitter-ray alignment, and gamepad virtual aim. Build/package success is not device-comfort acceptance.

### RV-0109 — VR shell/environment ambience

State: **draft**

The user-supplied **Starry Cereal** track is packaged as the VR shell's looping ambient menu music. The APK asset is a 267.312-second, 48 kHz stereo MP3 optimized to 48 kbps for handset package size (asset SHA-256 `dd0240297ea62fe2002b736ce2b43002cafc307ad76e32d795587012ffb99c00`; supplied source SHA-256 `26d5f261b080b913cce08ec8ea7e75105fbb6fb1d14a3808d4298302a24454cf`). Playback is shell-owned: it pauses for Activity lifecycle loss and whenever local video, DOS, or Native hosted content starts, resumes from the previous music position when the shell returns, loops continuously, and fails quiet rather than breaking VR startup.

Remaining gate: Galaxy S9 + Daydream View listening validation for perceived level, looping behavior, transitions, and long-session comfort. User-facing music volume/mute remains part of RV-0203 audio settings.

### RV-0216 — universal quick-access shell panel

State: **draft**

Implemented first slice:

- Menu/Start opens one shell-owned modal over Home, local media, DOS sessions, native modules, setup, and child menus;
- hosted/module input is suppressed while the modal is open so recovery actions cannot leak into guest content;
- real actions are exposed for headset-forward calibration, tracked-controller-forward calibration when a fresh pose exists, volume down/up, contextual Back, Home, Exit to Phone, and Close;
- Home and Exit stop active media/DOS/native content through the shell-owned lifecycle before mode changes;
- the modal also reports phone/controller battery, elapsed VR session time, Android thermal status, battery-sensor temperature, and rolling frame-time p95, refreshed once per second while open;
- brightness down/up adjusts the VR Activity window in bounded 10% steps without requiring global Android brightness-write permission;
- a nested Quick Settings page changes battery HUD visibility, numeric percentages, look-up reveal, pointer mode, and UI scale without leaving hosted content;
- unavailable controller-forward calibration is visibly disabled rather than pretending to work.

RV-0216's planned functional controls are now represented by implemented shell behavior. Remaining gate: Galaxy S9 + Daydream View comfort/readability and interaction validation before acceptance.

### RV-0108 / RV-0206 — first-run setup and user optical calibration

State: **draft**

The first successful VR entry routes into a resumable setup flow unless the current setup version is complete.

Current setup schema: **v2**.

Current functional pages:

1. neutral seated forward/recenter;
2. controller familiarization with active source and last normalized action feedback;
3. virtual user eye-spacing/IPD adjustment;
4. UI/readability scale;
5. battery HUD / look-up-mode preferences;
6. save-to-home or return without marking setup complete.

The familiarization page captures Back locally so it can be tested safely without leaving the page. Select, Recenter, navigation and volume actions remain live and are reported through the normalized action layer. The page is informational rather than a hard capability gate.

The renderer reads the saved physical Cardboard viewer profile when available and keeps the user eye-spacing value separate. User spacing is applied as a per-eye render-view correction rather than overwriting the physical viewer profile.

Current safety limits clamp user eye spacing to 50–80 mm. These are provisional engineering bounds and remain subject to Galaxy S9 + Daydream View validation.

If no saved Cardboard viewer profile exists, the draft renderer uses a 60 mm physical inter-lens fallback. That fallback must be replaced or confirmed through reference-headset profiling before optical calibration is considered accepted.

The complete planned onboarding remains broader than the current implementation: viewer confirmation, per-eye fine correction, comfort, audio and performance pages still remain.

### RV-0093 — updater

State: **draft**

Implemented:

- optional check on Stage A launch and explicit manual check;
- authoritative source restricted to public `mrcalzon02/ReverieVR` GitHub Releases with no GitHub token in the APK;
- phone-test prerelease discovery through the release list rather than `/releases/latest`;
- numeric `phone-test-<run>-<attempt>` ordering and selection of the highest valid newer release;
- dedicated non-debuggable `phoneTest` distribution build for handset and controller companion;
- permanent direct-distribution signing identity provisioned through protected GitHub Actions secrets;
- rerun-safe Android versionCode `GITHUB_RUN_NUMBER * 1000 + GITHUB_RUN_ATTEMPT`;
- exact installed build identity displayed in Stage A;
- GitHub SHA-256 asset digest required fail-closed;
- downloaded APK privately parsed before Android's installer sees it;
- package ID, strictly newer versionCode, phone-test tag/versionCode mapping, installed signer, and pinned permanent signer verified in-app;
- candidate signer verified in-app when Android exposes archive signing metadata; if an OEM/platform archive parser omits that metadata after all other checks pass, final replacement-signer continuity is delegated to Android Package Installer rather than falsely rejecting a valid package;
- trusted repository release URL filtering;
- Android Download Manager / package-installer handoff;
- updater controls omitted entirely outside the persistent signing channel;
- VR Home can request an update check and hands the operation back to Stage A.

Permanent signer evidence:

- certificate SHA-256: `464dc2d100c3bb14deac1da680d29418cc22e60e1857852cb54541f7d7787f7c`;
- phone-test #36 / versionCode 36001: first permanent-lineage package;
- phone-test #37 / versionCode 37001: same signer;
- phone-test #38 / versionCode 38001: same signer, full signed QoL package;
- phone-test #40 / versionCode 40001: same signer; updater archive-signer compatibility repair and CardboardView final-APK presence gate passed;
- CI independently verifies both headset and companion APK signer fingerprints before every release publication.

The old updater failure is fully explained: prereleases were invisible to the original endpoint and historical CI packages used different ephemeral debug signing certificates. Android correctly refused those packages as in-place replacements.

One transition install from the old ephemeral-debug lineage remains unavoidable. After that, the permanent lineage is designed for normal in-place upgrades.

Remaining acceptance gate: on the physical Galaxy S9, install the first permanent-lineage build once, use ReverieVR to discover a later permanent-lineage release (currently #38), allow Android to replace it without uninstalling, and verify ReverieVR settings/data survive. Issue #4 remains open until that device proof is captured.


### RV-0500 / RV-0501 — local flat video and mono 360 video

State: **draft**

Implemented as an intentionally early vertical slice (see ADR-0008):

- Stage A local-video selection through Android `ACTION_OPEN_DOCUMENT`;
- persisted read permission scoped to the selected document rather than broad media/storage access;
- remembered display name and projection mode;
- Android `MediaPlayer` asynchronous decode;
- decoder output through `SurfaceTexture` / `GL_TEXTURE_EXTERNAL_OES`;
- flat virtual-screen geometry with decoder-reported aspect ratio;
- mono equirectangular 360° sphere geometry;
- Daydream click play/pause;
- horizontal touchpad swipe seek requests mapped to ±10 seconds;
- Menu/back stop-and-return to the shell;
- GL-surface recreation path that releases the old decoder surface and reopens the selected URI against the replacement surface.

Not yet implemented/accepted:

- stereoscopic SBS/over-under media layouts;
- richer seeking UI/scrubber, library browser, and subtitles;
- shell-global player HUD polish;
- codec-coverage matrix;
- sustained decode, thermal, A/V sync, seam/orientation, and comfort validation on the Galaxy S9.

Remaining gate: successful build plus physical flat-video and mono-360 playback tests on the reference hardware.

### RV-0006 — scrcpy / ADB diagnostic compatibility

State: **draft**

ReverieVR now treats scrcpy compatibility as a first-class developer requirement rather than an incidental convenience.

The compatibility contract requires:

- Stage A must remain operable through normal scrcpy-injected mouse/keyboard/touch input;
- Stage B must remain mirrorable/recordable while the phone display stays on in the headset;
- no blanket Android `FLAG_SECURE` policy may be introduced on ReverieVR's normal app windows;
- the reference runtime must not seize the USB data path in a way that prevents normal ADB/scrcpy use;
- Daydream BLE controller transport must coexist with wired USB debugging;
- scrcpy host-audio absence on Android 10-or-earlier reference software is not treated as a ReverieVR audio failure;
- `--turn-screen-off` is prohibited for in-headset testing because the physical phone display is the VR display;
- scrcpy-assisted runs are valid for functional observation, capture, Stage A control, crash reproduction, and UI debugging, but are **not** valid sustained performance/thermal acceptance runs because mirroring/encoding adds workload.

Helper launchers are provided in `scripts/scrcpy-reverie.sh` and `scripts/scrcpy-reverie.bat`.

Remaining gate: verify USB debugging + scrcpy against the real Galaxy S9 while Daydream BLE is connected and Stage B is actively rendering.

### RV-0106 / RV-0107 — global power HUD and look-up reveal

State: **draft**

Implemented:

- separate transparent HUD texture rather than battery text baked into the menu panel;
- two always-available status rows while the HUD feature is enabled: PHONE and CTRL;
- real progress-bar fill from 0–100%;
- optional numeric percentage display using the existing Show Percentages preference;
- honest controller/phone unknown state shown as an empty bar plus `--`, never fabricated 0%;
- overlay rendered after both the normal VR shell and local video scene so media playback does not hide battery status;
- compact upper-right placement during normal forward viewing;
- optional look-up reveal using the player head-forward vector; above the current provisional threshold the HUD drops lower into the view;
- Battery HUD and Look-up Reveal remain user-toggleable QoL features;
- HUD geometry and texture storage are reused without per-eye allocation.

Remaining gates:

- Galaxy S9 / Daydream View legibility and binocular-comfort validation;
- verify the provisional look-up threshold feels natural;
- validate controller orientation axes before adding controller-pointing-up as an alternate reveal trigger;
- confirm no meaningful frame-time or thermal regression.

### RV-0202 / RV-0207 — virtual bindings and standard HID

State: **draft**

Implemented:

- stateful `InputBindingEngine` distinct from one-shot shell navigation actions;
- versioned persistent binding-profile codec;
- source signals for Select/Back/Recenter/volume, navigation, touchpad axes, Android gamepad axes, and head yaw/pitch deltas;
- transforms for digital, analog, positive-threshold and negative-threshold mappings;
- configurable scale, deadzone and threshold values;
- virtual guest keyboard with held-key lifetime;
- virtual mouse buttons, relative deltas, absolute pointer coordinates and wheel accumulator;
- virtual joystick axes/buttons;
- release-all behavior on profile change, Activity pause, controller disconnect and input-device removal;
- built-in starter profiles for DOS Doom-style control, generic head-mouse FPS control, and touchpad absolute-cursor control;
- direct Android HID keyboard passthrough for common DOS keys;
- direct Android HID mouse passthrough for left/right/middle buttons, relative/absolute motion and wheel;
- mouse events intercepted through both generic-motion and mouse-sourced pointer dispatch;
- pure-Java tests for held/released keys, threshold release, relative mouse accumulation, profile persistence, DOS content extensions and virtual mouse cleanup.

Remaining gates:

- actual Bluetooth keyboard test;
- actual Bluetooth/USB mouse test;
- scrcpy SDK and UHID input validation;
- relative pointer capture while a hosted DOS session owns the mouse;
- in-VR binding overlay/editor and host-reserved escape path.

### RV-0400 / RV-0401 — native procedural module foundation

State: **draft**

ADR-0015 now governs the native-game path.

Implemented foundation:

- native module ABI v1 uses fixed-width C structures with explicit structure sizes and ABI versions rather than exposing C++ classes across module boundaries;
- packaged modules are selected through a compile-time allowlist and loaded with `dlopen` / a fixed entry symbol;
- unknown module ids, missing symbols, descriptor/id mismatches, unsupported ABI versions, unsupported GLES major requirements and missing callbacks fail closed with diagnostics;
- lifecycle includes create/destroy, GL-context creation, explicit GL-context release, resume/pause, update and per-eye render callbacks;
- the host owns Cardboard, Activity lifecycle, shell recovery and the active GL context;
- the NDK build is now unconditional for the headset app while DOSBox Pure remains an optional additional native target;
- both `armeabi-v7a` and `arm64-v8a` remain target ABIs;
- OpenKTG is vendored from `jaromil/kkrieger-werkkzeug3` revision `72f7697c8b5be6fadae41f9ca6312cd5f88fdc4c` with its public-domain provenance;
- the separate `libreverie_module_test_chamber.so` expands an OpenKTG procedural texture at module creation and renders a deliberately simple GLES2 chamber;
- the proof module accepts normalized movement plus primary/secondary input and restores GL program/buffer/texture and enable-state ownership after each eye render;
- no Win32, Direct3D, DirectSound, kkrunchy/YASM runtime or arbitrary downloaded native code was introduced.

Stage B integration now implemented:

- VR Home exposes the packaged Procedural Test Chamber as a sixth shell-owned action;
- module launch activates a dedicated hosted profile mapping touchpad axes to virtual joystick X/Y and Select to virtual joystick button 0;
- native update executes on the Cardboard render thread using the existing normalized virtual input bus;
- each eye receives Cardboard's eye view plus ReverieVR user-IPD correction and Cardboard projection;
- Back is consumed by the shell, releases module GL resources on the render thread, stops the module and restores the previous binding profile;
- Activity pause/resume is forwarded without creating a second Activity or EGL context;
- renderer shutdown explicitly releases module GL resources before Java/native destruction;
- the shell-global power/status HUD renders after native module content.

Current-environment verification completed:

- remote `main` readback confirms ADR-0015, the ABI header, native host, separate test-module source/library target, Java runtime bridge, Stage B wiring and OpenKTG provenance are present;
- the four vendored OpenKTG files have exact Git blob SHA matches to the pinned upstream revision;
- static source checks confirm the NDK build no longer hard-fails when DOSBox Pure is absent;
- static source checks confirm unknown native-module ids fail closed, ABI/descriptor/callback validation is present, and only the compile-time allowlisted packaged test library is named;
- the proof module source contains no Win32, Direct3D, DirectSound or x86 inline-assembly path;
- Java/C++ structural brace checks passed for the new host/runtime/module and Stage B integration sources;
- GitHub Actions phone-test run #6 successfully executed the committed bootstrap verifier, JVM tests, headset and controller APK builds;
- Cardboard native code compiled and linked for both `arm64-v8a` and `armeabi-v7a` with Vulkan/Unity disabled;
- the headset APK passed an explicit package check for both ABIs of `libreverie_native_host.so`, `libreverie_module_test_chamber.so`, and `libreverie_dos_host.so`;
- `apksigner verify --verbose --print-certs` passed for the published headset APK;
- GitHub prerelease `phone-test-6-1` was published from commit `c70026f8e5bd307bd5f53d46c862a87b629bff20`;
- no Galaxy S9/device-runtime claim is made until the APK is installed and exercised on the reference hardware.

Remaining gates:

- Galaxy S9 stereo/head-tracking/movement/Select/Back/HUD/pause-resume validation;
- sustained frame-pacing, battery and thermal validation;
- shared module settings/audio/save/performance-budget services beyond the proof ABI.

This is the ADR-0015 validation stop. Do not expand into Kkrieger gameplay,
Werkkzeug3 mesh/operator porting or V2 audio until the proof module passes the
reference-device gates above.

### RV-0610 — Between Deliveries: The Red Ledger VR manual-service/dev-test foundation

State: **draft**

The first full native-game track remains deliberately bounded to a one-room
seated bar loop while the native host and physical Daydream interaction path
are being proven.

Implemented foundation:

- deterministic cash, debt, beer, cup, reputation, pressure, supplier,
  protection, inspection and supply-interruption simulation;
- four recurring patron archetypes and day-close accounting;
- low-complexity GLES2 concrete-room renderer with bar, working tap, wash area,
  cup rack, supplier cards, ledger, patron proxy, back-room props, event marker
  and flickering light;
- append-only ABI-v1 host-calibrated tracked/virtual-controller ray input and
  bounded module-private persistence callbacks;
- manual service state machine: take/return a clean cup, fill it at the tap,
  hand a filled cup to the patron, collect payment, wash returned cups, order
  supplies, handle protection, turn away a patron through the exit target and
  close the day through the ledger;
- cash and patron progression occur only when payment is explicitly collected;
- the game is fixed to its intended seated work position rather than inheriting
  Test Chamber locomotion;
- save schema v2 is an explicit 160-byte little-endian format preserving
  held-drink and pending-payment state with malformed-state rejection;
- successful work-surface actions persist through the host and valid v2 state
  is restored on module creation;
- Red Ledger has a dedicated hosted binding profile containing only Select ->
  virtual joystick button 0. Controller aim comes from the shell ray, so
  Test Chamber touchpad movement is not exposed as an inert game control;
- the trusted native host recognizes Red Ledger solely for device testing.
  Standard logging filters it out of Native Apps, Development logging exposes
  it as a `[DEV]` entry, and NativeModuleRuntime separately rejects a Red
  Ledger start unless the VR session was created in Development mode;
- Quick Menu suppression and shell-owned Back/Home/recenter/recovery remain in
  force over the native module.

Verified evidence:

- phone-test #52 completed successfully from commit
  `48d4f2f1d22c4370bba5caea5a612d06ba2018b2`;
- its C++17 `-Wall -Wextra -Wpedantic -Werror` Red Ledger test covered cup
  pickup/return, fill, serve, pending payment, explicit payment collection,
  washing, protection, day close, mid-transaction save/restore, corrupt-save
  rejection, inspection, supply interruption and patron turn-away;
- the same run completed the full unsigned Android/Java/JNI/NDK tree,
  persistently signed packages, APK verification, artifact preservation and
  prerelease publication;
- `phone-test-52-1` published `ReverieVR-phone-test-52-48d4f2f.apk` with
  SHA-256 `e425419a612752611860674d55f728d6edb11d289c5980a22506527024140fbf`;
- APK verification continued to require `libreverie_module_red_ledger.so` for
  both `armeabi-v7a` and `arm64-v8a`;
- phone-test #60 completed successfully from combined commit
  `416e866a7c87cf7bb802920afa9f786d73bb1874`, proving the Development-only
  enumeration, fail-closed Red Ledger runtime gate, dedicated Select-only
  binding profile/JVM regression test, full Java/JNI/NDK integration, signed
  packaging and dual-ABI APK verification together with the stereo-isolation
  repair;
- `phone-test-60-1` published headset APK
  `ReverieVR-phone-test-60-416e866.apk` with SHA-256
  `7a8aa42b47ff684a1393a244582198c9ed0ce3cd7578aa7da7291d72c3ed22ee`;
- no Galaxy S9 runtime-load, controller-usability, save durability, comfort or
  thermal acceptance is claimed by static/package evidence;
- the next physical-test build adds a Development-only ray-contact cursor at
  the selected AABB hit point and normalizes the module-side pointer direction
  defensively without changing ABI v1.

Remaining gates before normal user-visible launch:

- install the current development-admission build on the Galaxy S9, set
  launcher Logging to Development, enter VR and launch
  `Between Deliveries: The Red Ledger VR [DEV]` from Native Apps;
- physically exercise cup rack, tap, patron, payment, wash area, ledger,
  supplier cards, protection envelope and exit target, then tune reach/volumes
  from headset evidence;
- verify held-cup placement along the controller ray;
- verify Quick Menu, Home, Back and both recenter paths while Red Ledger is
  active, including hosted-input suppression behind the modal;
- verify v2 state across real exit/re-entry and an in-place APK update;
- perform the required 15-minute S9 frame-pacing/battery/thermal pass;
- only after those gates remove the Development-only filter and `[DEV]` label.

### RV-0410 / RV-0411 — DOS runtime baseline and modules

State: **draft**

Selected runtime:

- DOSBox Pure;
- tag `1.0-preview6`;
- pinned commit `a4a0bab7f8931433588f2fcad9045c85b277373d`;
- GPLv2 or later explicitly accepted for this integration;
- upstream Android NDK build already supports `armeabi-v7a` and `arm64-v8a`;
- upstream libretro metadata advertises keyboard/mouse focus and direct content support for ZIP/DOSZ, EXE/COM/BAT, ISO/CHD/CUE, IMG/IMA/VHD/JRC, M3U/M3U8, CONF and directories.

Implemented module foundation:

- Stage A DOS import panel;
- Android document picker for file/container imports;
- background copy into `files/dos-modules/<module-id>/content`;
- sanitized stable native filesystem path;
- per-module metadata file with display name, original filename, binding-profile id and import timestamp;
- module listing/deletion repository;
- explicit warning in design that a bare EXE is insufficient for games that depend on sibling assets;
- pinned upstream fetch scripts with dirty-checkout refusal and post-checkout SHA verification;
- DOSBox Pure source checkout excluded from ReverieVR git history while the pin/provenance stays committed.

Implemented native host foundation:

- conditional Gradle/ndk-build activation when the pinned DOSBox Pure checkout exists;
- upstream DOSBox Pure built as `libretro.so` for `armeabi-v7a` and `arm64-v8a`, with ReverieVR's JNI frontend built as `libreverie_dos_host.so`;
- libretro init/load/run/unload/deinit lifecycle behind the existing `DosNativeRuntime` Java contract;
- app-private system, save and content-directory environment callbacks;
- legacy core-option default capture, with 48 kHz audio and software-multithreaded Voodoo forced until a hardware-render callback exists;
- XRGB8888 framebuffer capture into a tightly packed host buffer;
- bounded two-second stereo PCM ring buffer;
- synchronized virtual keyboard, mouse, pointer and joystick polling from the existing virtual input bus;
- core shutdown request propagation and Android log forwarding;
- bootstrap verification rejects a fetched DOSBox Pure checkout whose commit does not match the project pin;
- host-side C++ syntax/type checking and JNI descriptor verification passed.

Additional Stage B integration now implemented:

- a dedicated Java DOS session worker owns native run cadence without executing DOSBox Pure on the Cardboard GL thread;
- lifecycle pause/resume releases guest input and pauses/resumes audio;
- a fifth VR Home action opens a paged in-headset DOS library and can launch any imported/bundled module without returning to the touchscreen;
- module launch temporarily activates that module's binding profile and restores the prior profile when the session ends;
- a nearest-filtered OpenGL guest texture consumes the native XRGB8888 frame, corrects byte-channel order in the shader, flips libretro's top-down image, and applies RV-0414 intended display aspect;
- Android AudioTrack consumes stereo 16-bit PCM on the DOS worker thread;
- Back now opens a shell-owned DOS quick overlay rather than terminating the guest immediately;
- opening the overlay pauses DOS execution/audio, releases current guest input and suppresses new controller/head/HID guest signals until Resume;
- overlay pause and Activity lifecycle pause are tracked independently, preventing Activity resume from silently resuming a guest behind the menu;
- the paused guest framebuffer remains visible behind a translucent shell panel;
- quick-overlay actions provide Resume, Recenter, media-volume down/up, Home and Exit VR;
- the active hosted binding-profile name is shown in the overlay while the shell retains control of recovery;
- the quick overlay now opens a dedicated paused DOS binding page;
- profile Previous/Next cycles DOS-capable built-ins, immediately swaps the hosted profile, persists the module's selected profile ID and clears any stale custom override;
- analog sensitivity and deadzone can be tuned in-headset while paused, with edits saved as a bounded module-local RVBIND1 override;
- Reset Tuning deletes the module-local override and restores the selected built-in profile;
- hosted-profile replacement never overwrites the user's pre-game global profile, which still restores on DOS exit;
- deterministic JVM coverage exercises signed analog sensitivity changes, deadzone bounds and digital-only no-op behavior.

Current-environment verification:

- phone-test run #8 exposed two stale AudioTrack references to the removed single `paused` flag and failed Java compilation as intended;
- commit `7133b3b40bbf6853b7fb2d5e4ba7eb1970bc26cd` replaced both stale references with the pause gate and source readback confirmed no `paused` variable references remain;
- phone-test run #9 passed JVM tests, `:app:assembleDebug`, `:controller-app:assembleDebug`, dual-ABI native-library APK presence checks and APK signature verification;
- run #9 published prerelease `phone-test-9-1` from commit `ba362cffed915f179c8a794803d05993239d6db6`;
- phone-test run #10 passed the complete JVM/Android/NDK build and verification path with the binding editor present;
- run #10 published prerelease `phone-test-10-1` from commit `72f2ab623e0993d9531a8b6aa7e333334ae96de0`.

Remaining gates:

- reference-device validation of DOS run cadence, audio latency/underruns, guest texture orientation/colors and resolution changes;
- full arbitrary per-binding remapping beyond the current profile selection plus analog sensitivity/deadzone tuning;
- richer library management such as sorting/filtering/favorites beyond the current paged launcher;
- integer-scale staging policy beyond nearest-neighbor guest sampling;
- directory-tree import;
- run the verified DOOM Shareware installer/autostart path and exercise E1M1 on the Galaxy S9.

### RV-0007 — Standard/development logging and manual diagnostic bundles

State: **draft**

Implemented:

- Standard mode defaults to milestone/incident/warning/error/fatal records;
- Development mode adds raw controller/gamepad/HID input, normalized inputs, binding transforms/outputs, head motion and per-frame VR diagnostic state;
- bounded 8192-record in-memory queue;
- rotating app-private files: Standard 1 MiB + 4 archives, Development 16 MiB + 4 archives;
- uncaught exceptions are synchronously drained to local logs before delegating to Android's existing crash handler;
- Stage A logging-mode selector;
- explicit Clear Local Logs action;
- explicit Export Diagnostic Bundle action using Android's create-document UI;
- ZIP manifest includes app/build/device/API/ABI/logging-mode/DOS-runtime-build state;
- manual export remains a permanent offline/private fallback independent of network submission;
- privacy warning that Development logs can include device names, local filenames, module names, input state and timing.

Development logging invalidates performance/thermal/frame-pacing acceptance evidence because it intentionally performs high-volume diagnostic work.

Automatic opt-in issue submission is tracked separately as RV-0208 so the manual logging/export acceptance boundary stays independent.

### RV-0208 — secure one-tap diagnostic issue submission

State: **draft**

Implemented on `main`:

- Stage A exposes **Submit diagnostics to GitHub** only when a valid HTTPS diagnostic-intake endpoint was injected at build time; absent configuration hides the control rather than leaving a dead button;
- submission requires an explicit privacy/reproduction dialog, a non-empty failure summary, optional expected behavior, and an explicit coarse-redaction choice to include or exclude ReverieVR log files;
- ReverieVR builds the exact ZIP first, calculates its `revdiag-<UUID>`, byte length and SHA-256, then shows a second review screen containing the public issue text/device/build metadata and whether logs are included before any network upload occurs;
- the Android application contains no GitHub token, GitHub App private key, storage credential, Cloudflare credential, or reusable privileged upload secret;
- failed uploads retain the generated ZIP in the app-private diagnostic outbox while the existing manual Export Diagnostic Bundle path remains available;
- if private storage succeeds but GitHub issue creation fails, the broker writes a pending receipt and `POST /v1/diagnostics/finalize` can retry issue creation using the diagnostic ID + exact SHA-256 without retransmitting the ZIP; Stage A persists that pending receipt identity across app restarts and offers retry before starting a new report;
- the first broker implementation is a Cloudflare Worker under `diagnostic-intake/` with a private R2 bundle store, SQLite-backed Durable Object rate limiter, bounded request/ZIP validation, canonical local-header/central-directory filename checks, server-side SHA-256 recomputation, scheduled 30-day raw-bundle/receipt expiry, and GitHub App installation-token issue creation;
- public GitHub issues contain sanitized build/device/reproduction metadata, diagnostic ID, byte length, server hash, and private receipt reference only; raw ZIP/object keys and pending metadata are not automatically published;
- broker deployment is isolated behind the protected GitHub environment `diagnostic-intake`, while the non-secret public endpoint is supplied separately through the `DIAGNOSTIC_INTAKE_URL` Actions variable;
- GitHub Issue #1 tracks the end-to-end acceptance work and ADR-0017 remains the governing security boundary.

Current verification: phone-test run #19 completed successfully against commit `5a35615691fbdce4f9a73a5fdbf97df2f66de91d`. It passed Worker syntax and broker smoke validation, JVM tests (including diagnostic ID/SHA-256 and stored-receipt retry state), Android/native/DOS assembly, APK-content verification, signature verification, release staging and prerelease publication. The published tag is `phone-test-19-1`. Earlier runs #15-#18 were superseded/cancelled under the new latest-build-wins concurrency policy rather than treated as acceptance evidence.

Remaining gates before RV-0208 can be accepted:

1. provision the `reverievr-diagnostics` R2 bucket and least-privilege GitHub App;
2. populate the protected `diagnostic-intake` environment secrets and deploy the Worker;
3. set `DIAGNOSTIC_INTAKE_URL` to the deployed HTTPS `/v1/diagnostics` endpoint and produce a phone-test build containing that public URL;
4. perform a real Galaxy S9 submission and verify the client hash, server hash, private receipt and created GitHub issue agree;
5. verify failure/retry behavior and that no privileged credential appears in the APK or public issue.

### RV-0414 — retro framebuffer display geometry

State: **draft**

Implemented:

- recognized source modes: 320x200, 320x240, 640x350, 640x400, 640x480, 720x400, 800x600, 1024x768, 1280x1024, 160x144 and 240x160;
- raw source aspect and intended physical display aspect are represented separately;
- Mode 13h/640x400/text modes can be presented as intended 4:3 instead of blindly stretching according to raw pixel dimensions;
- 1280x1024 remains 5:4;
- unknown modes fall back to native source aspect;
- integer-scale calculation exists for pixel-stable intermediate presentation;
- VR shell/HUD resolution remains independent from low-resolution guest content;
- Cardboard remains authoritative for actual per-eye viewport/projection/distortion rather than hard-coding generic 8:9/5:4 eye buffers or a universal supersampling factor.

Remaining gate: connect this geometry to the DOS framebuffer texture renderer and validate pixel/aspect/readability behavior in the Daydream View.

### RV-0415 — bundled DOOM Shareware

State: **draft**

Default-content baseline:

- DOOM Shareware v1.9;
- authoritative distributed payload: original `doom19s.zip`;
- expected byte size: 2,450,688;
- SHA-1 `8d0fbbbeba5ecb692a99f97e55dfb5365cfe5b77`;
- SHA-256 `cacf0142b31ca1af00796b4a0339e07992ac5f21bc3f81e7532fe1b5e1b486e6`;
- fetch helper uses an idgames archival mirror and refuses an unexpected digest;
- fetched binary is ignored by ReverieVR git;
- free release builds include it by default when fetched;
- release builds refuse an accidentally missing archive unless `-PexcludeDoomShareware` is explicitly supplied;
- Android packages the original archive unchanged as `assets/default-content/doom19s.zip`;
- Stage A startup re-verifies the archive and registers the built-in `DOOM Shareware v1.9` DOS module using the `dos-doom-shareware` binding profile;
- the original archive remains under the module's `content/` directory as the checksum-verifiable authoritative payload;
- a separate `runtime/` directory is regenerated from the verified ZIP with bounded extraction and canonical-path traversal rejection;
- the runtime adds generated `DOS.YML`, `DOSBOX.BAT` and `REVERIE.CFG` files without modifying/repacking the distributed archive;
- first launch uses DOSBox Pure `run_path` / `run_input` metadata to run DEICE with the C-drive/default-directory/confirmation sequence, then `DOOMS_19.EXE -d`, then `DOOM.EXE`;
- once `DOOMS/DOOM.EXE` exists, launch preparation deletes the bootstrap batch and rewrites `DOS.YML` to start Doom directly with no installer keystrokes;
- the generated first-run config enables mouse and SB16 SFX at 0x220 / IRQ 7 / DMA 1 while leaving music disabled until separate music validation;
- directory content is now an accepted DOS module content root so DOSBox Pure can mount the generated working tree directly;
- pure-Java tests cover ZIP extraction bounds/path traversal and the first-install/direct-play metadata plans;
- registered/full DOOM data is not bundled;
- the engine GPL and shareware-data license remain separate licensing domains.

Current-environment verification:

- phone-test run #11 passed the complete JVM/Android/NDK build and verification pipeline with the Doom bootstrap present;
- the build fetched the checksum-pinned `doom19s.zip` input before assembly and produced the signed headset/controller APK pair;
- run #11 published prerelease `phone-test-11-1` from commit `a6c06d3b4480b52be6cb6307123f38c4e5f1697d`;
- the published headset APK is `ReverieVR-phone-test-11-a6c06d3.apk` with release-reported SHA-256 `b43a8ea9047f6d76cba70bab2228633a62232e427a73e6585b48e4100d8e2b23`.

Distribution rule:

DOOM shareware is copyrighted shareware, not public domain. The default bundled-content path is for free/noncommercial ReverieVR releases. A paid/commercial distribution must obtain appropriate permission or deliberately omit the shareware archive.

Remaining gates:

- install phone-test #11 on the Galaxy S9;
- validate the DEICE prompt timing and generated runtime installation on the Galaxy S9;
- validate Sound Blaster SFX and decide the later music path without reintroducing SETUP.EXE as a mandatory first-run gate;
- verify E1M1 launch, saves, 320x200-to-4:3 presentation, input bindings, overlay/recovery and sustained thermal behavior.

### RV-0005 — Stage B frame cadence instrumentation

State: **draft**

Implemented:

- allocation-bounded rolling window of the latest 600 Cardboard frame intervals;
- lifecycle/background gaps over one second are excluded rather than misreported as catastrophic frames;
- one-minute Standard-log summaries report average, p95 and maximum frame interval plus counts above 1.5x and 2x the 60 Hz target interval;
- instrumentation is driven from Stage B `onNewFrame`, so shell, media, DOS and native-module modes use the same cadence evidence path;
- deterministic JVM coverage verifies cadence math, slow/severe classification, lifecycle-gap rejection and bounded storage;
- Stage B now keeps the latest battery-temperature broadcast reading and Android 10+ PowerManager thermal-status state outside the render loop;
- the once-per-minute `VR_PERFORMANCE` record correlates those thermal observations with the same cadence snapshot;
- Android versions without thermal-status support and failed/unavailable sensor reads are logged explicitly as `unavailable`, never fabricated;
- pure-Java coverage verifies battery-temperature formatting, thermal-status labels and unknown/unavailable behavior.

Interpretation boundary: these measurements are headset callback cadence, not GPU timer-query results. Battery temperature is the handset battery sensor reported by Android and must not be described as direct SoC, CPU or GPU junction temperature. Android thermal status is a platform throttling/severity signal, not a temperature reading. They are suitable for sustained regression evidence but must not be described as direct GPU render time. Development logging remains disallowed for acceptance evidence because its intentionally heavy diagnostics perturb timing.

Current-environment verification: phone-test run #7 passed JVM tests, `:app:assembleDebug`, `:controller-app:assembleDebug`, native-library APK presence checks and APK signature verification with the thermal-correlation instrumentation present. The pure-Java formatter was independently compiled from the committed blob and produced the expected available, unavailable and unknown-status forms.

Remaining gate: sustained Galaxy S9 headset runs without scrcpy to establish measured acceptance thresholds and observe whether cadence degradation correlates with battery temperature and Android thermal-status escalation.

## Performance posture

The Stage B shell deliberately avoids a general-purpose game engine. Cardboard is configured for OpenGL ES with Vulkan and Unity integration disabled. The shell avoids per-frame gaze/button-geometry allocation and reuses its UI bitmap/texture storage instead of allocating a new 1024×768 bitmap for every hover change.

No performance/thermal acceptance claim exists until sustained Galaxy S9 testing is performed.

## Quality-of-life baseline

The project backlog includes remembered global comfort preferences, universal recenter/seated recovery, scalable/high-contrast UI, captions/visual audio alternatives, flexible one-controller/remappable input, reduced-motion behavior, controller calibration/reconnect handling, quick access, nonblocking notifications, optional session reminders, and research-only camera peek.

Automatic convenience behaviors remain user-toggleable unless required for recoverability.

## Build environment and phone-test release

The interactive execution environment still does not expose a local Android SDK/NDK/ADB stack. Explicitly authorized GitHub Actions now supplies the reproducible cloud Android build environment instead.

Phone-test run #11 completed successfully from commit `a6c06d3b4480b52be6cb6307123f38c4e5f1697d` and published prerelease `phone-test-11-1`. The headset APK is a debug-signed sideload build for reference-device validation, not a production signing identity. Future phone-test builds may require uninstall/reinstall until persistent protected test/release signing is configured.

No Galaxy S9, headset, optical, Bluetooth, thermal, or comfort validation has yet been claimed.

## Next exact action

1. install the headset APK from GitHub prerelease `phone-test-11-1` on the Galaxy S9;
2. launch Stage A and confirm ordinary Android startup before entering the headset;
3. confirm **DOOM Shareware v1.9** appears in the DOS library, launch it once, and observe the automated DEICE/bootstrap path through to `DOOM.EXE`;
4. exit and relaunch DOOM to verify the second launch bypasses installer automation and starts the installed game directly;
5. verify Doom mouse/head-turn, movement/fire bindings, Sound Blaster SFX, overlay pause/recovery and E1M1 presentation;
7. test Stage A physical Daydream pairing and live controller input;
8. install the companion APK on a second Android phone and validate paired-phone RFCOMM control plus controller-phone battery telemetry;
9. enter the Daydream View and validate stereo/head tracking/gaze/Select navigation with each available input source;
10. walk through the first-run calibration pages;
11. choose and play a known-good local flat video, verify play/pause/back, then repeat with a mono equirectangular 360° sample;
12. background/resume during playback to exercise SurfaceTexture/decoder reattachment;
13. launch `scripts/scrcpy-reverie.sh` or `scripts\\scrcpy-reverie.bat` over USB and verify Stage A can be operated remotely while a controller source remains connected;
14. enter Stage B and verify the stereoscopic output remains visible in scrcpy while the physical phone display stays on in the headset;
15. repeat a representative media/control flow under scrcpy for diagnostic capture, then close scrcpy;
16. perform sustained frame-pacing/thermal acceptance **without scrcpy running**;
17. capture any build, controller-axis, phone-controller, optical, scrcpy, media-orientation, decode, UI-scale, frame-pacing, or thermal defects for the next repair pass.

## 2026-10-12 — Second layer of details on the three Home backgrounds

Source pass adds stationary deterministic low-poly geometry in the three existing scene presets without renaming their persisted IDs: birch room trim, framed art and console props; forest canopy silhouette depth, stones and meadow blooms; beach wavelets, sand ripples, dune grass and shells. Each scene conditionally draws only its own detail meshes and all meshes are allocated once, with a no-new-texture/no-shader design and JVM vertex-budget and central menu-clearance tests. See `docs/records/HOME-ENVIRONMENT-DETAIL-LAYER.md`. **Not yet Galaxy S9 visual/performance accepted.**
