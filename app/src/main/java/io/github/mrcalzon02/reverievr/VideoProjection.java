package io.github.mrcalzon02.reverievr;

import java.util.Locale;

/**
 * Selects the spatial surface and packed-eye interpretation independently of
 * the Android video decoder. Unknown media defaults to normal 2D playback.
 */
enum VideoProjection {
    FLAT_CINEMA("2D cinema", false, 0),
    FLAT_STEREO_SBS("3D cinema · side-by-side", false, 1),
    FLAT_STEREO_TB("3D cinema · top/bottom", false, 2),
    MONO_EQUIRECTANGULAR_360("2D 360°", true, 0),
    STEREO_EQUIRECTANGULAR_360_SBS("3D 360° · side-by-side", true, 1),
    STEREO_EQUIRECTANGULAR_360_TB("3D 360° · top/bottom", true, 2);

    private final String label;
    private final boolean immersive;
    private final int packing; // 0 = mono, 1 = left/right, 2 = top/bottom

    VideoProjection(String label, boolean immersive, int packing) {
        this.label = label;
        this.immersive = immersive;
        this.packing = packing;
    }

    String displayName() {
        return label;
    }

    boolean isImmersive() {
        return immersive;
    }

    boolean isStereo() {
        return packing != 0;
    }

    VideoProjection next() {
        VideoProjection[] modes = values();
        return modes[(ordinal() + 1) % modes.length];
    }

    /**
     * Packed frames have two views; their combined decoder aspect is not the
     * visible aspect of either eye's image on a floating cinema surface.
     */
    float eyeDisplayAspect(float packedAspect) {
        if (packing == 1) {
            return packedAspect * 0.5f;
        }
        if (packing == 2) {
            return packedAspect * 2.0f;
        }
        return packedAspect;
    }

    /**
     * Region in untransformed video UV coordinates: offsetX, offsetY, scaleX,
     * scaleY. Apply this BEFORE SurfaceTexture's producer transform.
     * Reuse the supplied buffer for both eyes; never allocate in drawEye.
     */
    void fillEyeUvRegion(boolean leftEye, float[] out) {
        out[0] = 0.0f;
        out[1] = 0.0f;
        out[2] = 1.0f;
        out[3] = 1.0f;
        if (packing == 1) {
            out[0] = leftEye ? 0.0f : 0.5f;
            out[2] = 0.5f;
        } else if (packing == 2) {
            // In the onscreen UV convention, the top image is y=0.5..1.
            out[1] = leftEye ? 0.5f : 0.0f;
            out[3] = 0.5f;
        }
    }

    /**
     * Filename tags are only hints, not a claim to detect embedded stereo
     * metadata or infer 3D from a wide aspect ratio. Users can override in VR.
     */
    static VideoProjection suggestForFileName(String fileName) {
        if (fileName == null || fileName.trim().isEmpty()) {
            return FLAT_CINEMA;
        }
        String words = " " + fileName.toLowerCase(Locale.ROOT)
            .replaceAll("[^a-z0-9]+", " ").trim() + " ";
        boolean panorama = words.contains(" 360 ")
            || words.contains(" equirectangular ")
            || words.contains(" eq360 ");
        boolean sbs = words.contains(" sbs ")
            || words.contains(" sidebyside ")
            || words.contains(" left right ");
        boolean tb = words.contains(" tb ")
            || words.contains(" topbottom ")
            || words.contains(" overunder ")
            || words.contains(" over under ")
            || words.contains(" ou ");
        // Conflicting tags are ambiguous: stay mono and require a user choice.
        if (sbs == tb) {
            return panorama ? MONO_EQUIRECTANGULAR_360 : FLAT_CINEMA;
        }
        if (panorama) {
            return sbs
                ? STEREO_EQUIRECTANGULAR_360_SBS
                : STEREO_EQUIRECTANGULAR_360_TB;
        }
        return sbs ? FLAT_STEREO_SBS : FLAT_STEREO_TB;
    }

    static VideoProjection fromPreference(String value) {
        if (value == null || value.trim().isEmpty()) {
            return FLAT_CINEMA;
        }
        try {
            return valueOf(value);
        } catch (IllegalArgumentException exception) {
            return FLAT_CINEMA;
        }
    }
}
