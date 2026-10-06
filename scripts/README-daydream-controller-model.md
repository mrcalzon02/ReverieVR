# Daydream controller visual asset

The controller visual is no longer waiting on an import handoff. ReverieVR now
packages a reviewed low-poly Daydream controller derivative at:

`app/src/main/assets/models/daydream/vr_controller_daydream.obj`

The runtime loader uses the model's separated material/object groups so
touchpad, Home/App, and volume-button feedback can remain visually distinct.

## Provenance

Original:

- **Daydream Vr Controller**
- author: **rje**
- Sketchfab UID: `c1944c64e06544babc90e9d0aa953551`
- source: https://sketchfab.com/3d-models/daydream-vr-controller-c1944c64e06544babc90e9d0aa953551
- original listing license: Creative Commons Attribution (CC BY)

Vendored derivative:

- https://github.com/TechnoBuddhist/VR-Controller-Daydream
- preserves the derivative repository README and GPL-3.0 license beside the
  runtime asset.

See `docs/records/DAYDREAM-CONTROLLER-MODEL-SOURCE.md` and the asset-local
`THIRD_PARTY.md` for the authoritative provenance record.

## Runtime contract

Replacing or revising the visual MUST NOT alter controller semantics.

- Physical Daydream input remains real 3DoF quaternion orientation.
- The model and laser emitter share the same shell-owned anchor/orientation.
- The shell may move that anchor with its small bounded headset/body inertial
  offset so the virtual hand is not visually stranded when the user leans.
- That movement is presentation compensation, not controller 6DoF tracking.
- Android gamepads continue to use virtual yaw/pitch aiming.
- Missing/corrupt visual assets fall back to the procedural controller model
  rather than disabling input.

## Future replacement procedure

If the original Sketchfab archive is later supplied and we choose to replace
this derivative:

1. verify model UID and author;
2. preserve the CC-BY attribution;
3. inspect meshes/materials/textures;
4. normalize scale/orientation to the existing ReverieVR controller anchor;
5. preserve distinct interactive surfaces where practical;
6. update the asset-local provenance notices;
7. retain the procedural fallback;
8. verify the final phone-test APK actually packages the replacement asset.
