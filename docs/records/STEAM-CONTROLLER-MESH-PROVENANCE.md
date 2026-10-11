# Original Steam Controller mesh provenance and handoff

Date: 2026-10-10, America/Anchorage. Target: RV-0209 offline mesh preparation.
Starting main: `133042ffb4517e4bb9a538f13d9cafc328989b6d`.
Concurrent main advanced to `febe7157f880614dd2a8ef93f839b7a8a17d3df1` during
asset preparation (phone-panel controls and Steam HID VR-entry/status work).
The local unpublished commit was rebased onto that verified revision without
conflicts, side branches or public-history rewriting; those changes were preserved.
AI Project Manager v3.0/Repository Execution Protocol was not available among
session skills; repository doctrine and execution contract were read.

## Authority and exact archive

- [Valve original-controller announcement](https://store.steampowered.com/news/app/353370/view/3931035846865618326).
- [Separate-parts STL ZIP](https://cdn.akamai.steamstatic.com/steam/controller/CAD_data/MM06181.rev01.01.SteamControllerWorkshop03.stl.zip).
- Archive bytes: **763,776,941**.
- Archive SHA256: `957eafa3a93d388fba1fc42769d06603f40800fb5ea2d613cf19e8ddc10df25b`.
- `LICENSE.TXT` bytes: **15,812**.
- License SHA256: `34b2f05d62a57b96f21625b4ad1b024a339d9617bba93f4eeed9f325869e74ce`.
- Exact license inside the archive: Creative Commons Attribution-NonCommercial-
  ShareAlike 4.0 International, copyright 2016 Valve Corporation; commercial
  products require contacting Valve. [License terms](https://creativecommons.org/licenses/by-nc-sa/4.0/).

No 2026 second-generation geometry was used. Source ZIP and 22 STLs are preserved
separately from derivatives in local storage outside Git. The full source license
is retained unchanged in the handoff. General/commercial redistribution rights
have not been established; Git contains tooling and records, not Valve geometry.
The local importer requires explicit noncommercial/sharealike acceptance.

## DEEFM evidence and quality

Intent: prepare source-verified, named render assets with truthful license and
quality gates. Execute: replace the quantization/global-variable converter at
its source; preserve CAD; produce QEM geometry, hashes, reports and previews.
Observe: initial QEM stalled far above budget; source ButtonA had thousands of
nonmanifold edges. The final pipeline uses meshoptimizer first and explicitly
records voxel approximations where needed. Verify: independent output readback,
binary normals, GLB roundtrip and visual inspection. Claim: offline render-asset
handoff only, not watertight manufacturing CAD or device support.

Real run: **29,734,379 input triangles -> 11,882 output triangles**, **22 parts**,
**17 recorded surface approximations**. Runtime binary: **856,384 bytes**, including
flat normals. Named OBJ/GLB/binary counts, finite coordinates, hashes, bounds and
unit normals passed independent readback. Assembled orthographic/isometric and
all individual views were visually inspected: recognizable silhouette, twin-pad/
joystick layout, trigger symmetry, relative assembly positions and orientation.

Boundary/nonmanifold flags remain in BatteryDoorMkVI, CaseBottomGPrime,
CaseFrontGPrime, CaseTopGPrime, MembraneAssyFreeman, PCBMain and ThumbTop29; exact
counts are in the manifest. Largest source/output bounding-box difference is
approximately 6.032 mm for internal PCBMain; external case pieces differ by at
most approximately 1.002 mm in their bounds. These comparisons are not Hausdorff
guarantees. Round holes, letters, tiny features and cavities may lose detail.
Colors are debug part identifiers, not physical materials.

Five regression tests passed: binary solid-header/nonfinite handling, global
budget/minimum allocation, callable conversion/duplicate-name isolation, runtime
handedness/units/normals, truncated-source rejection. Importer hash/extraction
checks are implemented; a full real-archive importer pass is recorded separately
if run. No unrun importer or device gate is claimed.

Local Android tests/build were attempted with Java 17 and Gradle 9.6.1. They
failed at configuration: **SDK location not found**. No APK, release, renderer
device validation or thermal acceptance was produced. No branch, PR or GitHub
Actions was used for validation.

## Offline dependencies

Pinned in requirements/package lock: NumPy 2.2.6 (BSD), matplotlib 3.10.3
(PSF/BSD-compatible), fast-simplification 0.1.13 (MIT), trimesh 4.7.4 (MIT),
SciPy 1.15.3 (BSD), scikit-image 0.25.2 (BSD), meshoptimizer 1.0.1 (MIT; npm
integrity recorded). Sources are PyPI and the official meshoptimizer npm package.
They run only during offline import/QA, introduce no Android API/ABI dependency
or runtime library cost, and can be replaced through the converter/`simplify.mjs`
boundary. Network is needed for initial installation/download, not local
conversion. Asset bytes above are measured, not an APK-size measurement.

## Handoff and acceptance

The user directed a Google Drive handoff so the online instance can perform
programming/hookup. This change ends at offline preparation. Physical mappings
remain empty: pointer/trigger-click observations contain no exact Android report
identity or trigger-side attribution.

Next: download the package, review topology flags, integrate a bounded GLES2
static-buffer renderer into existing controller visualization with correct GL
state/context/lifecycle and shell recovery, then map only observed S9 Android
reports. Keep unsupported gyro/dual-pad/grip behavior absent. Complete Android
build and S9 stereo/interaction/sustained thermal checks before RV-0209 acceptance.
