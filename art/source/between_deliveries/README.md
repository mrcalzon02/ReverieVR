# Between Deliveries — owner-authorized original Unreal packages

The owner confirmed on 2026-10-16 that these are their assets and authorized
inclusion in ReverieVR. This directory preserves **34 exact, unmodified UE5.7
source packages**: 24 canonical product icons, seven original DataTables,
the Barfront and two stool source objects. The manifest `SOURCE_ASSETS.json`
records original relative Content paths, lengths, Git object digests and
ownership. It does not claim an open-content CC license.

**Do not put the .uasset files directly into the Android APK.** They are full
editor source packages, not GLES2 textures or native sound formats. Their
embedded 256px JPEG *editor thumbnails* have deliberately not been repackaged
as original-resolution graphics.

The established `scripts/unreal/export_between_deliveries.py` now exports
all 24 canonical icons at their **300 × 300 source resolution**, in addition
to the 24 product mesh/texture groups and original shop DataTable. Run it in
Unreal Editor 5.7 with the original complete Content tree mounted under
`/Game`; the exporter refuses degraded icons and incomplete PNGs and writes
SHA-256-bearing records to its export manifest. The owner has also authorized
the original SoundWave media, handled by the existing audio exporter. Both
exporters require a real Unreal Editor installation, which is not available
in this execution environment. Actual FBX/PNG/WAV outputs and game-runtime
rendering/playback therefore must be verified **after an editor export**.

Maintain the original folder structure and package names when rebuilding the
UE project. The larger untouched source tree remains in the owner's original
Drive folder; this first committed group is not presented as an exhaustive
copy of every model, texture, character or audio source asset.
