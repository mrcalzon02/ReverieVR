# Breakwater: visible trajectory and sharper horizon

Status: source fixes with actual native GLES verification; Android build and physical headset acceptance tracked separately. Baseline main: `2c42e221efa85a1c8400aa2e3042890acef95418`.

## Problems observed

The owner could not see the trajectory projection and found the panoramic horizon severely blurred. The world-scale preview still used nine 9mm-wide planar dashes after the stage expanded to hundreds of metres. Those dashes become subpixel at distance and can be viewed edge-on. Earlier showcase captures also never supplied live aiming input, leaving `aim_ray_valid` false; those captures were insufficient evidence for trajectory visibility.

The panorama loader used 512×256 runtime reductions and linear magnification over the full 360° sphere. Actual higher-detail 1024×512 variants already existed in the approved shared Drive archive. No new concept art, AI upscaling, or invented image detail was needed.

The prior Android CI run [42324519157](https://github.com/mrcalzon02/ReverieVR/actions/runs/42324519157) failed compiling the new `for(bool far:...)` landscape loop. This was not a successful APK build. The loop now uses an indexed `ringIndex`, avoiding that declaration/parser portability issue. The full module also passes local compilation with an empty `far` macro.

## Changes

- `breakwater_trajectory_guide.h` emits 64 bounded crossed 3D dash pairs, dark edge bands and unlit cyan centres. Width grows with range to retain angular visibility, bounded between 0.025 and 3.2m half-width. This is only visual guide thickness, never a larger projectile or target collider.
- The guide samples the same muzzle-origin ballistic equation used by targeting. It ends at the first actual selected target or terrain/water crossing, with an amber contact diamond. Outside permitted traverse or during separate SAM aiming, the existing ghost/preview rules remain authoritative. No fixed head-facing overlay is added.
- Real live controller input now enables the preview in the Mesa fixture. Guide-on/off framebuffer comparisons require visibly changed cyan pixels in both eyes at each of the three stations. Actual observed counts: central 2255/2255; left 2296/2297; right 2313/2313 at 1280×800. These are screenshot pixel counts, not handset performance measurements.
- The four packaged horizons are replaced by the exact approved archived 1024×512 indexed PNG bytes. Their seams match exactly and their manifest byte counts/SHA-1/SHA-256 are updated. Old 512px runtime duplicates are removed; native, Java, optional companion staging and package checks all use the new exact paths.
- Breakwater explicitly chooses nearest-neighbor horizon filtering. Other shared-renderer consumers retain linear filtering by default. Existing 512×256 caller-generated images remain accepted by the renderer, while the packaged loader requires the actual 1024×512 assets.
- Exact-path native horizon caps rise from 32KiB to 128KiB. The PNG decoder adds an explicit width opt-in up to 1024, retaining 512 by default, maximum height 512, CRC checks and the existing 2MiB inflated-data bound. This does not broaden arbitrary file access.
- Encoded sky files total 295,062 bytes, previously 88,875. Four decoded CPU images use 8MiB instead of 2MiB, with corresponding GPU texture storage. No additional per-eye draw pass, per-frame decoding or new dependency is introduced. The original 1774×887 masters remain archived. Phone memory/thermal behavior is not inferred from local tests.

## Validation and limits

- GCC Breakwater visual suite including new trajectory geometry tests and actual panorama decoding, seam equality, retained default bound, opt-in bound, CRC rejection and native asset-scope tests: PASS. Clang unavailable locally.
- Actual native-module Mesa GLES execution: PASS; 16/16 materials, 4/4 1024px skies and 5/5 models load; guide visible in both eyes at all stations; context destruction/recreation and no render-time simulation advance pass.
- Existing packed 1/2/4/8-bit PNG, RGBA, CRC synthetic decoder tests: PASS (optional real CC0 tile fixtures absent, reported as zero).
- Native ABI executable: PASS; no ABI layout changes.
- Broader native documentation guard still reports existing Test Chamber shader-helper drift. No gate was disabled or relaxed to conceal it.
- Android CI must confirm the landscape-loop build repair. No installed APK, physical headset visibility, stereoscopic comfort or sustained phone thermal acceptance is claimed by local captures.
- Weapon/vehicle modeling, individual mounts, UVs and unloading transitions remain open work. These fixes do not close the full rebuild.
