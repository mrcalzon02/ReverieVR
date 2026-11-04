# Breakwater VR — generated military surface art

Six original image-generation source textures are stored here, unchanged at 1254×1254 resolution. These assets are **art material sources**, not screenshots of gameplay. Created specifically for the user's native Breakwater Battery VR coastal-defense project, 2026-11-04.

| Source image | Game material |
| --- | --- |
| coarse_shingle_beach_texture.png | shingle.png |
| weathered_coastal_bunker_concrete.png | quay_concrete.png |
| seamless_pixel_art_basalt_rock_texture.png | black_basalt.png |
| seamless_oxidized_coastal_steel_texture.png | oxidized_steel.png |
| seamless_pixel_art_sandbag_wall.png | sandbag_burlap.png |
| chipped_olive_military_metal_texture.png | olive_enamel.png |

The 64×64 optimized Android PNGs live at `app/src/main/assets/native/breakwater/materials/`. RGB values were box-resampled then converted to RGB565-compatible color steps; the PNGs are intended to be decoded **once per module startup**, merged into the existing GLES2 material/sprite atlas and never decoded per eye. All six source PNGs remain outside the APK. These generated references were not obtained from a third-party texture library. No borrowed logos, emblems, military insignia or trademarks are embedded as game markings.

The sandbag master is a wall pattern. Its low-resolution derivative is an interim surface dressing on the existing volumetric sandbag objects and needs headset inspection for visible sub-bag repetition. Source proportions and UV fit should be corrected when reference-device screenshots are available.
