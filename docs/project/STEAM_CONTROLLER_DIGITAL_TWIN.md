# Original 2015 Steam Controller digital twin asset

Source handoff: https://drive.google.com/file/d/1ESmQvEwsI3tgR1T0xITtyafPaNjz8wLh/view

**Provenance:** Valve 2016 original Steam Controller workshop CAD; modified mesh via exact welding/QEM and explicitly documented voxel approximation. Licensed **CC BY-NC-SA 4.0**. Keep the handoff's LICENSE.TXT and PROVENANCE.json with all redistributed runtime assets; commercial usage requires Valve permission. No endorsement by Valve.

**Verified package inspection (2026-10-10):** ZIP 1,693,956 bytes, 44 entries, `controller.rvmesh` 856,384 bytes, SHA256 `a2aa69cab532656d022ac88b069056a2e2cf6fd0c136b8b88d923a4b06459a40`; `controller.glb`, `assembled.obj`, 22 individually named OBJ files, validation/manifest/preview and conversion code. Validated `RVSC0001` count 22 and exact 856384-byte parse through all parts; 11,882 triangles. Seven topology warnings are disclosed in VALIDATION.json. Coordinate system is meters, +X right, +Y up, -Z forward. Do not recenter parts independently or treat approximation as exact physical collision mesh.

**Implemented source contracts:** `SteamControllerMesh.read` validates the documented little-endian 22-part, interleaved XYZ/normal file layout and bounds numeric inputs. `SteamControllerIdentity.matches` gates original controller identification behind Valve vendor ID plus Steam Controller product name; other Steam platform devices are not equivalent. Both have JVM test source.

**Still pending for a functional digital twin:** Copy the **actual binary** `controller.rvmesh` plus LICENSE.TXT/PROVENANCE.json into app assets using an authorized binary-capable repository upload path, decode it at runtime, upload GLES2 VBOs with context-loss support, render parts under the existing controller pose/ghost pass, route active primary-device identification (rather than just connected HID presence), and inspect Galaxy S9 input event IDs and poses. A provider/display-name alone is not proof that the physical Steam Controller is actively primary. Do not replace Daydream model until the real mesh is installed and the active input switch is wired. Signed APK and hardware validation not completed.

**Build and release policy:** no new GitHub Actions workflows; respect established project build path, do not bypass remote branch protections, do not claim any artifact or device behavior without verified evidence.
