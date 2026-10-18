# Breakwater Battery VR — real Beckenburg PSX combat props

**Owner source:** Beckenburg master asset index at https://docs.google.com/document/d/18G144Qd0-L5rL7NdqxMykqkXaBHtu8-ytr_yYPol6rI/edit, expanded PSX Mega Pack asset folder at https://drive.google.com/drive/folders/1a_Shb7jPYuD38gBXrHS2bSlQV-gSiTLr.

## Provenance and use

The owner confirmed commercial game-use rights for the entire purchased PSX Mega Pack. The game source repository **does not include the original GLB archive or individual raw GLBs**. Three genuine original source models have been selectively compiled into a private, game-specific, low-footprint binary triangle format (`BWM1`), containing **the source's real indexed geometry and sampled source texture RGB**—not arbitrary boxes or model stand-ins.

| In-game asset | Original Drive source | Original SHA-256 | Packed SHA-256 | Face count | Packed bytes |
|---|---|---|---|---:|---:|
| Concrete coastal barricade | [concrete_block_1.glb](https://drive.google.com/file/d/11tX6QyVB-pUi2RIgVrd2aoHYrC1259JL/view) | `5dffb998831ee96af55efb647529e447440b6b06dbffdbdd6ccb6035289226da` | `39a6d9da603c74de8a59f6fe64f152a34bad8cf9136845ad1fcb0f2b84e593b9` | 100 | 2,708 |
| Wooden supply crate | [wooden_crate_1.glb](https://drive.google.com/file/d/1XbElwfpPZazsRsUxdzD_vH6kCSnvCSNS/view) | `29532c345664c083f7db82e8410b9115111ac6a16a55b0b239381fac17bc9b9c` | `a47f42a7a5f49a71d62aa85508d354607d11d2c4dafa71b368e3c4a165d946b6` | 184 | 4,976 |
| Ammunition supply box | [ammo_box_mp_1.glb](https://drive.google.com/file/d/1FnsQzFc-srdZYCuwkkiKa6tB08r6S6Cj/view) | `3e033fb1d87d8c28c315bda0320b3be2071888f8407fab586c1b15f4a0e3e236` | `7d299dfd2c8af97e278624e4c79d0d7223055c1bca248ad6bbe13b8aa8840bd4` | 96 | 2,600 |

The BWM1 converter expanded each glTF indexed triangle in original mesh order. Per-vertex RGB was sampled from the embedded glTF texture and preserved, with world positions quantized to 1/8192 units. Each face becomes a genuine 3D triangle, with original texture colors averaged for the existing GLES2 XYZRGB triangle shader. No fake screenshot, replacement image card, runtime Blender dependency, per-frame asset extraction or Android PNG decoder.

## Native gameplay usage

- `breakwater_beckenburg_mesh.h`: bounds-checked, read-only BWM1 parser and instance emitter, using existing GLES2 triangle batching and day/night lighting.
- `breakwater_module.cpp`: calls the actual host `read_asset` at native module creation, loading the three real packaged `app/src/main/assets/native/breakwater/beckenburg/*.bwm` files **once**. Emits concrete sections along the left/right battery, wooden crates and ammunition supplies at the emplacement, with an extra ammo supply beside the purchased allied support turret.
- **No gameplay hitbox or combat-damage shortcuts:** these are static visual logistics props, deliberately outside the central gun sightline. Unlike the earlier primitive scenery, they are source-model geometry from the purchased library.
- `breakwater_campaign_test.cpp`: reads all three real binaries, rejects corrupt/truncated data and verifies their authentic triangle counts, finite coordinates and color ranges during actual emitter calls.

The selected PSX pack's expanded models are predominantly environment/prop assets. No verified WWII landing craft, gunboat, troop-rig or heavy assault vehicle model has yet been located in this source set. Do not present these three additions as replacements for the enemy ships or infantry. The original 424MB v3.2.1 archive exceeds the connected Drive service's 256MB raw-download limit, so a full v3.2.1 internal contents inspection was not possible in this run; the three selected expanded GLBs have individually verified source bytes.

## Verification and release gates

GitHub file readback and native regression are required; CI acceptance and Galaxy S9 headset screenshots/interaction are separate gates. The models are committed to the game source and rendered when the packaged assets are present. A successful APK build and headset observation must be recorded before claiming final device acceptance.
