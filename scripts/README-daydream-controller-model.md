# Importing the original Daydream controller visual

Preferred source:

- **Daydream Vr Controller**
- author: **rje**
- Sketchfab UID: `c1944c64e06544babc90e9d0aa953551`
- source: https://sketchfab.com/3d-models/daydream-vr-controller-c1944c64e06544babc90e9d0aa953551
- license: Creative Commons Attribution (CC BY)

The original model is intentionally preferred over the convenient
`TechnoBuddhist/VR-Controller-Daydream` derivative because that derivative
repository is distributed under GPL-3.0 while the original Sketchfab model is
CC BY.

## Why the repository does not download it automatically

Sketchfab's Download API requires an authenticated Sketchfab user and returns
temporary download URLs. Do not put a Sketchfab token, OAuth access token, or
account credential into ReverieVR or its public GitHub Actions workflow merely
to obtain a static model once.

Download the original model while signed into Sketchfab and retain the archive
as the authoritative source artifact outside the app repository until it has
been reviewed.

## Handoff

Provide the original downloaded Sketchfab archive (normally glTF/GLB content)
to the project.

Before committing the visual asset:

1. verify the archive/model UID and author;
2. preserve a copy of the CC-BY license/attribution information;
3. inspect meshes/materials/textures and remove unrelated payload;
4. normalize controller scale/orientation to ReverieVR's existing fixed
   virtual-hand origin;
5. preserve separate interactive surfaces where practical for touchpad,
   Home/App and volume-button feedback;
6. optimize only if the source asset proves materially heavier than the
   published 686-triangle / 709-vertex model;
7. keep the current procedural controller renderer as a safe fallback if the
   optional visual asset cannot load.

## Runtime contract

Replacing the procedural visual MUST NOT alter input semantics.

- Physical Daydream: real 3DoF quaternion orientation, fixed virtual position.
- Android gamepad: virtual yaw/pitch pointer, fixed virtual position.
- No positional/6DoF tracking may be inferred.
- The laser emitter, visible controller orientation, and UI raycast must share
  one transform.
- Missing/corrupt visual assets must fall back to the procedural model rather
  than disabling controller input.

See `docs/records/DAYDREAM-CONTROLLER-MODEL-SOURCE.md` for provenance.
