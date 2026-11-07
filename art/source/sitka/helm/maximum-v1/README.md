# Sitka maximum helm — physical primitive blockout

Built from the approved **SITKA — MAXIMUM HELM STUDY** reference, retaining wheel left, chart display left, sonar center, radio right, four gauges, lower switch row, twin throttles and rear rail/work lights.

This is an editable asset study, not a playable native console. Units are metres. The plate is 1.72 m wide; fit and reach on each vessel remain to be checked. The separate mesh components total 2,454 triangles before text conversion. No modeled gauge needles: shallow cylinders carry circular texture faces sampled from one 512×128 atlas, four 128px instruments. Twenty-four atlas frames demonstrate changing readings. Demo traces are illustrative, not game telemetry.

## Files and reproduction

- `Sitka_Maximum_Helm.blend`: editable scene; play frames 1–24 at 8 fps. Keep its `textures` folder beside it.
- `Sitka_Maximum_Helm.glb`: static frame-one geometry/material snapshot. GLB does **not** carry the Blender image-sequence or material-index animation.
- `make_textures.py`: run with Python + Pillow to create the gauge frames and four demonstration screen textures.
- `build_helm.py`: run `blender -b -t 4 --python build_helm.py` from any directory after making the textures. Blender 4.3.2 was used.
- `renders/maximum_helm.jpg`: actual model render (full PNG also in the review package).
- `Maximum_Helm_Demo.gif`: actual rendered 24-frame model animation in the downloadable review package; reproducible with the scripts here.

The two display surfaces alternate chart/sonar with radar/camera on the saved timeline. Each display has four separate bezel buttons named for its intended mode. The VHF face shows an explicitly labeled demonstration channel/status. Button metadata records proposed actions with `runtime_wired=false`. These are authored props, not shipped inert game controls.

The reference's trim gauge is preserved as design intent; current runtime instrument sources and missing trim telemetry must be reconciled before integration. Native helm placement currently has sonar right and GPS left; this study retains the approved concept's chart-left/sonar-center arrangement and does not mutate runtime mounts or saves. VHF behavior, mode selection, power/dependencies, foghorn/wipers, radar/camera inputs, interaction hit volumes, vessel attachment, purchase/save flow, stereo readability and Galaxy S9 performance are still integration work.

Build/source validation: Blender creation, GLB export and real Cycles render completed locally. No APK, CI run, native gameplay or device acceptance is claimed.
